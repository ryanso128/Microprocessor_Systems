//------------------------------------
// Lab 3 - Part 1: UART - Lab03_uart.c
//------------------------------------
//

#include "init.h"

UART_HandleTypeDef USB_UART_6;
uint8_t echo = 1;

char uart_getchar2(UART_HandleTypeDef *huart){
	char input[1];
	HAL_StatusTypeDef status = HAL_UART_Receive(huart, (uint8_t *)input, 1, 1);
//	printf("sigma");
	if(status == HAL_OK){
		if(input[0] >= 32 && input[0] <=126){
			return (char)input[0];
		}
		else if (input[0] == 27){
			return 27;
		}

	}
	return 0;
}

// Main Execution Loop
int main(void) {
	//Initialize the system
	Sys_Init();
	fflush(stdout);
	initUart(&USB_UART_6, 38400, USART6);

	while(1){
		char input1 = uart_getchar2(&USB_UART);
		if(input1 != 0){
			uart_putchar(&USB_UART_6, &input1);
			if(input1 == 27){
				printf("Program has ended\r\n");
				putchar(27);
				return 1;
			}
			uart_putchar(&USB_UART, &input1);
		}

		//Checking Device 2

		char input2 = uart_getchar2(&USB_UART_6);
		if(input2 != 0){
			uart_putchar(&USB_UART, &input2);
			if(input2 == 27){
				printf("Program has ended\r\n");
				putchar(27);
				return 1;
			}
			uart_putchar(&USB_UART, &input2);
		}
	}
}




