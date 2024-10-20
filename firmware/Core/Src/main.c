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
#include "commands.h"
#include "task.h"
#include "button.h"
#include "filesystem.h"
#include "record.h"
#include <stdbool.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LAUNCH_THRESHOLD        100
#define NOSE_OVER_TIMEOUT       SECONDS_TO_TICKS(0.5)
#define LANDING_NOISE_BAND      100
#define LANDING_TIMEOUT         SECONDS_TO_TICKS(5)
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

typedef enum {STATE_IDLE, STATE_RECORDING} State;
typedef enum {LAUNCH, APOGEE, LANDED} Status;

typedef struct {
  uint8_t status;
  int32_t ground_level;
  int32_t altitude;
  int32_t max_altitude;
  uint32_t max_altitude_time;
  int32_t descent_altitude;
  uint32_t descent_altitude_time;
} FlightStatus;

typedef struct { 
  State state;
  State last_state;
  FlightStatus *flight_status;
} Context;

// led flash sequences
const int8_t idle_sequence[] = {1, PAUSE, -2};
const int8_t recording_sequence[] = {2, -1}; 
const int8_t button_sequence[] = {1, 2, 3, PAUSE, -1};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void USBD_CDC_RxHandler(uint8_t *rxBuffer, uint32_t len);
void USB_Connect(void);
void USB_Disconnect(void);
void set_clock_divider(uint8_t divider);

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

void load_settings() {  //load settings
  uint8_t i = 0;
  Setting **settingList = get_settings();
  while (settingList[i] != NULL) {
    fs_read_config(settingList[i]->label, &settingList[i]->value);
    i++;
  }
  // configure power off timeout
  power_set_timeout(setting('o')->value);
}

void task_record_value(void *param) {
  RecordType *record = (RecordType *)param;
  if (*(record->state) == STATE_RECORDING) {
    fs_save(record->label, &record->value, record->size);
  }
}

/*
void task_update_altitude_buffer(void *param) {
  FlightStatus *fs = (FlightStatus *)param;
  ringbuffer_add(&fs->altitudes, record('A')->value);
}
*/

void update_flight_status(FlightStatus *fs) {
  // Altitude & status calculations
  uint32_t time = HAL_GetTick();
  //int32_t altitude_above_ground = fs->altitude - fs->ground_level;
  //if (altitude_above_ground > 0) {
    if (!(fs->status & (1 << LAUNCH)) && fs->altitude > fs->ground_level + LAUNCH_THRESHOLD) {
      fs->status |= (1 << LAUNCH);
    }
    if ((fs->status & (1 << LAUNCH)) && fs->altitude > fs->max_altitude) {
      fs->max_altitude = fs->altitude;
      fs->max_altitude_time = time;
    }
    if ((fs->status & (1 << LAUNCH)) && (time - fs->max_altitude_time) > NOSE_OVER_TIMEOUT) {
      fs->status |= (1 << APOGEE);
    }
    if((fs->status & (1 << APOGEE)) && fs->altitude < fs->descent_altitude) {
      fs->descent_altitude = fs->altitude;
      fs->descent_altitude_time = time;
    }
    if ((fs->status & (1 << APOGEE)) && (time - fs->descent_altitude_time) > LANDING_TIMEOUT) {
      fs->status |= (1 << LANDED);
    }

/*
}
  if ((fs->status & (1 << APOGEE)) && abs(fs->altitude - ringbuffer_read(&fs->altitudes, -LANDING_TIMEOUT)) < LANDING_NOISE_BAND) {
    fs->status |= (1 << LANDED);
  }
  */
}

void task_every_tick(void *param) {
  Context *context = (Context *)param;
  FlightStatus *fs = context->flight_status;
  static ButtonState button_state = BUTTON_IDLE;
 
  /* Non-reentrant timer based function calls */
  led_blink();
  power_tick();
  button_state = button_get_state();

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

  // Measurements
  record('V')->value = power_get_battery_voltage();
  record('T')->value = bmp_get_temperature();
  record('P')->value = bmp_get_pressure();
  fs->altitude = bmp_get_altitude();
  record('A')->value = fs->altitude;    
  record('S')->value = fs->status;

  // State specific behaviour and transitions
  switch (context->state) {
    case STATE_IDLE:
      if (context->last_state != STATE_IDLE) {
        led_reset_sequence();
        if (context->last_state == STATE_RECORDING) { 
          // minimum height increase to eliminate accidental press/cancel
          if (ALTITUDE_IN_METERS(fs->max_altitude - fs->ground_level) > 1) {
            uint32_t altitude_in_meters = ALTITUDE_IN_METERS(fs->max_altitude - fs->ground_level);
            led_add_number_sequence(altitude_in_meters);
            fs_save_config('m', &altitude_in_meters);
          } else {
            led_add_sequence(idle_sequence);
          }
          fs_flush(); // write the log end address to the index table.
        } else { 
          // not sure how we got here, but lets do something sensible
          led_add_sequence(idle_sequence);
        }
      }
      
      context->last_state = context->state;
      
      // ignore short press, proper press change to recording
      if (button_state == BUTTON_RELEASE_0) {
        led_reset_sequence();
        led_add_sequence(idle_sequence);
      } else if (button_state == BUTTON_RELEASE_1) {
        context->state = STATE_RECORDING;
      }
      
      break;
    case STATE_RECORDING:
      if (context->last_state != STATE_RECORDING) { 
        // on entry
        led_reset_sequence();
        led_add_sequence(recording_sequence);
        fs->ground_level = fs->altitude;
        fs->max_altitude = fs->ground_level;;
        fs->descent_altitude = 0x7FFFFFFF; //MAX_INT
        fs->status = 0;
      }

      context->last_state = context->state;
      update_flight_status(fs); 
 
      // ignore short press, proper press change to idle
      if (button_state == BUTTON_RELEASE_0) {
        led_reset_sequence();
        led_add_sequence(recording_sequence);
      } else if (button_state == BUTTON_RELEASE_1) {
        context->state = STATE_IDLE;
      }
      
      break;
  } // switch(state)
}

void init_task(Context *context) {
  uint8_t i = 0;
  RecordType **recordList = get_record_types();
  
  // variable recordings
  while(recordList[i] != NULL) {
    if (recordList[i]->setting->value > 0) {
      recordList[i]->state = &context->state;
      Task t;
      t.callback = task_record_value;
      t.param = recordList[i];
      t.delay = recordList[i]->setting->value;
      add_task(t);
    }
    i++;
  }

  Task t;

  // every tick
  t.callback = task_every_tick;
  t.param = context;
  t.delay = 1;
  add_task(t);
/*
  // landing timeout
  t.callback = task_update_altitude_buffer;
  t.param = context->flight_status;
  t.delay = SECONDS_TO_TICKS(1);
  add_task(t);
*/
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {

  /* USER CODE BEGIN 1 */
  FlightStatus flight_status;
  //ringbuffer_init(&flight_status.altitudes);
  
  Context context;
  context.state = STATE_IDLE;
  context.last_state = STATE_IDLE;
  context.flight_status = &flight_status;

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
  set_clock_divider(RCC_SYSCLK_DIV4); 
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/100);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC_Init();
  MX_SPI1_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  // Initalize peripherals, load settings and initialize the task system
  bmp_init(BMP_CS_GPIO_Port, BMP_CS_Pin);
  fs_init(&hspi1, FLASH_CS_GPIO_Port, FLASH_CS_Pin);
  uid = HAL_GetUIDw0() ^ HAL_GetUIDw1() ^ HAL_GetUIDw2();
  load_settings();

  init_task(&context);

  // setup previous altitude flash
  uint32_t max_altitude = setting('m')->value;
  if (max_altitude > 0) {
    led_add_number_sequence(max_altitude);
  }

  led_add_sequence(idle_sequence);

  // Calibrate The ADC On Power-Up For Better Accuracy
  HAL_ADCEx_Calibration_Start(&hadc);
  
  // init command line interpreter
  cmd_add("P", print_uint32, &record('P')->value); 
  cmd_add("T", print_int16, &record('T')->value); 
  cmd_add("A", print_int32, &record('A')->value); 
  cmd_add("V", print_uint16, &record('V')->value);
  cmd_add("ERASE", erase_flash, NULL);
  cmd_add("RESET", factory_reset, NULL);
  cmd_add("I", cmd_set_interactive, NULL);
  cmd_add("i", cmd_unset_interactive, NULL);
  cmd_add("R", read_flash, NULL);
  cmd_add("r", read_flash_binary, NULL);
  cmd_add("W", write_flash, NULL);
  cmd_add("w", write_flash, NULL);
  cmd_add("SET", set_config, NULL);
  cmd_add("GET", get_config, NULL);
  cmd_add("UID", print_uint32, &uid);
  cmd_set_print_function(print);

  // Wait for the button to be released (assumption is that it was pressed to wake us up)
  while (button_read() == BUTTON_DOWN);

  /* Infinite loop */
  /* USER CODE END 2 */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    // Time dependant task  
    execute_task();

    /* Deal with data recieved via USB */
    if (rx_buffer_index > 0) {
      led(ON);
      cmd_read_input((char *)rx_buffer, rx_buffer_index);
      rx_buffer_index = 0;
    }
  
    /* Transmit any data in the output buffer */
    if (tx_buffer_index > 0) {
      led(ON);
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
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
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

void set_clock_divider(uint8_t divider) {
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
  RCC_ClkInitStruct.AHBCLKDivider = divider;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
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
  power_set_mode(AWAKE);
  set_clock_divider(RCC_SYSCLK_DIV1);
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/100);
}

void USB_Disconnect(void) {
  power_set_mode(SNOOZE);
  set_clock_divider(RCC_SYSCLK_DIV4);
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/100);
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
