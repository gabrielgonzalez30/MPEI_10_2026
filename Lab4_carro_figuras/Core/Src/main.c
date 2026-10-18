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
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PI                3.14159265f
#define llanta_cm         6.5f      /* diametro de la llanta                         */
#define SIDE_CM           50.0f     /* longitud de cada lado                         */
#define TRACK_CM          14.0f     /* distancia entre centros de las dos llantas    */
#define ENCO_CONT         22.7f
#define COUNTS_PER_CM     (ENCO_CONT / (PI * llanta_cm))

#define M1_TRIM           1.00f
#define M2_TRIM           1.00f

#define BASE_SPEED        60.0f     /* % de PWM al avanzar                           */
#define TURN_SPEED        75.0f     /* % de PWM al girar                             */
#define MIN_PWM           35.0f
#define KP                6.0f      /* ganancia P: % de PWM por cuenta de diferencia */
#define MOVE_TIMEOUT_MS   5000U    /* seguridad si un encoder falla                 */
#define SETTLE_MS         300U      /* pausa para que el robot se detenga            */

#define TURN_CAL          3.103f      /* factor de calibracion del giro (ajustar)      */
#define PAUSE_FIGURE_MS   4000U     /* espera entre figuras distintas                */
#define PAUSE_REPEAT_MS   1000U     /* espera entre repeticiones de la misma figura  */

#define ENC_DEBOUNCE_US   1000U

/* Nivel del pin de direccion que hace girar cada motor "hacia adelante".
   Si una rueda gira al reves, cambia 1 por 0 en ese motor. */
#define M1_FWD_LEVEL      1
#define M2_FWD_LEVEL      1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t duty_pwm = 0;
uint8_t procentaje_pwm = 0;
volatile int32_t enc1 = 0;   /* pulsos del encoder 1 (PB7) */
volatile int32_t enc2 = 0;   /* pulsos del encoder 2 (PB6) */

static volatile uint32_t last_us1 = 0;
static volatile uint32_t last_us2 = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Interrupcion de los encoders (flanco de subida) con antirrebote por tiempo */
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

static float clampf(float v, float lo, float hi)
{
  return (v < lo) ? lo : (v > hi) ? hi : v;
}

/* dir: +1 adelante, -1 atras. pct: 0..100 */
static void motor1_set(int8_t dir, float pct)
{
  uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim2);
  GPIO_PinState lvl = (dir > 0) ? (GPIO_PinState)M1_FWD_LEVEL : (GPIO_PinState)!M1_FWD_LEVEL;
  HAL_GPIO_WritePin(Motor_1_GPIO_Port, Motor_1_Pin, lvl);
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3,
                        (uint32_t)(clampf(pct, 0.0f, 100.0f) * (arr + 1) / 100.0f));
}

static void motor2_set(int8_t dir, float pct)
{
  uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim3);
  GPIO_PinState lvl = (dir > 0) ? (GPIO_PinState)M2_FWD_LEVEL : (GPIO_PinState)!M2_FWD_LEVEL;
  HAL_GPIO_WritePin(Motor_2_GPIO_Port, Motor_2_Pin, lvl);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4,
                        (uint32_t)(clampf(pct, 0.0f, 100.0f) * (arr + 1) / 100.0f));
}

/* Mueve ambos motores hasta que cada encoder llegue a 'target' pulsos.
   Control P: iguala los pulsos de ambas ruedas para ir recto / girar simetrico. */
static void drive(int32_t target, int8_t d1, int8_t d2, float speed)
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

    float err = (float)(e1 - e2);                 /* >0: la rueda 1 va adelantada */
    float s1 = clampf(speed * M1_TRIM - KP * err, 0.0f, 100.0f);
    float s2 = clampf(speed * M2_TRIM + KP * err, 0.0f, 100.0f);

    motor1_set(d1, done1 ? 0.0f : clampf(s1, MIN_PWM, 100.0f));
    motor2_set(d2, done2 ? 0.0f : clampf(s2, MIN_PWM, 100.0f));

    if ((HAL_GetTick() - t0) > MOVE_TIMEOUT_MS) break;   /* seguridad */
  }

  motor1_set(d1, 0.0f);
  motor2_set(d2, 0.0f);
  HAL_Delay(SETTLE_MS);
}

static void forward_cm(float cm)
{
  int32_t target = (int32_t)(cm * COUNTS_PER_CM + 0.5f);
  drive(target, +1, +1, BASE_SPEED);
}

/* Giro sobre su eje: una rueda adelante y la otra atras (grados > 0 = izquierda) */
static void turn_deg(float deg)
{
  float arc_cm = (TRACK_CM / 2.0f) * (deg * PI / 180.0f) * TURN_CAL;
  int32_t target = (int32_t)(arc_cm * COUNTS_PER_CM + 0.5f);
  drive(target, -1, +1, TURN_SPEED);
}

/* Dibuja un poligono regular de 'sides' lados */
static void polygon(uint8_t sides)
{
  for (uint8_t i = 0; i < sides; i++)
  {
    forward_cm(SIDE_CM);
    turn_deg(360.0f / sides);
  }
}

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
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

  __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_3 , 0);
  __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_4 , 0);

  HAL_Delay(2000);

  polygon(4);


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
