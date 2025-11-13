//--------------------------------
// Lab 5 - Task 3 - IIR via DMA
//--------------------------------

#include "init.h"
#include <string.h>

ADC_HandleTypeDef       ADC1_Handle;
DAC_HandleTypeDef       DAC_Handle;
TIM_HandleTypeDef       TIM6_Handle;

DMA_HandleTypeDef       hdma_adc1;  // ADC1 -> DMA2 Stream0 Channel 0 (common mapping)
DMA_HandleTypeDef       hdma_dac1;  // DAC1 CH1 -> DMA1 Stream5 Channel 7 (common mapping)

static float x_1 = 0.0f, x_2 = 0.0f;
static float y_1 = 0.0f;

static const float c_1 = 0.3125f;
static const float c_2 = 0.240385f;
static const float c_3 = 0.3125f;
static const float c_4 = 0.296875f;

#define DMA_BUF_LEN   256U              // use 100–1000 for checkoff; 256 is a good starting point
static __attribute__((aligned(4))) uint16_t adc_buf[DMA_BUF_LEN];
static __attribute__((aligned(4))) uint16_t dac_buf[DMA_BUF_LEN];

static void configure_timer_100kHz(void);
static void configure_adc_dma(void);
static void configure_dac_dma(void);
static inline void process_block(uint16_t *in, uint16_t *out, uint32_t n);

int main(void)
{
    Sys_Init();

    configure_timer_100kHz();  // TIM6 as common trigger
    configure_adc_dma();       // ADC1 + DMA circular, triggered by TIM6 TRGO
    configure_dac_dma();       // DAC1 + DMA circular, triggered by TIM6 TRGO

    HAL_DAC_Start_DMA(&DAC_Handle, DAC_CHANNEL_1, (uint32_t*)dac_buf, DMA_BUF_LEN, DAC_ALIGN_12B_R);

    HAL_ADC_Start_DMA(&ADC1_Handle, (uint32_t*)adc_buf, DMA_BUF_LEN);

    HAL_TIM_Base_Start(&TIM6_Handle);

    while (1);
}

void DMA2_Stream0_IRQHandler(void)  { HAL_DMA_IRQHandler(&hdma_adc1); }    // ADC1
void DMA1_Stream5_IRQHandler(void)  { HAL_DMA_IRQHandler(&hdma_dac1); }    // DAC1 CH1

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);  // timing marker
        process_block(&adc_buf[0], &dac_buf[0], DMA_BUF_LEN / 2);
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);  // timing marker
        process_block(&adc_buf[DMA_BUF_LEN/2], &dac_buf[DMA_BUF_LEN/2], DMA_BUF_LEN / 2);
    }
}

static inline void process_block(uint16_t *in, uint16_t *out, uint32_t n)
{
    for (uint32_t k = 0; k < n; ++k) {
        float x_0 = (float)in[k];                 // raw ADC count (0..4095)
        float y_0 = c_1*x_0 + c_2*x_1 + c_3*x_2 + c_4*y_1;

        x_2 = x_1; x_1 = x_0; y_1 = y_0;

        if (y_0 < 0.0f)        y_0 = 0.0f;
        if (y_0 > 4095.0f)     y_0 = 4095.0f;
        out[k] = (uint16_t)(y_0 + 0.5f);
    }
}

static void configure_timer_100kHz(void)
{
    TIM6_Handle.Instance = TIM6;
    TIM6_Handle.Init.CounterMode = TIM_COUNTERMODE_UP;

    TIM6_Handle.Init.Prescaler = 215;   // (PSC+1) = 216 -> ~1 MHz if timer clock ~216 MHz
    TIM6_Handle.Init.Period    = 9;     // (ARR+1) = 10  -> 100 kHz
    TIM6_Handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&TIM6_Handle);

    TIM_MasterConfigTypeDef mcfg = {0};
    mcfg.MasterOutputTrigger = TIM_TRGO_UPDATE;
    mcfg.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&TIM6_Handle, &mcfg);
}

static void configure_adc_dma(void)
{
    /* PC2 -> ADC1_IN12 */
    GPIO_InitTypeDef g = {0};
    g.Pin  = GPIO_PIN_2;
    g.Mode = GPIO_MODE_ANALOG;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &g);

    ADC1_Handle.Instance = ADC1;
    ADC1_Handle.Init.Resolution           = ADC_RESOLUTION_12B;
    ADC1_Handle.Init.ScanConvMode         = ADC_SCAN_DISABLE;
    ADC1_Handle.Init.ContinuousConvMode   = DISABLE;
    ADC1_Handle.Init.ExternalTrigConv     = ADC_EXTERNALTRIGCONV_T6_TRGO;
    ADC1_Handle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    ADC1_Handle.Init.DataAlign            = ADC_DATAALIGN_RIGHT;
    ADC1_Handle.Init.NbrOfConversion      = 1;
    ADC1_Handle.Init.DMAContinuousRequests= ENABLE;   // continuous DMA requests in circular mode
    HAL_ADC_Init(&ADC1_Handle);

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = ADC_CHANNEL_12;
    ch.Rank         = 1;
    ch.SamplingTime = ADC_SAMPLETIME_56CYCLES;
    HAL_ADC_ConfigChannel(&ADC1_Handle, &ch);

    hdma_adc1.Instance                 = DMA2_Stream0;
    hdma_adc1.Init.Channel             = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode                = DMA_CIRCULAR;
    hdma_adc1.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_adc1.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_adc1);

    // Link ADC<->DMA and NVIC
    __HAL_LINKDMA(&ADC1_Handle, DMA_Handle, hdma_adc1);
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

static void configure_dac_dma(void)
{
    /* PA4 -> DAC1_OUT1 */
    GPIO_InitTypeDef g = {0};
    g.Pin  = GPIO_PIN_4;
    g.Mode = GPIO_MODE_ANALOG;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);

    DAC_Handle.Instance = DAC1;
    HAL_DAC_Init(&DAC_Handle);
    DAC_ChannelConfTypeDef ch = {0};
    ch.DAC_Trigger      = DAC_TRIGGER_T6_TRGO;
    ch.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&DAC_Handle, &ch, DAC_CHANNEL_1);

    hdma_dac1.Instance                 = DMA1_Stream5;
    hdma_dac1.Init.Channel             = DMA_CHANNEL_7;
    hdma_dac1.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_dac1.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_dac1.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_dac1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_dac1.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma_dac1.Init.Mode                = DMA_CIRCULAR;
    hdma_dac1.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_dac1.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_dac1);

    /* Link DAC<->DMA and NVIC */
    __HAL_LINKDMA(&DAC_Handle, DMA_Handle1, hdma_dac1); // Channel 1 uses DMA_Handle1 field
    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
}
