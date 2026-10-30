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
#include "adc.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include <stdio.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define Mo1_puenH_avan 1
#define Mo2_puenH_avan 1
#define MOVE_TIMEOUT_MS 8000U
#define Distancia 10.0f

#define M1_TRIM 1.0f
#define M2_TRIM 1.0f

#define Volt_ref 1200.0f
#define Histeresis 5.0f
#define ADC_muestras 8U

#define ADC_max_filtro 32U
#define Muestras_dist 15U
#define Espera_dist_ms 2U        /* el Sharp actualiza cada ~38 ms           */
#define Filtro_dist 0.5f         /* 0.1 = muy suave/lento, 1.0 = sin filtro  */

#define Duty_arranque 100.0f     /* % durante el impulso inicial             */
#define Tiempo_arranque_ms 150U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
char buf_lcd[24];
uint8_t duty = 80;
uint8_t ciclo_trab = 0;
uint16_t datos_temp = 0;
uint8_t datos_distan = 0;
uint16_t datos_alimentacion = 3300;
uint8_t motores_on = 1;
float dist_filtrada = 80.0f;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static uint16_t Lectura_ADC(uint32_t canal_leer)
{
	ADC_ChannelConfTypeDef config = {0};
	uint32_t suma = 0;

	config.Channel = canal_leer;
	config.Rank = ADC_REGULAR_RANK_1;
	config.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
	HAL_ADC_ConfigChannel(&hadc1, &config);

	for (uint32_t i = 0; i < ADC_muestras; i++)
	  {
	    HAL_ADC_Start(&hadc1);
	    HAL_ADC_PollForConversion(&hadc1, 10);
	    suma += HAL_ADC_GetValue(&hadc1);
	    HAL_ADC_Stop(&hadc1);
	  }
	  return (uint16_t)(suma / ADC_muestras);
}

static uint16_t Lectura_ADC_filtrada(uint32_t canal_leer, uint8_t muestras, uint32_t espera_ms)
{
	uint16_t m[ADC_max_filtro];
	uint8_t n = muestras;
	uint32_t suma = 0;

	if (n > ADC_max_filtro) n = ADC_max_filtro;
	if (n < 4) n = 4;

	for (uint8_t i = 0; i < n; i++)
	{
		m[i] = Lectura_ADC(canal_leer);
		HAL_Delay(espera_ms);
	}

	/* ordenar de menor a mayor (insercion) */
	for (uint8_t i = 1; i < n; i++)
	{
		uint16_t clave = m[i];
		int j = (int)i - 1;
		while (j >= 0 && m[j] > clave)
		{
			m[j + 1] = m[j];
			j--;
		}
		m[j + 1] = clave;
	}

	/* promedio de la mitad central */
	uint8_t ini = n / 4U;
	uint8_t fin = n - ini;
	for (uint8_t i = ini; i < fin; i++)
	{
		suma += m[i];
	}
	return (uint16_t)(suma / (uint32_t)(fin - ini));
}

static void Motores_avanzar(void)
{
  HAL_GPIO_WritePin(Motor_1_GPIO_Port, Motor_1_Pin, Mo1_puenH_avan);
  HAL_GPIO_WritePin(Motor_2_GPIO_Port, Motor_2_Pin, Mo2_puenH_avan);
    HAL_Delay(Tiempo_arranque_ms);
  __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_3, duty*M1_TRIM);
  __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_4, duty*M2_TRIM);
}

static void Motores_Detener(void)
{
  __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_3, 0);
  __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_4, 0);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
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
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
  Motores_avanzar();
  Lcd_Init();

 /* Lcd_Set_Cursor(1,1);
  sprintf(buf_lcd, "PWM: %u ",duty);
  Lcd_Send_String(buf_lcd);
  HAL_Delay(50)*/

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  /*Alimentación*/
	  uint16_t alimen_ref = Lectura_ADC(ADC_CHANNEL_VREFINT);
	  if (alimen_ref >0)
	  {
		  datos_alimentacion = (uint16_t)(Volt_ref*4095.0f/(float)alimen_ref);
	  }
	  /*Temperatura*/
	  uint16_t temp_ref = Lectura_ADC(ADC_CHANNEL_0);
	  float mv_temp = ((float)temp_ref * (float)datos_alimentacion) / 4095.0f;
	  datos_temp = (uint16_t)mv_temp;       /* mV = decimas de grado C */

	  /*Distancia*/
	  uint16_t distan_ref = Lectura_ADC_filtrada(ADC_CHANNEL_1,Muestras_dist,Espera_dist_ms);
	 	  float volt_dist = ((float)distan_ref * (float)datos_alimentacion) / 4095.0f / 1000.0f;
	 	  float dist_cm = 80.0f;
	 	  if (volt_dist > 0.4f)
	 	  {
	 		  dist_cm = 27.16f * powf(volt_dist, -1.15f);
	 	  }
	 	  if (dist_cm > 80.0f)
	 		  {
	 		  dist_cm = 80.0f;
	 		  }
	 	  if (dist_cm < 10.0f)
	 	  {
	 		  dist_cm = 10.0f;
	 	  }

	 	 dist_filtrada = (Filtro_dist * dist_cm) + ((1.0f - Filtro_dist) * dist_filtrada);
	 		  datos_distan = (uint8_t)dist_filtrada;

	 	 	  if (motores_on && (dist_filtrada <= Distancia))
	 	 	  {
	 	 		  Motores_Detener();
	 	 		  motores_on = 0;
	 	 	  }
	 	 	  else if (!motores_on && (dist_filtrada >= (Distancia + Histeresis)))
	 	 	  {
	 	 		  Motores_avanzar();
	 	 		  motores_on = 1;
	 	 	  }
		  Lcd_Set_Cursor(1,1);
		  sprintf(buf_lcd, "Temp %u.%u ", datos_temp/ 10U,datos_temp % 10U);
		  Lcd_Send_String(buf_lcd);
		  Lcd_Set_Cursor(2,1);
		  sprintf(buf_lcd, "V:%u.%uV D:%ucm  ", datos_alimentacion / 1000U, (datos_alimentacion % 1000U) / 10U, datos_distan);
		  Lcd_Send_String(buf_lcd);

		  HAL_Delay(50);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
