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
#include <stdbool.h>
#include "led.h"
#include "power.h"
#include "usbd_cdc_if.h"
#include "spi_wrapper.h"
#include "bmp280.h"
#include "w25qxx.h"
#include "command.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define REFRESH_MS  (20 / 10)
#define ALT_RING_BUF_LEN  20
#define SAMPLE_DIFFERENCE  10
#define LAUNCH_SPEED_KMPH  50
#define LAUNCH_THRESHOLD (LAUNCH_SPEED_KMPH * REFRESH_MS * SAMPLE_DIFFERENCE / 36)

#define DATA_OFFSET 0x10000LLU

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t sleep = SNOOZE;
uint8_t refresh_time = REFRESH_MS;

extern USBD_HandleTypeDef hUsbDeviceFS;
extern uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
extern uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];
uint8_t rx_buffer[APP_RX_DATA_SIZE];
uint16_t tx_buffer_index = 0;
uint16_t rx_buffer_index = 0;


W25QXX_HandleTypeDef w25qxx; // Handler for all w25qxx operations! 
uint32_t next_free_index_slot = 0;
uint32_t next_free_address = 0;



const int8_t idle_sequence[] = {1, PAUSE, -2};
const int8_t usb_sequence[] = {0, -1};
const int8_t recording_sequence[] = {2, SHORT_PAUSE, -2}; 

//const int8_t voltage_sequence[] = {1, 2, 3, PAUSE, -1};
//const int8_t altitude_sequence[] = {3, 5, 7, 0, PAUSE, -1};


int16_t temperature = 0;
uint32_t pressure = 101325;
uint16_t voltage = 0;
int32_t altitude = 0;

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
  itoa(get_battery_voltage(), buffer, 10);
  print(buffer, strlen(buffer));
}


void print(char *tx_buffer, uint16_t len) {
  memcpy(&UserTxBufferFS[tx_buffer_index], tx_buffer, len);
  tx_buffer_index += len;
}

void read_register() {
  uint32_t address = 0;
  uint8_t data = 0;
  char str_buf[3] = {0};
  
  address = atoi(cmd_get_param());

  if (address > 0) {
    //data = w25q_read_register(address);
    print(itoa(data, str_buf, 16), strlen(str_buf));
  }
}

void write_flash() {
  char* param;
  uint32_t address = 0;
  uint8_t data[16];
  uint8_t len = 0;

  param = cmd_get_param();
  if (param != NULL) {
    address = atoi(param);
  }

  param = cmd_get_param();
  while (param != NULL) {
    data[len++] = atoi(param);
    param = cmd_get_param();
  }

  if (len > 0) {
    //w25q_write_enable();
    //HAL_Delay(5);
    //w25q_write(address, data, len);
    w25qxx_write(&w25qxx, address, data, len);
    print("OK", 3);
  } else {
    print("ERR", 4);
  }

}


/* Reads param2 bytes (max 16) from address param1 and sends to serial.
 * e.g. R 0 16 to read 16 bytes at address 0.
 * Possible optimisation is to remove the buffer and write straight into the USB buffer.
 * Would need a revamp of the "print" function and removal of the interactive mode.
 */
void read_flash() {
  uint32_t address;
  uint8_t len;
  uint8_t buffer[16];
  char str_buf[3] = {0};
  
  address = atoi(cmd_get_param());
  len = atoi(cmd_get_param());

  if (len > 0) {
    w25qxx_read(&w25qxx, address, buffer, len);
    for (int i = 0; i < len; i++) {
      if (buffer[i] < 0x10) {
        print("0",1);
      }
      print(itoa(buffer[i], str_buf, 16), strlen(str_buf));
    }
  }
}

void read_flash_binary() {
  uint32_t address;
  uint8_t len;
  uint8_t buffer[64];
  
  address = atoi(cmd_get_param());
  len = atoi(cmd_get_param());

  if (len > 0 && len <= sizeof(buffer)) {
    w25qxx_read(&w25qxx, address, buffer, len);
    print((char *)buffer, len);
  }
}

void erase_flash() {
  print("Please wait...\n", 15);
  w25qxx_chip_erase(&w25qxx);
  print("OK",2);
}

void open_flash() {
  int16_t i = 0;
  uint32_t address = 0;
  uint32_t last_address = DATA_OFFSET;
  w25qxx_read(&w25qxx, i, (uint8_t *)&address, 4);
  while (i < DATA_OFFSET && address < 0xFFFFFFFF) {
    i+=4;
    last_address = address;
    w25qxx_read(&w25qxx, i, (uint8_t *)&address, 4);
  }
  next_free_index_slot = i;
  next_free_address = last_address;
}

void close_flash() {
  w25qxx_write(&w25qxx, next_free_index_slot, (uint8_t *)&next_free_address, 4);
}

void save(char label, uint8_t *data, uint16_t len) {
  w25qxx_write(&w25qxx, next_free_address, (uint8_t *)&label, 1);
  w25qxx_write(&w25qxx, next_free_address+1, data, len);
  next_free_address += (len + 1);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  static uint32_t tick = 0;
  static uint32_t last_tick = 0;
  static int32_t max_altitude = 0;
  static int32_t ground_altitude = 0;
  static bool recording = false;
  int32_t altitude_above_ground = 0;

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/100);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC_Init();
  MX_SPI1_Init();
  MX_USB_DEVICE_Init();
  MX_TIM16_Init();
  /* USER CODE BEGIN 2 */

  bmp_init(BMP_CS_GPIO_Port, BMP_CS_Pin);
  w25qxx_init(&w25qxx, &hspi1, FLASH_CS_GPIO_Port, FLASH_CS_Pin); 

  // Calibrate The ADC On Power-Up For Better Accuracy
  HAL_ADCEx_Calibration_Start(&hadc);
  voltage = 3921;
  // flash out the voltage
  led_add_number_sequence(voltage);
  // and the last altitude
  //led_add_sequence(altitude_sequence);
  // and got to idle blink
  led_add_sequence(idle_sequence);
  //cmd_add("LED_ON", led_on);
  cmd_add("P", print_pressure); 
  cmd_add("T", print_temperature); 
  cmd_add("A", print_altitude); 
  cmd_add("V", print_voltage);
  cmd_add("ERASE", erase_flash);
  cmd_add("I", cmd_set_interactive);
  cmd_add("i", cmd_unset_interactive);
//  cmd_add("RR", read_register);
//  cmd_add("W", write_flash);
  cmd_add("R", read_flash);
  cmd_add("r", read_flash_binary);
  
  cmd_set_print_function(print);

  //1 second delay to give the user time to release the power on button. 
  //This prevents us detecting the release as a seperate event later on.
  HAL_Delay(100);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    tick = HAL_GetTick();
    if ((tick - last_tick) >= refresh_time) {
      /* Make measurements */
      last_tick = tick;

      /* non-reentrant timer based function calls */
      led_blink();
      measure_battery_voltage();

      switch (read_button()) {
        case 3:
          sleep = SHUTDOWN;
          break;
        case 2:
          sleep = SLEEP;
          break;
        case 1:
          if (recording) {
            recording = false;
            led_add_number_sequence((uint16_t)(max_altitude / 100));
            close_flash();
          } else {
            recording = true;
            ground_altitude = altitude;
            max_altitude = 0;
            led_add_sequence(recording_sequence);
            open_flash();
          }
          break;
        default:
          break;
      }    

      /* measurements */
      voltage = get_battery_voltage();
      temperature = bmp_get_temperature();
      pressure = bmp_get_pressure();
      altitude = bmp_get_altitude();    
      
      altitude_above_ground = altitude - ground_altitude;
      
      if (altitude_above_ground > max_altitude) {
        max_altitude = altitude_above_ground;
      }


      /* don't do this too often, it'll fill the flash pretty quick (400k samples total)
       * before saving we need to "open" the flash, and if we don't close when we're 
       * done we'll corrupt the filesystem.
       */
      if (recording) {
        //save("P", &pressure, 4);
        save('A', (uint8_t *)&altitude, 4);
      }



      /* Deal with data recieved via USB */
      if (rx_buffer_index > 0) {
        cmd_read_input((char *)rx_buffer, rx_buffer_index);
        rx_buffer_index = 0;
      }
    
      /* Transmit any data in the output buffer */
      if (tx_buffer_index > 0) {
        if (CDC_Transmit_FS(UserTxBufferFS, tx_buffer_index) == USBD_OK) {
          tx_buffer_index = 0;
        }
      }
    }
   
    // Pause execution until woken by an interrupt.
    // The systick will do this for us every ms if we're in normal mode.
    power_down(sleep);
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
  //Led_Sequence(usb_sequence);
}

void USB_Disconnect(void) {
  //Led_Sequence(idle_sequence);
}

uint8_t get_button_state(void) {
  return HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin);
}

uint8_t read_button() {
  static uint16_t button_counter = 0;
  uint8_t button_result = 0;

  if (get_button_state()) {
    button_counter++;
  } else {
    if (button_counter > 250) {
      button_result = 3;
    } else if (button_counter > 100) {
      button_result = 2;
    } else if (button_counter > 2) {
      button_result = 1;
    }
    button_counter = 0;
  }
  return button_result;
}  

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  // Check which version of the timer triggered this callback and toggle LED
  if (htim == &htim16) {
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
