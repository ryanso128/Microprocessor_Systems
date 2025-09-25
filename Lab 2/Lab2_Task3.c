//----------------------------------
// Lab 2 - Template file
//----------------------------------

// -- Imports ---------------
//
#include "init.h"

//
//
// -- Prototypes ------------
//
void Init_GPIO();
void Init_Timer();

//
//
// -- Code Body -------------
//
TIM_HandleTypeDef hal_timer7;   // TIM7 handle
int main() {
	Sys_Init();
	Init_Timer();
	Init_GPIO();

	while (1){

	}
}

//
//
// -- Init Functions ----------
//
void Init_Timer() {

		// Enable the TIM6 interrupt (through NVIC).
		NVIC->ISER[54 / 32] = (uint32_t) 1 << (54 % 32);

		// Set the timer clock rate and period
		TIM6->PSC = 1079;
		TIM6->ARR = 9999;

		// Enable update events and generation of interrupt from update
		TIM6->EGR |= 1;
		TIM6->DIER |= 1;

		// Start the timer
		TIM6->CR1 |= 1;

		// Enable the TIM7 interrupt (through NVIC).
		HAL_NVIC_EnableIRQ(TIM7_IRQn);

	 	hal_timer7.Instance = TIM7;
	    hal_timer7.Init.Prescaler = 10799; // 1 kHz tick (1 ms)
//	    hal_timer7.Init.CounterMode = TIM_COUNTERMODE_UP;
	    hal_timer7.Init.Period = 9;     // 1 second blink initially
	    hal_timer7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

	    // Enable TIM7 interrupt
		HAL_TIM_Base_MspInit(&hal_timer7);
		HAL_TIM_Base_Init(&hal_timer7);
		HAL_TIM_Base_Start_IT(&hal_timer7);
}

void Init_GPIO() {

	 	GPIO_InitTypeDef GPIO_InitStruct = {0};

	    // PJ13  → output (LED)
	    GPIO_InitStruct.Pin = GPIO_PIN_13;
	    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	    HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);
	    HAL_GPIO_WritePin(GPIOJ,GPIO_PIN_13,GPIO_PIN_SET);

}


void TIM6_DAC_IRQHandler() {
	// Clear
	TIM6->SR &= ~1;



	int curr_period = __HAL_TIM_GET_AUTORELOAD(&hal_timer7);
	int new_period = curr_period + 10; //Updates 1ms time
	 __HAL_TIM_SET_AUTORELOAD(&hal_timer7, new_period);

	// If period is greater than 100ms, reset back to 1ms
	if(new_period >= 999) {
		__HAL_TIM_SET_AUTORELOAD(&hal_timer7, 9);
	}

}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM7) {
    	//Every interrupt trigger flash led output
        HAL_GPIO_TogglePin(GPIOJ, GPIO_PIN_13);
    }
}

void TIM7_IRQHandler() {

	HAL_TIM_IRQHandler(&hal_timer7);
}
