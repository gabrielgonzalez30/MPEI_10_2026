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
#define NUM_DISPLAYS 5
#define DEBOUNCE_TIME 50
#define SCROLL_SPEED 150  // Velocidad de desplazamiento

// Definición de pines para segmentos en PA0 - PA12
#define SEG_A   (1 << 0)   // PA0
#define SEG_B   (1 << 1)   // PA1
#define SEG_C   (1 << 2)   // PA2
#define SEG_D   (1 << 3)   // PA3
#define SEG_E   (1 << 4)   // PA4
#define SEG_F   (1 << 5)   // PA5
#define SEG_G   (1 << 6)   // PA6
#define SEG_H   (1 << 7)   // PA7
#define SEG_I   (1 << 8)   // PA8
#define SEG_J   (1 << 9)   // PA9
#define SEG_K   (1 << 10)  // PA10
#define SEG_L   (1 << 11)  // PA11
#define SEG_M   (1 << 12)  // PA12

// Definición de pines para transistores (PB3 a PB7)
#define TRANS1  (1 << 3)   // PB3 - Display 1
#define TRANS2  (1 << 4)   // PB4 - Display 2
#define TRANS3  (1 << 5)   // PB5 - Display 3
#define TRANS4  (1 << 6)   // PB6 - Display 4
#define TRANS5  (1 << 7)   // PB7 - Display 5

#define ALL_TRANS (TRANS1 | TRANS2 | TRANS3 | TRANS4 | TRANS5)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

		int letras[28];
		int displayEncendido=1;
		char nombre1[] = "GABRIEL DAVID GONZALEZ GARCIA";
		char nombre2[] = "LEIDY TATIANA ARDILA LOPEZ";
		char nombre3[] = "IVAN DARIO MELO LAGOS";
		char *nombres[3] = {nombre1, nombre2, nombre3};
		int nombreActual = 0;
		int longitud = 0;
		int token = 0;
		int displayActual = 0;
		int contadorScroll = 0;
		int botonEstadoAnterior = 1;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */
	void definirLetras(void);
	int getLetra(char letra);
	int secuenciarTransitores(void);
	void cambiarNombre(void);
	int leerBoton(void);
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

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  definirLetras();
    longitud = strlen(nombres[nombreActual]);
    displayEncendido = 1;
    botonEstadoAnterior = HAL_GPIO_ReadPin(Pulsador_GPIO_Port, Pulsador_Pin); // Pulsador en PC13
    token = 0;
    contadorScroll = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
		  while (1)
		  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
			  // Leer pulsador
			      if(leerBoton()) {
			        cambiarNombre();
			      }

			      // Control de desplazamiento (scroll)
			      if(contadorScroll < SCROLL_SPEED){
			        contadorScroll++;
			      } else {
			        contadorScroll = 0;
			        token++;
			        // Desplazarse hasta el final del nombre + número de displays
			        if(token > longitud + NUM_DISPLAYS) {
			          token = 0;
			        }
			      }

			      // Mostrar en los displays (de izquierda a derecha)
			      int displayIndex = displayActual;
			      if(displayIndex < NUM_DISPLAYS) {
			        int charIndex = token + displayIndex;
			        char displayChar;
			        int displayValue;

			        // Si el índice está dentro del texto, mostrar la letra
			        if(charIndex < longitud) {
			          displayChar = nombres[nombreActual][charIndex];
			          displayValue = ~getLetra(displayChar);
			        } else {
			          displayValue = 0;  // Apagar si no hay letra
			        }

			        // Escribir los primeros 13 segmentos en GPIOA (PA0-PA12)
			        GPIOA->ODR = (GPIOA->ODR & ~0x1FFF) | (displayValue & 0x1FFF);
			      }

			      HAL_Delay(2);

			      // Apagar TODOS los displays
			      GPIOB->ODR &= ~ALL_TRANS;

			      // Encender SOLO el display actual
			      GPIOB->ODR |= secuenciarTransitores();

			      displayActual++;
			      if(displayActual >= NUM_DISPLAYS) {
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
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin : Pulsador_Pin */
  GPIO_InitStruct.Pin = Pulsador_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Pulsador_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA2 PA3
                           PA4 PA5 PA6 PA7
                           PA8 PA9 PA10 PA11
                           PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12;
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
			  letras[0] = 119;    // A = 0000001110111
			  letras[1] = 127;    // B = 0000001111111
			  letras[2] = 57;     // C = 0000000111001
			  letras[3] = 94;     // D = 0000000111111
			  letras[4] = 121;    // E = 0000001111001
			  letras[5] = 113;    // F = 0000001110001
			  letras[6] = 125;    // G = 0000001111101
			  letras[7] = 118;    // H = 0000001110110
			  letras[8] = 2313;    // I = 0000101001001
			  letras[9] = 14;     // J = 0000000001110
			  letras[10] = 1648;  // K = 0011001110000
			  letras[11] = 56;    // L = 0000000111000
			  letras[12] = 694;   // M = 0001010110110
			  letras[13] = 1206;  // N = 0010010110110
			  letras[14] = 92;    // O = 0000001011100
			  letras[15] = 115;   // P = 0000001110011
			  letras[16] = 1123;  // Q = 0010001100011
			  letras[17] = 1139;  // R = 0010001110011
			  letras[18] = 109;   // S = 0000001101101
			  letras[19] = 2305;  // T = 0100100000001
			  letras[20] = 62;    // U = 0000000111110
			  letras[21] = 640;   // V = 0001010000000
			  letras[22] = 5174;  // W = 1010000110110
			  letras[23] = 5760;  // X = 1011010000000
			  letras[24] = 2688;  // Y = 0101010000000
			  letras[25] = 4617;  // Z = 1001000001001
		}

		int getLetra(char letra){
		  if(letra == ' '){
		    return 0;
		  } else if(letra >= 'A' && letra <= 'Z'){
		    return letras[(int)(letra - 'A')];
		  } else {
		    return 0;
		  }
		}


		int secuenciarTransitores(){
		  int valorActual = displayEncendido;
		  int resultado = 0;

		  // Mapeo de bits a pines PB3-PB7
		  if(valorActual & 1) resultado |= TRANS1;
		  if(valorActual & 2) resultado |= TRANS2;
		  if(valorActual & 4) resultado |= TRANS3;
		  if(valorActual & 8) resultado |= TRANS4;
		  if(valorActual & 16) resultado |= TRANS5;

		  displayEncendido = displayEncendido << 1;
		  if(displayEncendido > 16) {
		    displayEncendido = 1;
		  }

		  return resultado;
		}

		int leerBoton() {
		  int estadoActual = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13); // Pulsador en PC13

		  // Detectar flanco de bajada (botón presionado con pull-up)
		  if(estadoActual == 0 && botonEstadoAnterior == 1) {
		    botonEstadoAnterior = estadoActual;
		    HAL_Delay(DEBOUNCE_TIME);
		    return 1;
		  }

		  botonEstadoAnterior = estadoActual;
		  return 0;
		}

		void cambiarNombre() {
		  nombreActual++;
		  if(nombreActual >= 3) {
		    nombreActual = 0;
		  }
		  token = 0;           // Reiniciar token al cambiar de nombre
		  contadorScroll = 0;  // Reiniciar contador de scroll
		  longitud = strlen(nombres[nombreActual]);
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
