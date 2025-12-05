
/**************************************************************
 * If not using B03 revision DISCO board, remove the predefined
 * symbol USE_STM32F769I_DISCO_REVB03 from the project properties:
 *    C/C++ Build -> Settings -> MCU GCC Compiler -> Preprocessor
 * This is only needed if using the LCD
 */


#include <stdlib.h>

#include "stm32f769xx.h"
#include "stm32f7xx_hal.h"
#include "stm32f769i_discovery_lcd.h"

#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "jpeg_utils.h"

#include "init.h"
#include "helper_functions.h"

/* Defines */
#define LCD_FRAME_BUFFER        0xC0000000
#define JPEG_OUTPUT_DATA_BUFFER 0xC0200000

/* Global Variables */
uint32_t MCU_TotalNb = 0;
uint32_t num_bytes_decoded = 0;
uint8_t input = 0x1B;

JPEG_HandleTypeDef jpeg_handle;
JPEG_ConfTypeDef jpeg_info;
DMA2D_HandleTypeDef DMA2D_Handle;
char drive[4];
uint8_t buf[20000]; /* buffer */
FIL jpegFile;          // file handle used during decode
UINT jpeg_bytes_read;  // number of bytes just read from SD
uint8_t *jpeg_input_ptr = NULL;
uint32_t jpeg_input_remaining = 0;


int main(void)
{
	Sys_Init();

    printf("\033[2J\033[;H"); // Erase screen & move cursor to home position
    printf("\033c"); // Reset device
    fflush(stdout);

    /*
     * Leave this section even if you don't have an LCD
     */
    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);
    BSP_LCD_SelectLayer(0);
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    /* Task 1 */
    /* List all the files in the top directory of the SD Card */
    // Link FATFS Driver
    FATFS_LinkDriver(&SD_Driver, drive);
    // Mount the disk
    FATFS fs;   
	f_mount(&fs,(TCHAR const*)drive, 1);        

    // List the files
	FRESULT res;
	DIR dir;
	FILINFO fno;
	FILINFO file_array[2];
	FIL file;
    int file_idx = 0;
    UINT bytes_read;

	res = f_opendir(&dir, (TCHAR const*)drive); 
	for (int i = 0; i < 4; i++) {
		res = f_readdir(&dir, &fno);          
		if (fno.fattrib & AM_DIR) {            
			printf("   <DIR>   %s \r\n", fno.fname);
		} else {                              
			// Store all found files
			file_array[file_idx] = fno;
			printf("ID:%d %10lu %s \r\n", file_idx, fno.fsize, fno.fname);
			file_idx++;
		}
	}
	f_closedir(&dir);

	// Select a file
	printf("Select a file\r\n");
	int select_file = (int)getchar() - '0';
	printf("%d\r\n", select_file);

	// Get the selected file info
	fno = file_array[select_file];

	// Open the JPEG file (keep it open for streaming)
	f_open(&jpegFile, fno.fname, FA_READ);

	// Read first chunk into buffer
	f_read(&jpegFile, buf, fno.fsize, &jpeg_bytes_read);
	f_close(&jpegFile);
	printf("fsize = %lu, bytes_read = %u\r\n", fno.fsize, jpeg_bytes_read);

	jpeg_input_ptr       = buf;
	jpeg_input_remaining = jpeg_bytes_read;

    // Initialize JPEG
    jpeg_handle.Instance = JPEG;
    HAL_JPEG_Init(&jpeg_handle);
    num_bytes_decoded = 0;

    // Decode the passed file without DMA
    printf("Before Decode\r\n");
    HAL_JPEG_Decode(&jpeg_handle,buf, jpeg_bytes_read,(uint8_t*)JPEG_OUTPUT_DATA_BUFFER,8000, HAL_MAX_DELAY);
    printf("Decode Complete\r\n");
    HAL_JPEG_GetInfo(&jpeg_handle,&jpeg_info);
    printf("Width: %lu, Height: %lu \r\n",jpeg_info.ImageHeight,jpeg_info.ImageWidth);
    if(jpeg_info.ColorSpace != JPEG_YCBCR_COLORSPACE){
    	printf("NOT YCbCr\r\n");
    }

    // Print the image in PuTTy
    uint8_t *raw_output = colorConversion((uint8_t *)JPEG_OUTPUT_DATA_BUFFER, num_bytes_decoded);
    if (input != 0x1B) printPutty(raw_output, &jpeg_info);

    printf("\r\nTask 2 completed! Press any key to continue \r\n");
}


/*
 * Callback called whenever the JPEG needs more data
 */
void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbDecodedData)
{
	    // Move the pointer forward by the number of bytes just consumed
	    if (NbDecodedData > jpeg_input_remaining) {
	        NbDecodedData = jpeg_input_remaining;
	    }

	    jpeg_input_ptr       += NbDecodedData;
	    jpeg_input_remaining -= NbDecodedData;

	    if (jpeg_input_remaining > 0) {
	        // There is still compressed data left in buf; feed the rest
	        HAL_JPEG_ConfigInputBuffer(hjpeg, jpeg_input_ptr, jpeg_input_remaining);
	    } else {
	        // No more compressed data to give
	        HAL_JPEG_ConfigInputBuffer(hjpeg, jpeg_input_ptr, 0);
	    }
}

void HAL_JPEG_DataReadyCallback(JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength)
{
	num_bytes_decoded += OutDataLength;
	uint8_t *next = pDataOut + OutDataLength;
	HAL_JPEG_ConfigOutputBuffer(hjpeg, next, 8000);
}

void HAL_JPEG_DecodeCpltCallback(JPEG_HandleTypeDef *hjpeg)
{
	printf("JPEG has finished decoding \r\n");
	input = 0;
}

// Provided code for callback
// Called when the jpeg header has been parsed
// Adjust the width to be a multiple of 8 or 16 (depending on image configuration) (from STM examples)
// Get the correct color conversion function to use to convert to RGB
void HAL_JPEG_InfoReadyCallback(JPEG_HandleTypeDef *hjpeg, JPEG_ConfTypeDef *pInfo)
{
	// Have to add padding for DMA2D
	if(pInfo->ChromaSubsampling == JPEG_420_SUBSAMPLING)
	{
		if((pInfo->ImageWidth % 16) != 0)
			pInfo->ImageWidth += (16 - (pInfo->ImageWidth % 16));

		if((pInfo->ImageHeight % 16) != 0)
			pInfo->ImageHeight += (16 - (pInfo->ImageHeight % 16));
	}

	if(pInfo->ChromaSubsampling == JPEG_422_SUBSAMPLING)
	{
		if((pInfo->ImageWidth % 16) != 0)
			pInfo->ImageWidth += (16 - (pInfo->ImageWidth % 16));

		if((pInfo->ImageHeight % 8) != 0)
			pInfo->ImageHeight += (8 - (pInfo->ImageHeight % 8));
	}

	if(pInfo->ChromaSubsampling == JPEG_444_SUBSAMPLING)
	{
		if((pInfo->ImageWidth % 8) != 0)
			pInfo->ImageWidth += (8 - (pInfo->ImageWidth % 8));

		if((pInfo->ImageHeight % 8) != 0)
			pInfo->ImageHeight += (8 - (pInfo->ImageHeight % 8));
	}

	if(JPEG_GetDecodeColorConvertFunc(pInfo, &pConvert_Function, &MCU_TotalNb) != HAL_OK)
	{
		printf("Error getting DecodeColorConvertFunct\r\n");
		while(1);
	}
}

void JPEG_IRQHandler(void)
{
	HAL_JPEG_IRQHandler(&jpeg_handle);
}


