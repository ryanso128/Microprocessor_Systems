#include "init.h"
#include "stm32f769xx.h"
#include "stm32f7xx_hal.h"
#include "arm_math.h"

#include "stm32f769i_discovery.h"
#include "stm32f769i_discovery_audio.h"



#define SAMPLE_RATE     16000
#define FFT_SIZE        1024
#define HOP_SIZE        1024
#define TARGET_FREQ     440.0f

/* ---- Handles ---- */
DAC_HandleTypeDef      hdac1;
DMA_HandleTypeDef      hdma_dac1_ch1;
TIM_HandleTypeDef      htim6;

extern DMA_HandleTypeDef hDmaTopLeft;
extern DMA_HandleTypeDef hDmaTopRight;

extern SAI_HandleTypeDef haudio_out_sai;

arm_rfft_fast_instance_f32 fft_inst;

extern void DMA2_Stream6_IRQHandler(void);

/* Buffers */
uint16_t  pdmBuf[64];
int16_t   pcmBufA[FFT_SIZE];
int16_t   pcmBufB[FFT_SIZE];
float32_t pcmFloat[FFT_SIZE];

float32_t fftInput[FFT_SIZE];
float32_t fftOutput[FFT_SIZE];
float32_t magnitude[FFT_SIZE/2];
float32_t shiftedFFT[FFT_SIZE];

float32_t ifftOutput[FFT_SIZE];
uint16_t  dacOutA[FFT_SIZE];
uint16_t  dacOutB[FFT_SIZE];

volatile uint8_t activeInputBuf  = 0;
volatile uint8_t activeOutputBuf = 0;

#define AUDIO_IN_CHANNELS    2             // F769I DISCO uses 2 mic channels in BSP
#define AUDIO_FRAME_SAMPLES  FFT_SIZE        // one frame = FFT_SIZE samples per channel
#define USE_CHANNEL_INDEX  1

#define USE_AUDIO_CODEC_OUT   1
#define OUT_VOLUME            90

#define AUDIO_OUT_CHANNELS    2
#define AUDIO_OUT_HALF_FRAMES FFT_SIZE
#define AUDIO_OUT_DMA_FRAMES  (AUDIO_OUT_HALF_FRAMES * 2)

// stereo interleaved DMA buffer
__attribute__((aligned(32)))
static int16_t AudioOutDmaBuf[AUDIO_OUT_DMA_FRAMES * AUDIO_OUT_CHANNELS];

//  processed mono frame to play
__attribute__((aligned(32)))
static int16_t LatestOutMono[FFT_SIZE];
static volatile int LatestOutReady = 0;

static int16_t LatestOutMonoBuf[2][FFT_SIZE];
static volatile int latest_idx = 0;      // which buffer is ready to play



/*************** ADD: Debug + move processing out of ISR ***************/
volatile uint32_t g_in_tc = 0;
volatile uint32_t g_in_err = 0;
volatile uint32_t g_out_tc = 0;
volatile uint32_t g_out_ht = 0;
volatile uint32_t g_out_err = 0;
extern volatile uint32_t g_s6_irq;

static volatile uint8_t g_frame_ready = 0;
static int16_t g_mono_frame[FFT_SIZE];   // buffer filled by ISR, processed in main
static float g_last_f_in = 0.0f;
static float g_last_f_target = 0.0f;
static float g_last_ratio = 1.0f;
static volatile uint8_t g_print_ready = 0;

void Audio_Out_Init(void);

static void FillAudioOutHalf(int halfIndex);
static void ConvertDac12ToPcm16_Mono(const uint16_t *dac12, int16_t *pcm16, int n);
/*************** ADD: “Option A” clock ownership flag ***************/
__attribute__((aligned(32)))
static int16_t MicInBuf[AUDIO_FRAME_SAMPLES * AUDIO_IN_CHANNELS];
static int32_t AudioInScratch[1024];               // required by BSP audio in

static float lastRatio = 1.0f;


void configureDAC();
void configureTIM6();

void Audio_In_Init(void);

void ProcessAudioBlock(int16_t *input, int16_t *out_pcm16);
float DetectFundamental(float32_t *mag, int n);
void PitchShift(float32_t *fft, float32_t *shifted, float ratio);
float ComputeTargetFreq(float f_in);

#define CACHELINE_SZ 32U

static inline void DCache_CleanByAddr(void *addr, int32_t size)
{
    uint32_t a = (uint32_t)addr;
    uint32_t start = a & ~(CACHELINE_SZ - 1U);
    uint32_t end   = (a + (uint32_t)size + CACHELINE_SZ - 1U) & ~(CACHELINE_SZ - 1U);
    SCB_CleanDCache_by_Addr((uint32_t*)start, (int32_t)(end - start));
}

static inline void DCache_InvalidateByAddr(void *addr, int32_t size)
{
    uint32_t a = (uint32_t)addr;
    uint32_t start = a & ~(CACHELINE_SZ - 1U);
    uint32_t end   = (a + (uint32_t)size + CACHELINE_SZ - 1U) & ~(CACHELINE_SZ - 1U);
    SCB_InvalidateDCache_by_Addr((uint32_t*)start, (int32_t)(end - start));
}

/*******************************************************************************
 * MAIN
 ******************************************************************************/
int main(void)
{
    // Course-provided init (clocks, caches, UART, etc.)
    Sys_Init();
    SCB->VTOR = FLASH_BASE;   // 0x08000000
    __DSB();
    __ISB();



    printf("\033[2J\033[H");

    uint32_t *vt = (uint32_t*)SCB->VTOR;
    uint32_t vec_stream6 = vt[16 + DMA2_Stream6_IRQn];
    __enable_irq();




	/* Start the headphone path FIRST */
    printf("Calling Audio_Out_Init...\r\n");
    Audio_Out_Init();
    printf("Returned from Audio_Out_Init\r\n");
	/* Then start the MEMS mic path */
	Audio_In_Init();       // BSP_AUDIO_IN_InitEx + Record

	configureDAC();
	configureTIM6();

    for (int i = 0; i < FFT_SIZE; i++) {
		dacOutA[i] = 2048; // mid-scale (silence)
	}

	// Start TIM6 (sample rate clock for DAC)
	HAL_TIM_Base_Start(&htim6);

	// Start DAC with DMA, playing dacOutA in circular mode
	HAL_DAC_Start_DMA(&hdac1,
					  DAC_CHANNEL_1,
					  (uint32_t*)dacOutA,
					  FFT_SIZE,
					  DAC_ALIGN_12B_R);


    // Init FFT engine once
    arm_rfft_fast_init_f32(&fft_inst, FFT_SIZE);
    g_in_tc  = 0;
    g_in_err = 0;
    g_out_ht = 0;
    g_out_tc = 0;
    static uint32_t prev_in_tc = 0;


    while (1)
    {
//    	HAL_Delay(500);
    	// Heartbeat / sanity prints once every ~500ms
    	    static uint32_t t0 = 0;
    	    if (HAL_GetTick() - t0 > 500)
    	    {
    	        t0 = HAL_GetTick();

    	        printf("IN_TC=%lu OUT_HT=%lu OUT_TC=%lu S6_IRQ=%lu\r\n",
    	               (unsigned long)g_in_tc,
    	               (unsigned long)g_out_ht,
    	               (unsigned long)g_out_tc,
    	               (unsigned long)g_s6_irq);
    	        if (g_print_ready)
    	        {
    	            g_print_ready = 0;
    	            printf("Detected: %.1f Hz | Target: %.1f Hz | Ratio: %.3f\r\n",
    	                   g_last_f_in, g_last_f_target, g_last_ratio);
    	        }
    	    }

    	    // If a new frame is ready, run the heavy DSP here (NOT in ISR)
    	    if (g_frame_ready)
    	    {
    	        g_frame_ready = 0;

    	        // Process g_mono_frame -> LatestOutMono (PCM16 mono)
    	        // Make ProcessAudioBlock write PCM16 directly into LatestOutMono
    	        int write_idx = latest_idx ^ 1;
    	        ProcessAudioBlock(g_mono_frame, (int16_t*)LatestOutMonoBuf[write_idx]);
    	        latest_idx = write_idx;
    	        ConvertDac12ToPcm16_Mono(dacOutA, LatestOutMono, FFT_SIZE);
    	        LatestOutReady = 1;



    	        // Optional: debug peak (NOT in IRQ)
    	        int16_t peak = 0;
    	        for (int i = 0; i < FFT_SIZE; i++) {
    	            int16_t a = LatestOutMono[i]; if (a < 0) a = -a;
    	            if (a > peak) peak = a;
    	        }
    	    }


    }
}

/*******************************************************************************
 * CORE AUDIO PROCESSING ROUTINE
 ******************************************************************************/
void ProcessAudioBlock(int16_t *input, int16_t *out_pcm16)
{
    for (int i = 0; i < FFT_SIZE; i++)
        fftInput[i] = (float32_t)input[i];

    /***** FFT *****/
    arm_rfft_fast_f32(&fft_inst, fftInput, fftOutput, 0);

    /***** Magnitude *****/
    arm_cmplx_mag_f32(fftOutput, magnitude, FFT_SIZE/2);

    /***** Pitch detection *****/
    float f_in = DetectFundamental(magnitude, FFT_SIZE/2);

    // Compute target note frequency
    float f_target = ComputeTargetFreq(f_in);

    // Raw ratio
    float ratio = 1.0f;
    if (f_in > 0.0f)
        ratio = f_target / f_in;

    // Smooth ratio across blocks (retune speed)
    const float alpha = 0.2f;   // 0 → super smooth, 1 → instant
    ratio = alpha * ratio + (1.0f - alpha) * lastRatio;
    lastRatio = ratio;

    /***** Pitch shifting *****/
    PitchShift(fftOutput, shiftedFFT, ratio);

    /***** iFFT *****/
    arm_rfft_fast_f32(&fft_inst, shiftedFFT, ifftOutput, 1);

//PCM
    for (int i = 0; i < FFT_SIZE; i++)
    {
        float x = ifftOutput[i] * 0.5f;     // gain
        if (x > 32767.0f) x = 32767.0f;
        if (x < -32768.0f) x = -32768.0f;
        out_pcm16[i] = (int16_t)x;
    }


    if (f_in > 50.0f && f_in < 1000.0f) {
        g_last_f_in = f_in;
        g_last_f_target = f_target;
        g_last_ratio = ratio;
        g_print_ready = 1;
    }
}

float DetectFundamental(float32_t *mag, int n)
{
    const float fMin = 80.0f;     // lowest fundamental to detect
    const float fMax = 600.0f;    // highest fundamental to detect (voice range)

    // Convert frequency limits → FFT bin indices
    int kMin = (int)(fMin * FFT_SIZE / SAMPLE_RATE);
    int kMax = (int)(fMax * FFT_SIZE / SAMPLE_RATE);
    if (kMax > n - 1) kMax = n - 1;

    float maxVal = 0.0f;
    int   maxIdx = kMin;

    // Find peak ONLY in the allowed range
    for (int k = kMin; k <= kMax; k++)
    {
        if (mag[k] > maxVal)
        {
            maxVal = mag[k];
            maxIdx = k;
        }
    }

    float freq = (float)maxIdx * SAMPLE_RATE / (float)FFT_SIZE;
    return freq;
}


void PitchShift(float32_t *fft, float32_t *shifted, float ratio)
{
    // fft[] and shifted[] are length FFT_SIZE and store complex numbers
    // as interleaved [Re0, Im0, Re1, Im1, ...].

    // Clear output spectrum
    for (int i = 0; i < FFT_SIZE; i++)
        shifted[i] = 0.0f;

    int numBins = FFT_SIZE / 2;  // number of complex bins

    for (int i = 0; i < numBins; i++)
    {
        int newIndex = (int)(i * ratio);

        if (newIndex >= 0 && newIndex < numBins)
        {
            // Source complex value
            float re = fft[2 * i];
            float im = fft[2 * i + 1];

            // Accumulate into destination bin (simple overlap-add if collisions)
            shifted[2 * newIndex]     += re;
            shifted[2 * newIndex + 1] += im;
        }
    }
}

float ComputeTargetFreq(float f_in)
{
    // Ignore silly values: treat as unvoiced / noise
    if (f_in < 50.0f || f_in > 1000.0f) {
        return f_in; // no shift
    }

    // Convert frequency to "note number" using A4 = 440 Hz
    // note = 69 + 12*log2(f/440)
    float note = 69.0f + 12.0f * (logf(f_in / 440.0f) / logf(2.0f));

    // Snap to nearest semitone
    int   note_int  = (int)roundf(note);

    // Convert back to frequency
    float f_target = 440.0f * powf(2.0f, (note_int - 69) / 12.0f);

    return f_target;
}

void configureDAC(void)
{
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    hdac1.Instance = DAC1;
    HAL_DAC_Init(&hdac1);

    DAC_ChannelConfTypeDef DAC_CH1 = {0};
    DAC_CH1.DAC_Trigger      = DAC_TRIGGER_T6_TRGO;   // driven by TIM6
    DAC_CH1.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

    HAL_DAC_ConfigChannel(&hdac1, &DAC_CH1, DAC_CHANNEL_1);
}

void HAL_DAC_MspInit(DAC_HandleTypeDef *hdac)
{
    if (hdac->Instance == DAC1)
    {
        GPIO_InitTypeDef GPIO_InitStruct = {0};

        /* PA4 = DAC_OUT1 */
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitStruct.Pin  = GPIO_PIN_4;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* DMA1 Stream5 Channel7 -> DAC1_CH1 */
        hdma_dac1_ch1.Instance                 = DMA1_Stream5;
        hdma_dac1_ch1.Init.Channel             = DMA_CHANNEL_7;
        hdma_dac1_ch1.Init.Direction           = DMA_MEMORY_TO_PERIPH;
        hdma_dac1_ch1.Init.PeriphInc           = DMA_PINC_DISABLE;
        hdma_dac1_ch1.Init.MemInc              = DMA_MINC_ENABLE;
        hdma_dac1_ch1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        hdma_dac1_ch1.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
        hdma_dac1_ch1.Init.Mode                = DMA_CIRCULAR;     // loop buffer
        hdma_dac1_ch1.Init.Priority            = DMA_PRIORITY_HIGH;
        hdma_dac1_ch1.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;

        HAL_DMA_Init(&hdma_dac1_ch1);

        /* Link DAC <-> DMA */
        __HAL_LINKDMA(hdac, DMA_Handle1, hdma_dac1_ch1);

        /* Enable DMA clock already done above, no NVIC needed if you don’t care about DMA IRQs */
    }
}

void configureTIM6(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim6.Instance = TIM6;

    // Make TIM6 tick at 1 MHz
    uint32_t pclk1   = HAL_RCC_GetPCLK1Freq();
    uint32_t timclk  = pclk1 * 2;                 // APB1 prescaler != 1 → timer clock doubled
    uint32_t presc   = (timclk / 1000000UL) - 1;  // 1 MHz

    htim6.Init.Prescaler         = presc;
    htim6.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim6.Init.Period            = (1000000UL / SAMPLE_RATE) - 1; // 16kHz updates
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(&htim6);

    // Route update event to TRGO so DAC sees it
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig);
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        __HAL_RCC_TIM6_CLK_ENABLE();
        // no GPIO, no NVIC needed
    }
}

void Audio_In_Init(void)
{
    uint8_t st;

    st = BSP_AUDIO_IN_InitEx(
        INPUT_DEVICE_DIGITAL_MIC,
        SAMPLE_RATE,                        // 16 kHz
        DEFAULT_AUDIO_IN_BIT_RESOLUTION,    // 16-bit
        AUDIO_IN_CHANNELS                   // 2 channels
    );
    printf("BSP_AUDIO_IN_Init status = %d\r\n", st);

    // Enable DFSDM DMA IRQs
    HAL_NVIC_SetPriority(AUDIO_DFSDMx_DMAx_TOP_LEFT_IRQ, 0x0F, 0);
    HAL_NVIC_EnableIRQ(AUDIO_DFSDMx_DMAx_TOP_LEFT_IRQ);

    HAL_NVIC_SetPriority(AUDIO_DFSDMx_DMAx_TOP_RIGHT_IRQ, 0x0F, 0);
    HAL_NVIC_EnableIRQ(AUDIO_DFSDMx_DMAx_TOP_RIGHT_IRQ);

    // Scratch buffer for BSP internal use
    st = BSP_AUDIO_IN_AllocScratch(AudioInScratch, sizeof(AudioInScratch));
    printf("BSP_AUDIO_IN_AllocScratch status = %d\r\n", st);

    // Start recording into MicInBuf (size is in 16-bit samples)
    st = BSP_AUDIO_IN_Record((uint16_t*)MicInBuf,
                             AUDIO_FRAME_SAMPLES * AUDIO_IN_CHANNELS); // 16-bit samples

    printf("BSP_AUDIO_IN_Record status = %d\r\n", st);
}

/*************** ADD: Audio OUT init + helpers ***************/
void Audio_Out_Init(void)
{
    uint8_t st;

    st = BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, 90, SAMPLE_RATE);
    printf("BSP_AUDIO_OUT_Init status = %d\r\n", st);

    haudio_out_sai.hdmatx->Instance->CR &= ~(DMA_SxCR_CHSEL_Msk);
    haudio_out_sai.hdmatx->Instance->CR |= (10 << DMA_SxCR_CHSEL_Pos);

    DMA2->HIFCR = DMA_HIFCR_CTCIF6 |
                  DMA_HIFCR_CHTIF6 |
                  DMA_HIFCR_CTEIF6 |
                  DMA_HIFCR_CDMEIF6 |
                  DMA_HIFCR_CFEIF6;

    // IMPORTANT: do NOT force SLOT_02. Either remove slot call or use 0123.
    // BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_0123);

    BSP_AUDIO_OUT_SetVolume(10);
    BSP_AUDIO_OUT_SetMute(AUDIO_MUTE_OFF);
    BSP_AUDIO_OUT_Resume();

    // Use BSP IRQ macro (matches the BSP's DMA stream choice)
    HAL_NVIC_SetPriority(AUDIO_OUT_SAIx_DMAx_IRQ, 5, 0);
    HAL_NVIC_EnableIRQ(AUDIO_OUT_SAIx_DMAx_IRQ);

    FillAudioOutHalf(0);
    FillAudioOutHalf(1);
    DCache_CleanByAddr(AudioOutDmaBuf, sizeof(AudioOutDmaBuf));

    st = BSP_AUDIO_OUT_Play((uint16_t*)AudioOutDmaBuf, sizeof(AudioOutDmaBuf)); // bytes
    printf("BSP_AUDIO_OUT_Play status = %d\r\n", st);
    printf("DMA2 Stream6 Channel = %lu\r\n",
           (unsigned long)((haudio_out_sai.hdmatx->Instance->CR >> 25) & 0x7));
}





static void ConvertDac12ToPcm16_Mono(const uint16_t *dac12, int16_t *pcm16, int n)
{
    for (int i = 0; i < n; i++)
    {
        int32_t x = (int32_t)dac12[i] - 2048; // center
        x = x * 12;                           // conservative gain
        if (x > 32767) x = 32767;
        if (x < -32768) x = -32768;
        pcm16[i] = (int16_t)x;
    }
}


static void FillAudioOutHalf(int halfIndex)
{
    int16_t *dst = &AudioOutDmaBuf[halfIndex * (FFT_SIZE * 2)];

    if (!LatestOutReady) {
        for (int i = 0; i < FFT_SIZE; i++) {
            dst[2*i] = 0;
            dst[2*i+1] = 0;
        }
    } else {
        for (int i = 0; i < FFT_SIZE; i++) {
        	const int16_t *src = LatestOutMonoBuf[latest_idx];
        	int16_t s = src[i];
            dst[2*i] = s;
            dst[2*i+1] = s;
        }
    }

    DCache_CleanByAddr(dst, FFT_SIZE * 2 * sizeof(int16_t));
}






void AUDIO_DFSDMx_DMAx_TOP_LEFT_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hDmaTopLeft);
}

void AUDIO_DFSDMx_DMAx_TOP_RIGHT_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hDmaTopRight);
}

void BSP_AUDIO_IN_TransferComplete_CallBack(void)
{
    // Make sure CPU sees fresh mic data
    DCache_InvalidateByAddr(MicInBuf, sizeof(MicInBuf));

    // Copy one channel into a working mono frame (FAST only)
    for (int i = 0; i < FFT_SIZE; i++)
    {
        g_mono_frame[i] = MicInBuf[AUDIO_IN_CHANNELS * i + USE_CHANNEL_INDEX];
    }

    g_in_tc++;
    g_frame_ready = 1;   // tell main loop: process this frame
}


void BSP_AUDIO_IN_Error_Callback(void)
{
	g_in_err++;
}


void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
    g_out_ht++;
    //BSP_LED_Toggle(LED1);
    FillAudioOutHalf(0);
}
void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
    g_out_tc++;
    //BSP_LED_Toggle(LED1);
    FillAudioOutHalf(1);
}



void BSP_AUDIO_OUT_Error_CallBack(void)
{
#if USE_AUDIO_CODEC_OUT
	g_out_err++;
#endif
}
