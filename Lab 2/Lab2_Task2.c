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
int count = 0;

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
	NVIC -> ISER[54 >> 5] = (1 << (54 & 31));

	// Set the timer clock rate and period
	TIM6 -> PSC = 10799;
	TIM6 -> ARR = 999;

	// Enable update events and generation of interrupt from update
	TIM6 -> EGR |= 1;
	TIM6 -> DIER |= 1;

	// Start the timer
	TIM6 -> CR1 |= 1;
}

// Change as needed if not using TIM6
void TIM6_DAC_IRQHandler() {
	// Clear Interrupt Bit
	TIM6->SR &= 0;

	// Other code here:
	count++;
	int seconds = count / 10;
	int decimal = count % 10;
	printf("%d.%d\r\n", seconds, decimal);

}
