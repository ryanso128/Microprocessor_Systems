#include "init.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1000

uint32_t sourceBuffer[BUFFER_SIZE];
uint32_t destBuffer_DMA[BUFFER_SIZE];
uint32_t destBuffer_SW[BUFFER_SIZE];

DMA_HandleTypeDef hdma_memtomem;
volatile uint8_t dmaTransferComplete = 0;

void DMA_Init_MemToMem(void);
void CopyBuffer_Software(uint32_t *src, uint32_t *dst, uint32_t size);
void CopyBuffer_DMA(uint32_t *src, uint32_t *dst, uint32_t size);
void PerformanceTest(int buffer);

void DMA2_Stream0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_memtomem);
    dmaTransferComplete = 1;
}

void HAL_DMA_XferCpltCallback(DMA_HandleTypeDef *hdma)
{
    if(hdma->Instance == DMA2_Stream0)
    {
        dmaTransferComplete = 1;
    }
}

void DMA_Init_MemToMem(void)
{
    hdma_memtomem.Instance = DMA2_Stream0;
    hdma_memtomem.Init.Channel = DMA_CHANNEL_0;
    hdma_memtomem.Init.Direction = DMA_MEMORY_TO_MEMORY;
    hdma_memtomem.Init.PeriphInc = DMA_PINC_ENABLE;      // Source increment
    hdma_memtomem.Init.MemInc = DMA_MINC_ENABLE;         // Destination increment
    hdma_memtomem.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;  // 32-bit
    hdma_memtomem.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;     // 32-bit
    hdma_memtomem.Init.Mode = DMA_NORMAL;
    hdma_memtomem.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_memtomem.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    HAL_DMA_Init(&hdma_memtomem);

    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

void CopyBuffer_Software(uint32_t *src, uint32_t *dst, uint32_t size)
{
    for(uint32_t i = 0; i < size; i++)
    {
        dst[i] = src[i];
    }
}

void CopyBuffer_DMA(uint32_t *src, uint32_t *dst, uint32_t size)
{
    dmaTransferComplete = 0;
    HAL_DMA_Start_IT(&hdma_memtomem, (uint32_t)src, (uint32_t)dst, size);
    while(dmaTransferComplete == 0);
}

void PerformanceTest(int buffer)
{
    uint32_t cycles_sw, cycles_dma;

    printf("Buffer Size: %lu elements\r\n", buffer);

    // Initialize buffers
    for(uint32_t i = 0; i < buffer; i++)
    {
        sourceBuffer[i] = i + 0xDEAD0000;
    }

    memset(destBuffer_SW, 0, sizeof(destBuffer_SW));
    memset(destBuffer_DMA, 0, sizeof(destBuffer_DMA));

    // Software Copy
    DWT->CYCCNT = 0;
    CopyBuffer_Software(sourceBuffer, destBuffer_SW, buffer);
    cycles_sw = DWT->CYCCNT;

    // DMA Copy
    DWT->CYCCNT = 0;
    CopyBuffer_DMA(sourceBuffer, destBuffer_DMA, buffer);
    cycles_dma = DWT->CYCCNT;

    // Verify Correctness
    uint8_t sw_pass = 1, dma_pass = 1;

    for(uint32_t i = 0; i < buffer; i++)
    {
        if(destBuffer_SW[i] != sourceBuffer[i])
            sw_pass = 0;
        if(destBuffer_DMA[i] != sourceBuffer[i])
            dma_pass = 0;
    }

    printf("Software Copy: %7lu cycles - %s\r\n", cycles_sw, sw_pass ? "PASS" : "FAIL");
    printf("DMA Copy:      %7lu cycles - %s\r\n\n", cycles_dma, dma_pass ? "PASS" : "FAIL");
}

int main(void)
{
    Sys_Init();

    // Enable the DWT_CYCCNT register
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR = 0xC5ACCE55;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    printf("\033[2J\033[;H");

    DMA_Init_MemToMem();

    int buffers[3] = {10, 100, 1000};
    for(int i = 0; i < 3; i++){
    	PerformanceTest(buffers[i]);
    }

    while(1);
}
