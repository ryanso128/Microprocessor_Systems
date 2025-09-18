//------------------------------------------------------------------------------------
// Hello.c
//------------------------------------------------------------------------------------
//
// Test program to demonstrate serial port I/O.  This program writes a message on
// the console using the printf() function, and reads characters using the getchar()
// function.  An ANSI escape sequence is used to clear the screen if a '2' is typed.
// A '1' repeats the message and the program responds to other input characters with
// an appropriate message.
//
// Any valid keystroke turns on the green LED on the board; invalid entries turn it off
//


//------------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------------
#include "stm32f769xx.h"
#include "hello.h"

#include<stdint.h>

void GPIO_Init();
//------------------------------------------------------------------------------------
// MAIN Routine
//------------------------------------------------------------------------------------
int main(void)
{
    Sys_Init(); // This always goes at the top of main (defined in init.c)
    GPIO_Init();
    HAL_Delay(1000); // Pause for a second. This function blocks the program and uses the SysTick and
                     // associated handler to keep track of time.

    while(1)
    {
    	 if(!((GPIOC -> IDR >> 7) & 1U)){
    		 GPIOJ -> ODR |= 1U << 13;
    	 } else {
    		 GPIOJ -> ODR &= ~(1U << 13);
    	 }

    	 if(!((GPIOC -> IDR >> 6) & 1U)){
    		 GPIOJ -> ODR |= 32U;
    	 } else {
    		 GPIOJ -> ODR &= ~(32U);
    	 }

    	 if(!((GPIOJ -> IDR >> 1) & 1U)){
    		 GPIOA -> ODR |= 1U << 12;
    	 } else {
    		 GPIOA -> ODR &= ~(1U << 12);
    	 }

    	 if(!((GPIOF -> IDR >> 6) & 1U)){
    		 GPIOD -> ODR &= ~(1U << 4);
    	 } else {
    		 GPIOD -> ODR |= 1U << 4;
    	 }
    }
}

void GPIO_Init(){
    // Initialization of all GPIO clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOJEN;
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN;

	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

	// Initialize input GPIO
	// Input 1 - PC7
	GPIOC -> MODER &= ~(3U << 14);
	GPIOC -> PUPDR |= 1U << 14;

	// Input 2 - PC6
	GPIOC -> MODER &= ~(3U << 12);
	GPIOC -> PUPDR |= 1U << 12;

	// Input 3 - PJ1
	GPIOJ -> MODER &= ~(3U << 2);
	GPIOJ -> PUPDR |= 1U << 2;

	// Input 4 - PF6
	GPIOF -> MODER &= ~(3U << 12);
	GPIOF -> PUPDR |= 1U << 12;

	// Initialize output GPIO
	// LED 1 - PJ13
	GPIOJ -> MODER |= 1U << 26;
	GPIOJ -> OTYPER &= ~(1U << 13);
	GPIOJ -> PUPDR &= ~(3U << 26);

	// LED 2 - PJ5
	GPIOJ -> MODER |= 1024U; // Initialize PJ5 to output
	GPIOJ -> OTYPER &= ~(32U);
	GPIOJ -> PUPDR &= ~(3U << 10);

	// LED 3 - PA12
	GPIOA -> MODER |= 1U << 24;
	GPIOA -> OTYPER &= ~(1U << 12);
	GPIOA -> PUPDR &= ~(3U << 24);

	// LED 4 - PD4
	GPIOD -> MODER |= 1U << 8;
	GPIOD -> OTYPER &= ~(1U << 4);
	GPIOD -> PUPDR &= ~(3U << 8);
}

