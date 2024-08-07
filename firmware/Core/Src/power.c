#include "power.h"
#include "gpio.h"
#include "adc.h"
#include "w25qxx.h"
#include "bmp280.h"
#include "led.h"

#define VOL_OFFSET  200

static uint16_t voltage = 0;
extern W25QXX_HandleTypeDef w25qxx; // Handler for all w25qxx operations! 

void power_down(PowerLevel level) {
  switch (level) {
    case AWAKE:
      break;
    case SNOOZE:
      HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
      break;
    case SLEEP:
      w25qxx_sleep(&w25qxx);
      bmp_reset();
      HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);
      __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
      HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
      HAL_PWR_EnterSTANDBYMode();
      break;
    case SHUTDOWN:
      HAL_GPIO_WritePin(SHUTDOWN_GPIO_Port, SHUTDOWN_Pin, GPIO_PIN_SET);
      break;
    default:
      break;
  }
}

void measure_battery_voltage(void) {
  static uint8_t counter = 0;
  switch (counter) {
    case 0:  
      HAL_GPIO_WritePin(nSENSE_EN_GPIO_Port, nSENSE_EN_Pin, GPIO_PIN_RESET);
      break;
    case 1:
      HAL_ADC_Start_IT(&hadc);
      break;
    case 3:
      HAL_GPIO_WritePin(nSENSE_EN_GPIO_Port, nSENSE_EN_Pin, GPIO_PIN_SET);
      if (voltage < 3000) {
        power_down(SLEEP);
      }
      break;
    default:
      break;
  }
  counter++;  
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    voltage = HAL_ADC_GetValue(hadc) * 6200 / 4096 + VOL_OFFSET;
}

uint16_t get_battery_voltage(void) {
  return voltage;
}
