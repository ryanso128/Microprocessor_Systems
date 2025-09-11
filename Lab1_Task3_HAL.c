#include "stm32f769xx.h"
#include "hello.h"

#include<stdint.h>

void SystemClock_Config(void);

void GPIO_Init();


//------------------------------------------------------------------------------------
// MAIN Routine
//------------------------------------------------------------------------------------
int main(void)
{
    Sys_Init(); // This always goes at the top of main (defined in init.c)
    GPIO_Init();
    HAL_Delay(1000); // Pause for a second. This function blocks the program and uses the SysTick and
    while(1)
    {


        HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_13, !HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_7));
        HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_5, !HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_6));
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, !HAL_GPIO_ReadPin(GPIOJ,GPIO_PIN_1));
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, HAL_GPIO_ReadPin(GPIOF,GPIO_PIN_6)); //LD4 uses inverted logic
        HAL_Delay(100);


        }
}
void GPIO_Init(){
		__HAL_RCC_GPIOI_CLK_ENABLE();
	    __HAL_RCC_GPIOJ_CLK_ENABLE();
	    __HAL_RCC_GPIOC_CLK_ENABLE();
	    __HAL_RCC_GPIOF_CLK_ENABLE();
	    __HAL_RCC_GPIOD_CLK_ENABLE();
	    __HAL_RCC_GPIOA_CLK_ENABLE();


	    GPIO_InitTypeDef GPIO_Output1 = {0};
	    GPIO_InitTypeDef GPIO_Output2 = {0};
	    GPIO_InitTypeDef GPIO_Output3 = {0};

	    GPIO_InitTypeDef GPIO_Input1 = {0};
	    GPIO_InitTypeDef GPIO_Input2 = {0};
	    GPIO_InitTypeDef GPIO_Input3 = {0};

	    // LEDs as outputs
	    //Set the Arduino Header pins D0 through D3 to be digital inputs with internal pull-up resistors enabled.
	    GPIO_Output1.Pin = GPIO_PIN_13 | GPIO_PIN_5; //LD1 & LD2
	    GPIO_Output1.Mode = GPIO_MODE_OUTPUT_PP;
	    GPIO_Output1.Pull = GPIO_NOPULL;
	    GPIO_Output1.Speed = GPIO_SPEED_LOW;
	    HAL_GPIO_Init(GPIOJ, &GPIO_Output1);

	    GPIO_Output2.Pin = GPIO_PIN_12;  //LD3
	    GPIO_Output2.Mode = GPIO_MODE_OUTPUT_PP;
	    GPIO_Output2.Pull = GPIO_NOPULL;
		GPIO_Output2.Speed = GPIO_SPEED_LOW;
		HAL_GPIO_Init(GPIOA, &GPIO_Output2);

		GPIO_Output3.Pin = GPIO_PIN_4; //LD4
		GPIO_Output3.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_Output3.Pull = GPIO_NOPULL;
		GPIO_Output3.Speed = GPIO_SPEED_LOW;
		HAL_GPIO_Init(GPIOD, &GPIO_Output3);


	    // Switches as inputs with pull-ups
	    GPIO_Input1.Pin = GPIO_PIN_7 | GPIO_PIN_6; //D0 & D1
	    GPIO_Input1.Mode = GPIO_MODE_INPUT;
	    GPIO_Input1.Pull = GPIO_PULLUP;
	    GPIO_Input1.Speed = GPIO_SPEED_LOW;
	    HAL_GPIO_Init(GPIOC, &GPIO_Input1);

	    GPIO_Input2.Pin = GPIO_PIN_1;  //D2
	    GPIO_Input2.Mode = GPIO_MODE_INPUT;
		GPIO_Input2.Pull = GPIO_PULLUP;
		HAL_GPIO_Init(GPIOJ, &GPIO_Input2);

		GPIO_Input3.Pin = GPIO_PIN_6; //D3
		GPIO_Input3.Mode = GPIO_MODE_INPUT;
		GPIO_Input3.Pull = GPIO_PULLUP;
		HAL_GPIO_Init(GPIOF, &GPIO_Input3);
}
