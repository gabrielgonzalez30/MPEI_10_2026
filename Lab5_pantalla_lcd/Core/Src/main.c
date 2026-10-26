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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define Pi 3.141592654f
#define diam_llanta_cm 6.5f
#define Cont_Enco 22.7f
#define Cont_Puls_cm (Cont_Enco/(Pi*diam_llanta_cm))
#define Tiemp_min_puls 1000u

#define distan_cm 50.0f
#define Mo1_puenH_avan 1
#define Mo2_puenH_avan 1
#define MOVE_TIMEOUT_MS   8000U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
char buf_lcd[18];
uint8_t duty = 50;
uint8_t ciclo_trab = 0;
uint32_t t_m1 = 0 ;
uint32_t t_m2 = 0;


volatile int32_t puls_enco1 = 0;
volatile int32_t puls_enco2 = 0;

static volatile uint32_t ultimo_puls1 = 0;
static volatile uint32_t ultimo_puls2 = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  uint32_t now = DWT->CYCCNT / (SystemCoreClock / 1000000U);   /* microsegundos */

  if (GPIO_Pin == Encoder_1_Pin)
  {
    if ((uint32_t)(now - last_us1) >= ENC_DEBOUNCE_US)
    {
      last_us1 = now;
      __atomic_fetch_add(&enc1, 1, __ATOMIC_RELAXED);
    }
  }
  else if (GPIO_Pin == Encoder_2_Pin)
  {
    if ((uint32_t)(now - last_us2) >= ENC_DEBOUNCE_US)
    {
      last_us2 = now;
      __atomic_fetch_add(&enc2, 1, __ATOMIC_RELAXED);
    }
  }
}

static void drive(int32_t target, int8_t d1, int8_t d2, float speed, float ramp)
{
  uint32_t t0 = HAL_GetTick();
  uint8_t done1 = 0, done2 = 0;

  enc1 = 0;
  enc2 = 0;

  while (!(done1 && done2))
  {
    int32_t e1 = enc1;
    int32_t e2 = enc2;
    if (e1 >= target) done1 = 1;
    if (e2 >= target) done2 = 1;
    float base = speed;
    if (ramp > 0.0f)
    {
      float rem = (float)target - 0.5f * (float)(e1 + e2);
      if (rem < ramp)
      {
        base = RAMP_END_PWM + (speed - RAMP_END_PWM) * clampf(rem / ramp, 0.0f, 1.0f);
      }
    }

    motor1_set(d1, done1 ? 0.0f : clampf(s1, MIN_PWM, 100.0f));
    motor2_set(d2, done2 ? 0.0f : clampf(s2, MIN_PWM, 100.0f));

  }
  motor1_set(d1, 0.0f);
  motor2_set(d2, 0.0f);
  HAL_Delay(SETTLE_MS);
}

static void forward_cm(float cm)
{
  int32_t target = (int32_t)(cm * COUNTS_PER_CM + 0.5f);
  drive(target, +1, +1, BASE_SPEED, 0.0f);
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
  MX_TIM1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, duty);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
   __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, duty);
  Lcd_Init();

  Lcd_Set_Cursor(1,1);
  sprintf(buf_lcd, "PWM: %u ",duty);
  Lcd_Send_String(buf_lcd);
  Lcd_Set_Cursor(2,1);
  sprintf(buf_lcd, "Porcentaje: %u%% ",ciclo_trab );
  Lcd_Send_String(buf_lcd);
  HAL_Delay(50);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

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
