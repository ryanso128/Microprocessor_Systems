//----------------------------------
// Lab 3 - Part 4: SPI - Lab3_Task4.c
//----------------------------------
//

#include "init.h"

SPI_HandleTypeDef hspi2;
uint8_t start_flag = 1;
uint16_t curr_temp = 0;

void configureSPI(void);
char uart_getchar2(UART_HandleTypeDef *huart);
uint8_t reverse(uint8_t b);
void spi_transfer(uint8_t *tx1, uint8_t *tx2, uint8_t *rx1, uint8_t *rx2);
void spi_commander(char *cmd);

int main(void){
	Sys_Init();
	configureSPI();
	
	char cmd;
	printf("\033[2J\033[H");
	char r = 'r';
	spi_commander(&r);
	printf("\r\n");
	
	while (start_flag) {
		fflush(stdout);
		
		// Poll for a single character from stdin (semihosting / SWV)
		cmd = uart_getchar2(&USB_UART);
		spi_commander(&cmd);
	}
}

void configureSPI(void) {
	hspi2.Instance = SPI2;
	hspi2.Init.Mode = SPI_MODE_MASTER;
	hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
	hspi2.Init.NSS = SPI_NSS_SOFT;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256; // under 100 kbit/s
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	HAL_SPI_Init(&hspi2);
}

char uart_getchar2(UART_HandleTypeDef *huart) {
	char input[1];
	HAL_UART_Receive(huart, (uint8_t *)input, 1, 10);
	return (char)input[0];
}
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
	// SPI GPIO initialization structure here
		GPIO_InitTypeDef GPIO_InitStruct;
	if (hspi->Instance == SPI2)
	{
		// Enable SPI GPIO port clocks, set HAL GPIO init structure's values for each
		// SPI-related port pin (SPI port pin configuration), enable SPI IRQs (if applicable), etc.

		// SCK  = PA12
		// MISO = PB14
		// MOSI = PB15

		// SCK
		GPIO_InitStruct.Pin = GPIO_PIN_12;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		// MISO, MOSI
		GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
		GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		//NSS
		GPIO_InitStruct.Pin = GPIO_PIN_11;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;       // Manual control
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		// Set CS high (inactive)
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
	}
}

uint8_t reverse(uint8_t b){
	uint8_t result = 0;
	for(int i = 0; i < 8; i++){
		result <<= 1;
		result |= (b&1);
		b >>= 1;
	}
	return result;
}

void spi_transfer(uint8_t *tx1, uint8_t *tx2, uint8_t *rx1, uint8_t *rx2)
{
	uint8_t curr_tx[2] = {0};
	uint8_t curr_rx[2];
	curr_tx[0] = *tx1;
	curr_tx[1] = *tx2;


	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);   // CS low
	HAL_SPI_TransmitReceive(&hspi2, curr_tx, curr_rx, 2, 10);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);     // CS high

	*rx1 = curr_rx[0];
	*rx2 = curr_rx[1];

}

void spi_commander(char *cmd) {
	uint8_t tx[2] = {0};
	uint8_t rx[2] = {0};
	if (*cmd == 'r')    // Read firmware version
	{
		tx[0] = 0b01000000;
		tx[1] = 0;
		spi_transfer(&tx[0],&tx[1], &rx[0],&rx[1]);
		uint8_t version = reverse(rx[1]);
		uint8_t ver_major = version >> 4;
		uint8_t ver_minor = version & 0x0F;
		printf("STaTS Firmware v%d.%d\r\n", ver_major, ver_minor);
	}
	else if (*cmd == 't')   // Read temperature
	{

		tx[0] = 0b10000000;
		tx[1] = reverse(0x02);
		spi_transfer(&tx[0],&tx[1], &rx[0],&rx[1]);
		// To do
		// Lo register
		tx[0] = 0b00100000;
		tx[1] = 0;
		spi_transfer(&tx[0],&tx[1], &rx[0],&rx[1]);


		uint8_t temp_lo = reverse(rx[1]);

		//uint8_t temp_lo = (rx[0] << 4) + rx[1];
		// Hi register
		tx[0] = 0b00101000;
		tx[1] = 0;
		spi_transfer(&tx[0],&tx[1], &rx[0],&rx[1]);
		uint16_t raw_temp = (reverse(rx[1]) << 8) + temp_lo;
		double total_temp = 357.6 - (0.187 * (double)raw_temp);
		printf("raw_temp: %d\r\n", raw_temp);
		printf("Temperature: %lf\r\n", total_temp);
	}
	else if (*cmd == 'l')
	{
		//Check do you want to read DEVID or change DEVID
		char input;
		printf("Would you like to read or change DEVID\r\n a: Write, b: Read\r\n");
		do{
			input = uart_getchar(&USB_UART, 0);
		}while (input != 'a' && input != 'b');

		if(input == 'a'){
				//Write
			tx[0] = 0b10000000; //Write
			tx[1] = reverse(0x80);
			spi_transfer(&tx[0],&tx[1], &rx[0],&rx[1]);//Unlocks Write protection for DEVID

			//Now we change the DEVID for real using register 9
			tx[0] = 0b11001000;
			tx[1] = (0x05); //Change devid to 5;
			spi_transfer(&tx[0],&tx[1], &rx[0],&rx[1]);

			tx[0] = 0b01001000;
			tx[1] = 0; //Dont care
			spi_transfer(&tx[0],&tx[1], &rx[0],&rx[1]);

			printf("Devid: 0x%X\r\n",reverse(rx[1]));
		} else {
				//Read
				//Initial is 16
			tx[0] = 0b01001000;
			tx[1] = 0; //Dont care
			spi_transfer(&tx[0],&tx[1], &rx[0],&rx[1]);
			printf("Devid: 0x%X\r\n",reverse(rx[1]));
		}
	} else if (*cmd == 'c') {
		// write to register 0, bit 5
		tx[0] = 0b10000000;
		tx[1] = reverse(0x04);
		spi_transfer(&tx[0],&tx[1], &rx[0],&rx[1]);
		printf("STaTS terminal cleared.\r\n");
	} else if (*cmd == 27) {
		printf("========= Menu ===========\r\n Pick a subtask 4-7\r\n");
		char input;
		while (input != 't' && input != 'r' && input != 'c' && input != 'l'){
			input = uart_getchar(&USB_UART, 0);
		}
		spi_commander(&input);
	} else if (*cmd == '-'){
		tx[0] = 0b00110000;
		tx[1] = 0;
		spi_transfer(&tx[0],&tx[1], &rx[0],&rx[1]);

		if(reverse(rx[1]!=0)) printf("Peripheral to Controller: %c\r\n",reverse(rx[1]));

	} else {
		tx[0] = 0b10110000;
		tx[1] = reverse((uint8_t)*cmd);
		spi_transfer(&tx[0],&tx[1], &rx[0],&rx[1]);
		//Send to peripherals

		tx[0] = 0b00110000;
		tx[1] = 0;
		spi_transfer(&tx[0],&tx[1], &rx[0],&rx[1]);

		while(reverse(rx[1]!=0)){
			printf("Peripheral to Controller: %c\r\n",reverse(rx[1]));
			tx[0] = 0b00110000;
			tx[1] = 0;
			spi_transfer(&tx[0],&tx[1], &rx[0],&rx[1]);
		}
	}
}