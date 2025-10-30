//--------------------------------
// Lab 4 - Sample - Lab04_sample.c
//--------------------------------
//
//

#include "init.h"
#include <stdio.h>

void configureADC();

ADC_HandleTypeDef hadc1;

const uint8_t num_samples = 100;   // 10 s / 0.1 s = 100 samples
const float reference_voltage = 3.3f;
const float adc_resolution = 4095.0f;  // 12-bit ADC maximum value


// Main Execution Loop
int main(void)
{
	Sys_Init();          // Initialize system (clock, UART, etc.)
	configureADC();
	printf("\033[2J\033[H");

	float buffer[100] = {0.0f};
	float sum = 0.0f;
	float v_min = reference_voltage;
	float v_max = 0.0f;
	uint8_t index = 0;

	while (1)
	{
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
		uint32_t adc_value = HAL_ADC_GetValue(&hadc1);

		float voltage = (adc_value / adc_resolution) * reference_voltage;

		sum -= buffer[index];
		buffer[index] = voltage;
		sum += voltage;
		index = (index + 1) % num_samples;

		float average = sum / num_samples;

		if (voltage < v_min) v_min = voltage;
		if (voltage > v_max) v_max = voltage;

		printf("ADC=0x%03lX  V=%.3f V  Avg=%.3f  Min=%.3f  Max=%.3f\r\n",
			   adc_value, voltage, average, v_min, v_max);

		HAL_Delay(100);
	}
}

void configureADC()
{
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.ScanConvMode = DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 1;
	hadc1.Init.DMAContinuousRequests = DISABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

	HAL_ADC_Init(&hadc1);

	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = ADC_CHANNEL_6;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
	sConfig.Offset = 0;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}


void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
	// GPIO init
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	if (hadc->Instance == ADC1)
	{
		__HAL_RCC_GPIOA_CLK_ENABLE();

		// Configure PA6 (ADC1_IN4) as analog input
		GPIO_InitStruct.Pin = GPIO_PIN_6;
		GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	}
}
