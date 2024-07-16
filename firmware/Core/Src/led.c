#include "led.h"
#include "gpio.h"
#define BLINK_OFF_TIME 20
#define MAX_QUEUE_LEN 10
#define PAUSE 10
#define END_OF_QUEUE 0xFF
#define MAX_INDEX (32 / 2 - 1)


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

uint32_t blink_queue[] = {3, 5, 7, 0, PAUSE, END_OF_QUEUE};
uint8_t queue_index = 0;
uint8_t blink_index = 0;
uint8_t blink_off_counter = BLINK_OFF_TIME;

uint32_t codes[] = {  
  0x0AAFFFFF, //0
  0x000000AB, //1
  0x00000AAF, //2
  0x00000ABF, //3
  0x0000AAFF, //4
  0x0000ABFF, //5
  0x000AAFFF, //6
  0x000ABFFF, //7
  0x00AAFFFF, //8
  0x00ABFFFF, //9
  0xAAAAAAAA  //PAUSE
};

void Led_Blink() {
  uint32_t code;
  uint8_t blink;

  if (blink_off_counter == 0) {
    code = codes[blink_queue[queue_index]];
    blink = (code >> (2 * blink_index)) & 0x00000003;
    
    if (blink & 0x01) {
      Led(ON);
    } 

    if ((blink & 0x02) && (blink_index <= MAX_INDEX)) {
      blink_index++;
    } else {
      blink_index = 0;
      queue_index++;
      if (queue_index > MAX_QUEUE_LEN || blink_queue[queue_index] == END_OF_QUEUE) {
        queue_index = 0;
      }
    }

    blink_off_counter = BLINK_OFF_TIME;
  } else {
    Led(OFF);
    blink_off_counter--;
  }
}
