/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led.h"
#include "power.h"
#include "usbd_cdc_if.h"
#include "spi_wrapper.h"
#include "bmp280.h"

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
uint8_t sleep = SLEEP;
extern uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
extern uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];
extern USBD_HandleTypeDef hUsbDeviceFS;
int8_t idle_sequence[] = {1, PAUSE, -2};
int8_t usb_sequence[] = {2, SHORT_PAUSE, -2};
int8_t voltage_sequence[] = {1, 2, 3, PAUSE, -1};
int8_t altitude_sequence[] = {3, 5, 7, 0, PAUSE, -1};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
uint8_t Button(void);
void USBD_CDC_RxHandler(uint8_t *rxBuffer, uint32_t len);
void USB_Connect(void);
void USB_Disconnect(void);
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
  uint8_t txBuffer[] = "V:     , T:     , P:       , A:      \n\r";
  int32_t temperature = 0;
  uint32_t pressure = 0;
  int32_t altitude = 0;
  int16_t voltage = 0;
  int16_t velocity = 0;


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
  MX_DMA_Init();
  MX_ADC_Init();
  MX_SPI1_Init();
  MX_USB_DEVICE_Init();
  MX_TIM16_Init();
  /* USER CODE BEGIN 2 */

  /* Setup GPIO to sensible "Power On" states */
  HAL_GPIO_WritePin(SHUTDOWN_GPIO_Port, SHUTDOWN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(CHARGE_EN_GPIO_Port, CHARGE_EN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(BMP_CS_GPIO_Port, BMP_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(nSENSE_EN_GPIO_Port, nSENSE_EN_Pin, GPIO_PIN_SET);
  
  bmp_init(BMP_CS_GPIO_Port, BMP_CS_Pin);
  
  // Calibrate The ADC On Power-Up For Better Accuracy
  HAL_ADCEx_Calibration_Start(&hadc);

  /* Enable timer interrupt for LED and button timing */
  HAL_TIM_Base_Start_IT(&htim16);

  // flash out the voltage
  add_led_sequence(voltage_sequence);
  // and the last altitude
  add_led_sequence(altitude_sequence);
  // and got to idle blink
  add_led_sequence(idle_sequence);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
/*  
    switch (state) {
      case IDLE:
        break;
      case READY:
        break;
      case INFLIGHT:
        break;
      case LANDED:
        break;
      case STANDBY:
        break;
      case POWERDOWN:
        break;
      case CONNECTED:
        break;
      default:
        break;
    }
*/

    voltage = get_battery_voltage();
    temperature = bmp_get_temperature();
    pressure = bmp_get_pressure();
    altitude = bmp_get_altitude();


     /* If USB is connected do transmit loop, otherwise sleep */
    if(hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) {
      itoa(voltage, (char *)&txBuffer[3], 10); 
      itoa(temperature, (char *)&txBuffer[12], 10); 
      itoa(pressure, (char *)&txBuffer[21], 10);      
      itoa(altitude, (char *)&txBuffer[32], 10);
      CDC_Transmit_FS(txBuffer, sizeof(txBuffer));
      HAL_Delay(1000);
    } else {
      // normally pause execution until woken by an interrupt
      power_down(sleep);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI14|RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void USBD_CDC_RxHandler(uint8_t *rxBuffer, uint32_t len) {
  // deal with received data here...
}

void USB_Connect(void) {
  //Led_Sequence(usb_sequence);
}

void USB_Disconnect(void) {
  //Led_Sequence(idle_sequence);
}

uint8_t get_button_state(void) {
  return HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin);
}

void read_button() {
  static uint16_t button_counter = 0;

  if (get_button_state()) {
    button_counter++;
  } else {
    if (button_counter > 100) {
      sleep = DEEPSLEEP;  // 2 seconds = deep sleep, wake up by pressing button again
    }
    if (button_counter > 250) {
      sleep = SHUTDOWN;  // 5 seconds = shutdown, need USB to wake, lowest power
    }
    button_counter = 0;
  }
}  

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  // Check which version of the timer triggered this callback and toggle LED
  if (htim == &htim16) {
    blink();

    measure_battery_voltage();
    
    read_button();
  }
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

#ifdef  USE_FULL_ASSERT
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
