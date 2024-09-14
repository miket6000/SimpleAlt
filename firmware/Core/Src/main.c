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
#include "bmp280.h"
#include "w25qxx.h"
#include "command.h"
#include "button.h"
#include "filesystem.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define STATE_IDLE 0
#define STATE_RECORDING 1

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern USBD_HandleTypeDef hUsbDeviceFS;
extern uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
extern uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];
uint8_t rx_buffer[APP_RX_DATA_SIZE];
uint16_t tx_buffer_index = 0;
uint16_t rx_buffer_index = 0;
uint32_t uid = 0;

const int8_t idle_sequence[] = {1, PAUSE, -2};
const int8_t usb_sequence[] = {0, -1};
const int8_t recording_sequence[] = {1, 2, -2}; 
const int8_t button_sequence[] = {1, 2, 3, PAUSE, -1};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
uint8_t Button(void);
void USBD_CDC_RxHandler(uint8_t *rxBuffer, uint32_t len);
void USB_Connect(void);
void USB_Disconnect(void);
void print(char *tx_buffer, uint16_t len);
void led_blink(void);
//measure_battery_voltage();
uint8_t read_button(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void print(char *tx_buffer, uint16_t len) {
  // if we are going to overflow the buffer, drop the message entirely.
  if (tx_buffer_index + len < sizeof(UserTxBufferFS)) {
    memcpy(&UserTxBufferFS[tx_buffer_index], tx_buffer, len);
    tx_buffer_index += len;
  }
}

void print_pressure() {
  char buffer[8];
  itoa(bmp_get_pressure(), buffer, 10);
  print(buffer, strlen(buffer));
}

void print_altitude() {
  char buffer[8];
  itoa(bmp_get_altitude(), buffer, 10);
  print(buffer, strlen(buffer));
}

void print_temperature() {
  char buffer[8];
  itoa(bmp_get_temperature(), buffer, 10);
  print(buffer, strlen(buffer));
}

void print_voltage() {
  char buffer[8];
  itoa(power_get_battery_voltage(), buffer, 10);
  print(buffer, strlen(buffer));
}

// W address bytes
void write_flash() {
  char* param;
  uint32_t address = 0;
  uint8_t len = 0;
  uint8_t flash_buffer[64];

  param = cmd_get_param();
  if (param != NULL) {
    address = atoi(param);
  }

  param = cmd_get_param();
  // WARNING NO CHECKS!!
  while (param != NULL) {
    flash_buffer[len++] = atoi(param);
    param = cmd_get_param();
  }

  if (len > 0) {
    fs_raw_write(address, flash_buffer, len);
  } 
}

// R address bytes
void read_flash() {
  uint32_t address;
  uint8_t len;
  uint8_t flash_buffer[64];

  char str_buf[3] = {0};
  
  address = atoi(cmd_get_param());
  len = atoi(cmd_get_param());

  if (len > 0) {
    fs_raw_read(address, flash_buffer, len);
    for (int i = 0; i < len; i++) {
      if (flash_buffer[i] < 0x10) {
        print("0",1);
      }
      print(itoa(flash_buffer[i], str_buf, 16), strlen(str_buf));
    }
  }
}

// r address bytes
void read_flash_binary() {
  uint32_t address;
  uint8_t len;
  uint8_t flash_buffer[64];
  
  address = atoi(cmd_get_param());
  len = atoi(cmd_get_param());

  if (len > 0 && len <= sizeof(flash_buffer)) {
    fs_raw_read(address, flash_buffer, len);
    print((char *)flash_buffer, len);
  }
}

void erase_flash() {
  fs_erase();
  print("OK\n", 3);
}

void set_config() {
  char *label = cmd_get_param();
  uint32_t value = atoi(cmd_get_param());
  fs_save_config(label[0], &value);
  print("OK\n", 3); 
}

void get_config() {
  char *label = cmd_get_param();
  uint32_t value = 0xFFFFFFFF;
  fs_read_config(label[0], &value);
  char str_buf[10] = {0};
  print(itoa(value, str_buf, 10), strlen(str_buf));
}

void get_uid() {
  char str_buf[10] = {0};
  print(itoa(uid, str_buf, 16), strlen(str_buf));
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  uint32_t tick = 0;
  uint32_t last_tick = 0;
  
  int32_t altitude = 0;
  int32_t ground_altitude = 0;
  int32_t altitude_above_ground = 0;
  uint32_t max_altitude = 0;
  
  int16_t temperature = 0;
  uint32_t pressure = 101325;
  uint16_t voltage = 0;

  uint32_t sample_rate_altitude = SECONDS_TO_TICKS(0.05);
  uint32_t sample_rate_temperature = SECONDS_TO_TICKS(1);
  uint32_t sample_rate_pressure = SECONDS_TO_TICKS(1);
  uint32_t sample_rate_voltage = SECONDS_TO_TICKS(1);

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  // Set tick to be 10ms
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/100);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC_Init();
  MX_SPI1_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  bmp_init(BMP_CS_GPIO_Port, BMP_CS_Pin);
  fs_init(&hspi1, FLASH_CS_GPIO_Port, FLASH_CS_Pin);
  uid = fs_get_uid();
  
  fs_read_config('P', &sample_rate_pressure);
  fs_read_config('T', &sample_rate_temperature);
  fs_read_config('A', &sample_rate_altitude);
  fs_read_config('V', &sample_rate_voltage);
  fs_read_config('M', &max_altitude);

  if (max_altitude > 0) {
    led_add_number_sequence(max_altitude);
  }

  led_add_sequence(idle_sequence);

  // Calibrate The ADC On Power-Up For Better Accuracy
  HAL_ADCEx_Calibration_Start(&hadc);
  
  cmd_add("P", print_pressure); 
  cmd_add("T", print_temperature); 
  cmd_add("A", print_altitude); 
  cmd_add("V", print_voltage);
  cmd_add("ERASE", erase_flash);
  cmd_add("I", cmd_set_interactive);
  cmd_add("i", cmd_unset_interactive);
  cmd_add("R", read_flash);
  cmd_add("r", read_flash_binary);
  cmd_add("SET", set_config);
  cmd_add("GET", get_config);
  cmd_add("UID", get_uid);
  cmd_set_print_function(print);

  //1 second delay to give the user time to release the power on button. 
  //This prevents us detecting the release as a seperate event later on.
  HAL_Delay(SECONDS_TO_TICKS(1));

  /* USER CODE END 2 */

  uint8_t state = 0; // 0 = idle, 1 = recording 
  uint8_t last_state = 0;
  ButtonState button_state = BUTTON_IDLE;

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    tick = HAL_GetTick();
    if ((tick - last_tick) >= 1) {
      last_tick = tick;

      /* Non-reentrant timer based function calls */
      led_blink();
      button_state = button_get_state();
      power_tick();

      /* check button presses */
      switch (button_state) {
        case BUTTON_DOWN:
          led_reset_sequence();
          led_add_sequence(button_sequence);
        case BUTTON_HELD: // do nothing
          break; 
        case BUTTON_RELEASE_0: // handled in state machine code
          power_idle_reset();
          break;
        case BUTTON_RELEASE_1: // handled in state machine code
          power_idle_reset();
          break;
        case BUTTON_RELEASE_2:
          power_set_mode(SLEEP);
          break;
        case BUTTON_RELEASE_3:
          power_set_mode(SHUTDOWN);
          break;
        case BUTTON_IDLE: // do nothing
          break;
      }

      /* Measurements */
      voltage = power_get_battery_voltage();
      temperature = bmp_get_temperature();
      pressure = bmp_get_pressure();
      altitude = bmp_get_altitude();    
      
      altitude_above_ground = altitude - ground_altitude;
      if (altitude_above_ground > 0) {
        if (altitude_above_ground > max_altitude) {
          max_altitude = altitude_above_ground;
        }
      }

      /* State specific behaviour and transitions */
      switch (state) {
        case STATE_IDLE:
          if (last_state != STATE_IDLE) {
            if (last_state == STATE_RECORDING) { 
              led_reset_sequence();
              // minimum height increase to eliminate accidental press/cancel
              if (ALTITUDE_IN_METERS(max_altitude) > 1) {
                uint32_t altitude_in_meters = ALTITUDE_IN_METERS(max_altitude);
                led_add_number_sequence(altitude_in_meters);
                fs_save_config('M', &altitude_in_meters);
              } else {
                led_add_sequence(idle_sequence);
              }
              fs_flush(); // write the log end address to the index table.
            } else { // not sure how we got here, but lets do something sensible
              led_reset_sequence();
              led_add_sequence(idle_sequence);
            }
          }
          
          last_state = state;
          
          // ignore short press, proper press change to recording
          if (button_state == BUTTON_RELEASE_0) {
            led_reset_sequence();
            led_add_sequence(idle_sequence);
          } else if (button_state == BUTTON_RELEASE_1) {
            state = STATE_RECORDING;
          }
          
          break;
        case STATE_RECORDING: // recording
          if (last_state != STATE_RECORDING) { // on entry
            led_reset_sequence();
            led_add_sequence(recording_sequence);
            ground_altitude = altitude;
            max_altitude = 0;
          }

          last_state = state;

          // save the data
          if (tick % sample_rate_altitude == 0) {
            fs_save('A', &altitude,     sizeof(altitude));
          }
          
          if (tick % sample_rate_pressure == 0) {
            fs_save('P', &pressure,     sizeof(pressure));
          }

          if (tick % sample_rate_temperature == 0) {
            fs_save('T', &temperature,  sizeof(temperature));
          }
          
          if (tick % sample_rate_voltage == 0) {
            fs_save('V', &voltage,      sizeof(voltage));
          }

          // ignore short press, proper press change to idle
          if (button_state == BUTTON_RELEASE_0) {
            led_reset_sequence();
            led_add_sequence(recording_sequence);
          } else if (button_state == BUTTON_RELEASE_1) {
            state = STATE_IDLE;
          }
          
          break;
      } // switch(state)

    } // Tick > refresh time

    /* Deal with data recieved via USB */
    if (rx_buffer_index > 0) {
      power_set_mode(AWAKE); //Dirty hack
      power_idle_reset(); //don't go to sleep if actively in use
      cmd_read_input((char *)rx_buffer, rx_buffer_index);
      rx_buffer_index = 0;
    }
  
    /* Transmit any data in the output buffer */
    if (tx_buffer_index > 0) {
      if (CDC_Transmit_FS(UserTxBufferFS, tx_buffer_index) == USBD_OK) {
        tx_buffer_index = 0;
      }
    }
   
    // Pause execution until woken by an interrupt.
    // The systick will do this for us every 10 ms if we're in normal mode.
    power_management();
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
  //DANGER - does not check for rx_buffer over run.
  memcpy(&rx_buffer[rx_buffer_index], rxBuffer, len);
  rx_buffer_index += len;
}

void USB_Connect(void) {
  // Doing anything at all in this function tends to result in the application faulting  
}

void USB_Disconnect(void) {
  // Doing anything at all in this function tends to result in the application faulting
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
