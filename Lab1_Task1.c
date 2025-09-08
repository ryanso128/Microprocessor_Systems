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

char choice;
//------------------------------------------------------------------------------------
// MAIN Routine
//------------------------------------------------------------------------------------
int main(void)
{
    Sys_Init(); // This always goes at the top of main (defined in init.c)

    printf("\033[2J\033[;H"); // Erase screen & move cursor to home position
    fflush(stdout);

    __HAL_RCC_GPIOJ_CLK_ENABLE(); 	// Through HAL
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOJEN;

    GPIOJ->MODER |= 1024U; //Bitmask for GPIO J Pin 5 initialization (set it to Output mode): 0x00000400U or 1024U in decimal
    GPIOJ->BSRR = (uint16_t)GPIO_PIN_5; // Turn on Green LED (LED2)
    GPIOJ->BSRR = (uint32_t)GPIO_PIN_5 << 16; // Turn off Green LED (LED2)
    GPIOJ->ODR ^= (uint16_t)GPIO_PIN_5; // Toggle LED2


    HAL_Delay(1000);

    while(1)
    {
    	// Get char. if ascii value == 27 quit.
    	// Otherwise, print ascii character.
		choice = getchar();
		if(choice == 27) return 1;
		printf("The keyboard character is %c.\r\n", choice);
    }
}

