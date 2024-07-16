#include "led.h"
#include "gpio.h"
#define BLINK_OFF_TIME 17
#define MAX_SEQUENCE_LEN 10
#define SHORT_PAUSE 10
#define PAUSE 11
#define END_OF_SEQUENCE 0xFF
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

uint32_t blink_sequence[] = {3, 5, 7, 0, PAUSE, END_OF_SEQUENCE};
uint8_t sequence_index = 0;
uint8_t blink_index = 0;
uint8_t blink_off_counter = BLINK_OFF_TIME;

/* Seqences are read right to left, two bits at a time. The first bit of each pair
 * indicates that this is part of the current sequence. The second bit indicates 
 * whether the LED is on (1) or off (0).
 * LEDs are on for 1 cycle, then off for BLINK_OFF_TIME cycles. 
 * Cycle length is determined by time between calls to Led_Blink().
 */
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
  0x0000AAAA, //SHORT_PAUSE
  0xAAAAAAAA  //PAUSE
};

void Led_Blink() {
  uint32_t code;
  uint8_t blink;

  if (blink_off_counter == 0) {
    code = codes[blink_sequence[sequence_index]];
    blink = (code >> (2 * blink_index)) & 0x00000003;
    
    if (blink & 0x01) {
      Led(ON);
    } 

    if ((blink & 0x02) && (blink_index <= MAX_INDEX)) {
      blink_index++;
    } else {
      blink_index = 0;
      sequence_index++;
      if (sequence_index > MAX_SEQUENCE_LEN || blink_sequence[sequence_index] == END_OF_SEQUENCE) {
        sequence_index = 0;
      }
    }

    blink_off_counter = BLINK_OFF_TIME;
  } else {
    Led(OFF);
    blink_off_counter--;
  }
}
