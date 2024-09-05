#include "button.h"
#include <stdint.h>
#include "main.h"

static uint8_t button_read(void) {
  return HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin);
}

ButtonState button_get_state() {
  static uint16_t button_counter = 0;
  static ButtonState button_result = BUTTON_IDLE;

  if (button_read() == 1) { // button is down
    button_counter++;
    if (button_counter > 2) {
      if (button_result == BUTTON_IDLE) {
        button_result = BUTTON_DOWN;
      } else {
        button_result = BUTTON_HELD;
      }
    } else {
      button_result = BUTTON_IDLE;
    }
  } else { // button is up
    if (button_counter > SECONDS_TO_TICKS(4.4)) { // blinking 1 2 3 takes 4.32 seconds 
      button_result = BUTTON_RELEASE_3;
    } else if (button_counter > SECONDS_TO_TICKS(2.2)) { // blinking 1 2 takes 2.16 seconds
      button_result = BUTTON_RELEASE_2;
    } else if (button_counter > SECONDS_TO_TICKS(0.4)) { // blinking 1 takes 0.36 seconds
      button_result = BUTTON_RELEASE_1;
    } else if (button_counter > 0) {
      button_result = BUTTON_RELEASE_0; // bounce.
    } else {
      button_result = BUTTON_IDLE;
    }
    button_counter = 0;
  }

  return button_result;
}  

