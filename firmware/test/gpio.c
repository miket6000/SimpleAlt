#include "gpio.h"
#include <stdio.h>

int state = 0;

void HAL_GPIO_WritePin(int port, int pin, int state) {
  if (state == 0) {
    printf(".");
  } else {
    printf("o");
  }
}

void HAL_GPIO_TogglePin(int port, int pin) {
  if (state == 0) {
    state = 1;
  } else {
    state = 0;
  }
  HAL_GPIO_WritePin(0, 0, state);
}

