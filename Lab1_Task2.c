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
#include <ctype.h>

char choice;
int total_chars;
int non_prints;

char esc_char = 27;
int boardCols = 80;
int boardRows = 24;

//------------------------------------------------------------------------------------
// MAIN Routine
//------------------------------------------------------------------------------------
int main(void)
{
    Sys_Init(); // This always goes at the top of main (defined in init.c)

    printf("\033[2J\033[;H"); // Erase screen & move cursor to home position
    fflush(stdout); 


    // Need to enable clock for peripheral bus on GPIO Port J
    __HAL_RCC_GPIOJ_CLK_ENABLE(); 	// Through HAL
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOJEN; // or through registers

    GPIOJ->MODER |= 1024U; //Bitmask for GPIO J Pin 5 initialization (set it to Output mode): 0x00000400U or 1024U in decimal
    GPIOJ->BSRR = (uint16_t)GPIO_PIN_5; // Turn on Green LED (LED2)
    GPIOJ->BSRR = (uint32_t)GPIO_PIN_5 << 16; // Turn off Green LED (LED2)
    GPIOJ->ODR ^= (uint16_t)GPIO_PIN_5; // Toggle LED2

    HAL_Delay(1000); // Pause for a second. This function blocks the program and uses the SysTick and

    // Set background to blue with yellow text and clear the screen
    printf("\033[33;44m\033[2J");
    // Move cursor to line 4, column 4 and create a 4-14 scroll section
    printf("\033[4;13r");
    printf("\033[4;5H");
    total_chars = 0;
    non_prints = 0;

    while(1)
    {
        // Print the total printable and non-printable characters.
    	printf("\0337");
    	printf("\033[%d;0HPrintable: %d, Non-Printable: %d", boardRows, total_chars, non_prints);
    	printf("\0338");

        // Get char. if esc/^[, quit
		choice = getchar();
		if(choice == esc_char){
			return 1;
		}
        // if not printable, print message and continue
		if(!isprint(choice)){
			printf("\0337");
			printf("\033[18;0H\033[2KThe received value $%x is ", choice);
			printf("\033[4m'not printable'\033[24m.");
			printf("\0338");
			non_prints++;
			fflush(stdout);
			continue;
		}

		printf("%c", choice);
		fflush(stdout);
		total_chars++;

        // if at end of line, move to next line
		if(total_chars % (boardCols - 8) == 0){
			printf("\r\n\033[4C");
		}
    }
}

