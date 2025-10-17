//----------------------------------
// Lab 3 - Part 3: SPI - Lab3_Task3.c
//----------------------------------
//

#include "init.h"

SPI_HandleTypeDef hspi2;

void configureSPI();

int main(void)
{
	Sys_Init(); 
	configureSPI();
	printf("\033[2J\033[H");
	 printf("SPI2 Loopback Test\r\n");
	 uint8_t tx, rx;
	 while (1) {
	        // Get a byte from terminal (USB_UART)
	        tx = uart_getchar(&USB_UART, 1);

	        // Send and receive simultaneously
	        if (HAL_SPI_TransmitReceive(&hspi2, &tx, &rx, 1, 10) == HAL_OK) {
	            printf("SPI RX: %c (0x%02X)\r\n", rx, rx);
	        } else {
	            printf("SPI Error\r\n");
	        }
	  }
}

void configureSPI()
{
	hspi2.Instance = SPI2;
	hspi2.Init.Mode = SPI_MODE_MASTER;
	hspi2.Init.TIMode = SPI_TIMODE_DISABLE;

	hspi2.Init.Direction = SPI_DIRECTION_2LINES; //Bidirectional
	hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi2.Init.NSS = SPI_NSS_SOFT;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; // ~1 MHz
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;

	hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

	HAL_SPI_Init(&hspi2);
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
	// SPI GPIO initialization structure here
	 GPIO_InitTypeDef GPIO_InitStruct;
	if (hspi->Instance == SPI2)
	{
		GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	}
}
