/* USER CODE BEGIN Header */
		/**
		  ******************************************************************************
		  * @file           : main.c
		  * @brief          : Main program body
		  ******************************************************************************
		  * @attention
		  *
		  * Copyright (c) 2026 STMicroelectronics.
		  * All rights reserved.
		  *
		  * This software is licensed under terms that can be found in the LICENSE file
		  * in the root directory of this software component.
		  * If no LICENSE file comes with this software, it is provided AS-IS.
		  *
		  ******************************************************************************
		  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
	#include <string.h>
	#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
		int letras[28];
		int numeroDisplays=5;
		int displayEncendido=0;

		char nombre1[] = "GABRIEL DAVID GONZALEZ GARCIA";
		char nombre2[] = "LEIDY TATIANA ARDILA LOPEZ";
		char nombre3[] = "IVAN DARIO MELO LAGOS";
		int longitud=0;

		const uint16_t transistores[5] = {
				(1 << 3), // Display 1 -> PB3
				(1 << 4), // Display 2 -> PB4
				(1 << 5), // Display 3 -> PB5
				(1 << 6), // Display 4 -> PB6
				(1 << 7)  // Display 5 -> PB7
			};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */
		void definirLetras(void);
		int getLetra(char letra);
		int secuenciarTransitores(int display);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
		  int displayActual=0;
		  int numRepe=50;
		  int aux=0;
		  int token=0;

		  // Variables para alternar entre nombres y manejo del pulsador
		  char *nombreActual = nombre1;
		  int estadoPulsador = 0; // 0: nombre1, 1: nombre2
		  uint8_t estadoAnteriorBoton = GPIO_PIN_SET;
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
		  definirLetras();
		  nombreActual = nombre1;
		  longitud = strlen(nombreActual);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
		  while (1)
		  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		uint8_t estadoActualBoton = HAL_GPIO_ReadPin(Pulsador_GPIO_Port, Pulsador_Pin);
		if (estadoAnteriorBoton == GPIO_PIN_SET && estadoActualBoton == GPIO_PIN_RESET)
					{
						HAL_Delay(20); // Anti-rebote para evitar falsas pulsaciones
						estadoPulsador++;
						if (estadoPulsador > 2) {
							estadoPulsador = 0; // Al presionar por 3ª vez vuelve a 0 (Reinicia al Nombre 1)
						}

						// Asigna la dirección del nombre a mostrar
						switch (estadoPulsador)
						{
						case 0:
							nombreActual = nombre1;
							break;
						case 1:
							nombreActual = nombre2;
							break;
						case 2:
							nombreActual = nombre3;
							break;
						default:
							nombreActual = nombre1;
							break;
						}
						longitud = strlen(nombreActual); // Actualiza la longitud de la palabra
						token = 0;                       // Reinicia la marquesina desde la primera letra
						aux = 0;
					}
			  estadoAnteriorBoton = estadoActualBoton;

			  if(aux<numRepe){
				 aux++;
			  }else{
				  aux=0;
				  token++;
				  if(token>longitud + numeroDisplays){
					  token = 0;
				  }
			  }

			  uint16_t valorLetra = 0;
			  int posCaracter = token + displayActual;

			  if (posCaracter < longitud)
			  {
				  valorLetra = getLetra(nombreActual[posCaracter]);
			  }else{
				  valorLetra = 0;
			  }

			  GPIOA->ODR = (GPIOA -> ODR & ~0x1FFF) | (valorLetra & 0x1FFF);
			  GPIOC->ODR = (GPIOC->ODR & ~GPIO_PIN_14) | ((valorLetra & 0x2000) << 1);
			  HAL_Delay(1);

			 // GPIOA->ODR=getLetra('A');
			  GPIOB->ODR &=((1<<3) | (1<<4) | (1<<5) | (1<<6) | (1<<7));
			  GPIOB->ODR |= secuenciarTransitores(displayActual);


			  displayActual++;
					  if (displayActual >= numeroDisplays) {
						  displayActual = 0;
			 }
		  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin : Pulsador_Pin */
  GPIO_InitStruct.Pin = Pulsador_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Pulsador_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PC14 */
  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA2 PA3
                           PA4 PA5 PA6 PA7
                           PA8 PA9 PA10 PA11
                           PA12 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB3 PB4 PB5 PB6
                           PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
		void definirLetras(){
			letras[0]=8759;
			letras[1]=2703;
			letras[2]=57;
			letras[3]=2191;
			letras[4]=8761;
			letras[5]=8753;
			letras[6]=573;
			letras[7]=8758;
			letras[8]=2185;
			letras[9]=30;
			letras[10]=9520;
			letras[11]=56;
			letras[12]=374;
			letras[13]=1142;
			letras[14]=63;
			letras[15]=8755;
			letras[16]=4159;
			letras[17]=9779;
			letras[18]=8749;
			letras[19]=2177;
			letras[20]=62;
			letras[21]=4400;
			letras[22]=5174;
			letras[23]=5440;
			letras[24]=2368;
			letras[25]=4361;


		}

		int getLetra(char letra){
			if(letra==' ' || letra <'A' || letra >	'Z'){
				return 0;
			}else{
				return letras[(int)(letra- 'A')];
			}

		}

		int secuenciarTransitores(int display){
			if( display >= 0 && display < numeroDisplays)
			{
				return transistores[display];
			}
			return 0;
		}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
		  /* User can add his own implementation to report the HAL error return state */
		  __disable_irq();
		  while (1)
		  {
		  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
		  /* User can add his own implementation to report the file name and line number,
			 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
