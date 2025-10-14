//------------------------------------
// Lab 3 - Part 2: UART - Lab3Task2.c
//------------------------------------
//

#include "init.h"

UART_HandleTypeDef USB_UART_6;
char input;
uint8_t start_flag = 1;
uint8_t pound_flag = 0;

char uart_getchar2(UART_HandleTypeDef *huart);
void Init_GPIO();
void commander(char* input, UART_HandleTypeDef *huart);

// Main Execution Loop
int main(void) {
	//Initialize the system
	Sys_Init();
	Init_GPIO();
	fflush(stdout);
	initUart(&USB_UART_6, 38400, USART6);

	input = uart_getchar2(&USB_UART_6);
	HAL_UART_Transmit_IT(&USB_UART_6, (uint8_t*) &input, 1);
	input = uart_getchar2(&USB_UART);
	HAL_UART_Transmit_IT(&USB_UART, (uint8_t*) &input, 1);

	while(start_flag){
	}
}

void Init_GPIO(){
	HAL_NVIC_EnableIRQ(USART6_IRQn);
	HAL_NVIC_EnableIRQ(USART1_IRQn);

	GPIO_InitTypeDef GPIO_LD1;
	GPIO_LD1.Pin = GPIO_PIN_13;
	GPIO_LD1.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_LD1.Pull = GPIO_NOPULL;
	GPIO_LD1.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init(GPIOJ, &GPIO_LD1);
}

void commander(char* input, UART_HandleTypeDef *huart){
	if(*input == 'e'){
		printf("Program Ended");
		putchar(27);
		start_flag--;
	} else if(*input == 'c'){
		printf("\033[2J\033[;H");
		fflush(stdout);
	} else if(*input == 'l'){
		if(huart->Instance == USART6){
			HAL_GPIO_TogglePin(GPIOJ, GPIO_PIN_13);
		}
	}
}

char uart_getchar2(UART_HandleTypeDef *huart){
	char input[1];
	HAL_UART_Receive_IT(huart, (uint8_t *)input, 1);
	return (char)input[0];
}

// Handle USB/UART1 Interrupts with HAL
void USART1_IRQHandler() {
	HAL_UART_IRQHandler(&USB_UART);
}

// Handle UART6 Interrupts with HAL
void USART6_IRQHandler() {
	HAL_UART_IRQHandler(&USB_UART_6);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart){
	input = uart_getchar2(huart);
	if(huart -> Instance == USART1){
		HAL_UART_Transmit_IT(&USB_UART_6, (uint8_t*) &input, 1);
		if(pound_flag){
			commander(&input, huart);
			pound_flag--;
		} else if(input == 27){
			printf("Program Ended");
			putchar(27);
			start_flag--;
		} else if(input == 35){
			pound_flag++;
		} else {
			HAL_UART_Transmit_IT(&USB_UART, (uint8_t*) &input, 1);
		}
	} else if(huart -> Instance == USART6){
		if(pound_flag){
			commander(&input, huart);
			pound_flag--;
		} else if(input == 27){
			printf("Program Ended");
			putchar(27);
			start_flag--;
		} else if(input == 35){
			pound_flag++;
		} else {
			HAL_UART_Transmit_IT(&USB_UART, (uint8_t*) &input, 1);
		}
	}
}




