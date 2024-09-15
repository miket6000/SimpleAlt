#include "power.h"
#include "gpio.h"
#include "adc.h"
#include "bmp280.h"
#include "led.h"
#include "filesystem.h"

#define VOLTAGE_OFFSET    200
#define NUM_ADC_BITS      12
#define VOLTAGE_SLOPE     6200
#define MIN_TIMEOUT       60
// If running without a battery you must set the alarm voltage to 0
#define VOLTAGE_LOW_ALARM 3000 

static uint16_t voltage = 0;
static PowerMode power_mode = SNOOZE;
static uint32_t idle_timeout = SECONDS_TO_TICKS(1200);
static uint32_t idle_timer = SECONDS_TO_TICKS(1200);

void power_set_mode(PowerMode mode) {
  power_mode = mode;  
}

void power_set_timeout(uint32_t timeout) {
  idle_timeout = timeout;
  idle_timer = timeout;
}

void power_tick() {
  static uint16_t measurement_timer = 0;
  
  switch (measurement_timer++) {
    case 0:  
      HAL_GPIO_WritePin(nSENSE_EN_GPIO_Port, nSENSE_EN_Pin, GPIO_PIN_RESET);
      break;
    case 1:
      HAL_ADC_Start_IT(&hadc);
      break;
    case 3:
      HAL_GPIO_WritePin(nSENSE_EN_GPIO_Port, nSENSE_EN_Pin, GPIO_PIN_SET);
      if (voltage < VOLTAGE_LOW_ALARM) {
        power_mode = SLEEP; 
      }
      break;
    case 99: // one second, reset timer. 
      measurement_timer = 0;
    default:
      break;
  }

  if (idle_timer-- == 0 && idle_timeout > MIN_TIMEOUT) {
    power_mode = SLEEP;
  }
}

void power_idle_reset() {
  idle_timer = idle_timeout;
}

void power_management() {
  switch (power_mode) {
    case AWAKE:
      break;
    case SNOOZE:
      HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
      break;
    case SLEEP:
      fs_stop();
      bmp_reset();
      HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);
      __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
      HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
      HAL_PWR_EnterSTANDBYMode();
      break;
    case SHUTDOWN:
      fs_stop();
      HAL_GPIO_WritePin(SHUTDOWN_GPIO_Port, SHUTDOWN_Pin, GPIO_PIN_SET);
      break;
    default:
      break;
  }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    voltage = ((HAL_ADC_GetValue(hadc) * VOLTAGE_SLOPE) >> NUM_ADC_BITS) + VOLTAGE_OFFSET;
}

uint16_t power_get_battery_voltage(void) {
  return voltage;
}
