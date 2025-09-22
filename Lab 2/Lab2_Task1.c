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

//
//
// -- Code Body -------------
//

int main() {
	Sys_Init();
	Init_GPIO();

	while (1){

	}
}

void Init_GPIO() {

	// PJ1 is the 10th pin, register starts 0
	SYSCFG->EXTICR[0] |= (0x9 << 4);

	GPIOJ -> PUPDR |= 4U;//PJ1 = D2

	// Set interrupt enable for EXTI1.
	int IRQn = 7; //EXTI1 is position 7
	NVIC->ISER[IRQn / 32] = (uint32_t) 1 << (IRQn % 32);

	EXTI->IMR |= (1 << 1);
	EXTI->RTSR |= (1 << 1);

	// Initializing PC7 as input pullup
	GPIO_InitTypeDef GPIO_InitStruct;

	GPIO_InitStruct.Pin = GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

// Non-HAL GPIO/EXTI Handler
void EXTI1_IRQHandler() {
    // Clear Interrupt Bit by setting it to 1.
    EXTI->PR |= (1 << 1);
    for(int i =0; i < 10; i++);
    printf("Register Interrupt\r\n");

}

//HAL - GPIO/EXTI Handler
void EXTI9_5_IRQHandler() {
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	printf("PC7 toggled.\r\n");
}
