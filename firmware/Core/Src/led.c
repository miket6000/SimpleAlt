#include "led.h"
#include "gpio.h"
#define LED_Pin GPIO_PIN_0
#define LED_GPIO_Port GPIOB

void Led(led_state_t state) {
  switch (state) {
    case ON:
      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
      break;
    case TOGGLE:
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
      break;
    default:
      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
      break;
  }
}

