/**************************************************************
 * If not using B03 revision DISCO board, remove the predefined
 * symbol USE_STM32F769I_DISCO_REVB03 from the project properties:
 *    C/C++ Build -> Settings -> MCU GCC Compiler -> Preprocessor
 * This is only needed if using the LCD
 */


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



int main(void)
{
		Sys_Init();

	    printf("\033[2J\033[;H"); 
	    printf("\033c"); 
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

	    FATFS fs;     /* Pointer to the filesystem object */
		f_mount(&fs,(TCHAR const*)drive, 1);  

	    // List the files
		FRESULT res;
		DIR dir;
		FILINFO fno;
		FILINFO file_array[2];
		FIL file;
	    char line[100]; /* Line buffer */
	    int file_idx = 0;

		res = f_opendir(&dir, (TCHAR const*)drive);   
		for (int i = 0; i < 4; i++) {
			res = f_readdir(&dir, &fno);      
			if (fno.fattrib & AM_DIR) {         
				printf("   <DIR>   %s \r\n", fno.fname);
			} else {                         
				// Store file
				file_array[file_idx] = fno;
				printf("ID:%d %10lu %s \r\n", file_idx, fno.fsize, fno.fname);
				file_idx++;
			}
		}
		f_closedir(&dir);

	    // Select a file
		printf("Select a file id\r\n");
		int select_file = (int)getchar() - '0';
		printf("%d \r\n",select_file);

		// Print out it's contents
		fno = file_array[select_file]; 
		f_open(&file, fno.fname ,FA_READ);
		printf("File contents: \r\n");
		while (f_gets(line, sizeof line, &file)) {
			printf("%s\r\n",line);
		}

	    // Close the file and unmount
		f_close(&file);
		f_mount(&fs,(TCHAR const*)drive, 1);

	    printf("\r\nTask 1 completed! Press any key to continue \r\n");
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
