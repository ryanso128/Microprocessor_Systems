#include "init.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 256  // Max line length

SPI_HandleTypeDef hspi;
DMA_HandleTypeDef hdma_spi_tx;
DMA_HandleTypeDef hdma_spi_rx;

char inputBuffer[BUFFER_SIZE];
char txBuffer[BUFFER_SIZE];
char rxBuffer[BUFFER_SIZE];
volatile uint16_t bufferIndex = 0;
volatile uint8_t spiTransferComplete = 0;

void SPI_Init(void);
void SPI_DMA_Init(void);
void ProcessLine(void);

void DMA1_Stream4_IRQHandler(void)  // SPI2 TX
{
    HAL_DMA_IRQHandler(&hdma_spi_tx);
}

void DMA1_Stream3_IRQHandler(void)  // SPI2 RX
{
    HAL_DMA_IRQHandler(&hdma_spi_rx);
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi_param)
{
    if(hspi_param->Instance == SPI2)
    {
        spiTransferComplete = 1;
    }
}

void SPI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pins for SPI2
     * PA12 - SCK (SPI2_SCK)
     * PB14 - MISO (SPI2_MISO)
     * PB15 - MOSI (SPI2_MOSI)
     */
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    hspi.Instance = SPI2;
    hspi.Init.Mode = SPI_MODE_MASTER;
    hspi.Init.Direction = SPI_DIRECTION_2LINES;
    hspi.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi.Init.NSS = SPI_NSS_SOFT;
    hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

    HAL_SPI_Init(&hspi)
}

void SPI_DMA_Init(void)
{
    hdma_spi_tx.Instance = DMA1_Stream4;
    hdma_spi_tx.Init.Channel = DMA_CHANNEL_0;
    hdma_spi_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi_tx.Init.PeriphInc = DMA_PINC_DISABLE;  // SPI DR doesn't increment
    hdma_spi_tx.Init.MemInc = DMA_MINC_ENABLE;      // Memory increments
    hdma_spi_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi_tx.Init.Mode = DMA_NORMAL;
    hdma_spi_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_spi_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    HAL_DMA_Init(&hdma_spi_tx);

    __HAL_LINKDMA(&hspi, hdmatx, hdma_spi_tx);

    HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);

    hdma_spi_rx.Instance = DMA1_Stream3;
    hdma_spi_rx.Init.Channel = DMA_CHANNEL_0;
    hdma_spi_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_spi_rx.Init.PeriphInc = DMA_PINC_DISABLE;  // SPI DR doesn't increment
    hdma_spi_rx.Init.MemInc = DMA_MINC_ENABLE;      // Memory increments
    hdma_spi_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi_rx.Init.Mode = DMA_NORMAL;
    hdma_spi_rx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_spi_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    HAL_DMA_Init(&hdma_spi_rx);

    __HAL_LINKDMA(&hspi, hdmarx, hdma_spi_rx);

    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
}

void ProcessLine(void)
{
    if(bufferIndex == 0) return;  // Nothing to send
    memcpy(txBuffer, inputBuffer, bufferIndex);
    memset(rxBuffer, 0, BUFFER_SIZE);

    uint16_t length = bufferIndex;

    printf("\r\nSending %d bytes via SPI DMA...\r\n", length);

    /* Start SPI Transfer with DMA */
    spiTransferComplete = 0;

    if(HAL_SPI_TransmitReceive_DMA(&hspi, (uint8_t*)txBuffer, (uint8_t*)rxBuffer, length) != HAL_OK)
    {
        printf("ERROR: SPI DMA Transfer Failed!\r\n");
        return;
    }

    while(spiTransferComplete == 0);

    printf("Received via SPI: ");
    for(uint16_t i = 0; i < length; i++)
    {
        putchar(rxBuffer[i]);
    }
    printf("\r\n");

    bufferIndex = 0;
    memset(inputBuffer, 0, BUFFER_SIZE);
}

/* ===== MAIN FUNCTION ===== */
int main(void)
{
    Sys_Init();

    // Enable the DWT_CYCCNT register
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR = 0xC5ACCE55;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    DWT->CYCCNT = 0;
    printf("\033[2J\033[;H");

    SPI_Init();
    SPI_DMA_Init();
    printf("Type a line and press Enter to send via SPI\r\n\r\n");
    printf("> ");

    while(1)
    {
        char c = getchar();

        if(c == '\r' || c == '\n')
        {
            /* Process the line */
            ProcessLine();
            printf("> ");
        }
        else if(c == 127 || c == 8)  // Backspace
        {
            if(bufferIndex > 0)
            {
                bufferIndex--;
                inputBuffer[bufferIndex] = '\0';
                printf("\b \b");  // Erase character on screen
            }
        }
        else if(bufferIndex < BUFFER_SIZE - 1)
        {
            inputBuffer[bufferIndex++] = c;
            putchar(c);  // Echo character
            fflush(stdout);
        }
    }
}
