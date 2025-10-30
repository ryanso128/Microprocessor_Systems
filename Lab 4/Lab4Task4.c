//--------------------------------
// Lab 4 - Sample - Lab04_sample.c
//--------------------------------
//
//

#include "init.h"
ADC_HandleTypeDef ADC1_Handle;
ADC_InitTypeDef ADC1_CONF;
ADC_ChannelConfTypeDef ADC1_CH;

DAC_HandleTypeDef DAC_Handle;
DAC_ChannelConfTypeDef DAC_CH1;

GPIO_InitTypeDef GPIO_ADC;
GPIO_InitTypeDef GPIO_DAC;

// Global variables referring to the filter equation
float x[3] = {0.0f, 0.0f, 0.0f};
float y[2] = {0.0f, 0.0f};
float c[4] = {0.3125f, 0.240385f, 0.3125f, 0.296875f};

void configureADC();
void configureDAC();

// Main Execution Loop
int main(void){
	//Initialize the system
	Sys_Init();
	configureADC();
	configureDAC();

	// Start the ADC Intterupt
	__HAL_ADC_ENABLE_IT(&ADC1_Handle, ADC_IT_EOC);
	HAL_ADC_Start_IT(&ADC1_Handle);

	// Start the DAC
	HAL_DAC_Start(&DAC_Handle,DAC_CHANNEL_1);

	while(1);
}
void configureADC(){
	// Continuous 12-Bit ADC Setup.
	ADC1_CONF.Resolution = ADC_RESOLUTION_12B;
	ADC1_CONF.ScanConvMode = ADC_SCAN_DISABLE;
	ADC1_CONF.ContinuousConvMode = ENABLE;
	ADC1_CONF.DataAlign = ADC_DATAALIGN_RIGHT;
	ADC1_CONF.NbrOfConversion = 1;
	ADC1_CONF.EOCSelection = ADC_EOC_SEQ_CONV;

	ADC1_Handle.Instance = ADC1;
	ADC1_Handle.Init = ADC1_CONF;

	HAL_ADC_Init(&ADC1_Handle);

	// Configure the ADC channel
	ADC1_CH.Channel = ADC_CHANNEL_12;
	ADC1_CH.SamplingTime = ADC_SAMPLETIME_56CYCLES;
	ADC1_CH.Rank = 1;
	HAL_ADC_ConfigChannel(&ADC1_Handle,&ADC1_CH);
}

void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc){
	//PC2, Analog Input
	GPIO_ADC.Pin= GPIO_PIN_2;
	GPIO_ADC.Mode = GPIO_MODE_ANALOG;
	GPIO_ADC.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC,&GPIO_ADC);

	HAL_NVIC_SetPriority(ADC_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(ADC_IRQn);
}

void configureDAC(){
	// Init DAC
	DAC_Handle.Instance = DAC1;
	HAL_DAC_Init(&DAC_Handle);

	// Configure the DAC channel
	DAC_CH1.DAC_Trigger = DAC_TRIGGER_NONE;
	DAC_CH1.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
	HAL_DAC_ConfigChannel(&DAC_Handle,&DAC_CH1,DAC_CHANNEL_1);
}

void HAL_DAC_MspInit(DAC_HandleTypeDef* hdac){
	GPIO_DAC.Pin= GPIO_PIN_4;
	GPIO_DAC.Mode = GPIO_MODE_ANALOG;
	GPIO_DAC.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA,&GPIO_DAC);
}

void ADC_IRQHandler(){
    HAL_ADC_IRQHandler(&ADC1_Handle);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc){
    if(hadc->Instance == ADC1)    {
    	// Read the current ADC value
        x[0] = (float)HAL_ADC_GetValue(&ADC1_Handle);

        // Assembly implementation
        asm volatile(
    		"vmul.f32 s0, %[x[0]], %[c[0]]\n" // Multiply c[0] and x[0]
    		"vmla.f32 s0, %[x[1]], %[c[1]]\n" // Multiply c[1] and x[1] and accumlate with the last product
    		"vmla.f32 s0, %[x[2]], %[c[2]]\n" // Multiply c[2] and x[2] and accumlate with the last product
    		"vmla.f32 s0, %[y[1]], %[c[3]]\n" // Multiply c[3] and y[1] and accumlate with the last product
    		"vstr s0, %[y[0]]\n" // Store the result into y[0]
    			:[y[0]] "=m" (y[0])
    			:[x[0]] "t#" (x[0]), [x[1]] "t#" (x[1]), [x[2]] "t#" (x[2]), [y[1]] "t#" (y[1]),
				 [c[0]] "t#" (c[0]), [c[1]] "t#" (c[1]), [c[2]] "t#" (c[2]), [c[3]] "t#" (c[3])
    			:"s0"
    	);

        // Update the DAC value
        HAL_DAC_SetValue(&DAC_Handle, DAC_CHANNEL_1, DAC_ALIGN_12B_R, (uint32_t)y[0]);

        // Update stored variables for next interrupt
        x[2] = x[1];
        x[1] = x[0];
        y[1] = y[0];
    }
}
