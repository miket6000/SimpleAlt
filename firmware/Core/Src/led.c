#include "led.h"
#include "gpio.h"

#define MAX_BLINK (32 / 2 - 1)
#define LED_MASK 0x01
#define VALID_MASK 0x02

/* Sequencer state variables */
uint8_t sequence_index = 0;
uint8_t blink_index = 0;
uint8_t blink_off_counter = BLINK_OFF_TIME;
uint8_t sequence_head = 0;
int8_t sequence[SEQUENCE_LEN] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

/* Seqence codes are read right to left, two bits at a time. The first bit of each pair
 * indicates that this code is still valid. The second bit indicates whether the LED is 
 * on (1) or off (0) for this cycle.
 * LEDs are on for 1 sub-cycle, then off for BLINK_OFF_TIME sub-cycles. 
 * Cycle length is determined by time between calls to Led_Blink().
 */
uint32_t codes[] = {  
  0x0AAFFFFF, //0
  0x000000AB, //1
  0x000002AF, //2
  0x00000ABF, //3
  0x00002AFF, //4
  0x0000ABFF, //5
  0x0002AFFF, //6
  0x000ABFFF, //7
  0x002AFFFF, //8
  0x00ABFFFF, //9
  0x0000AAAA, //SHORT_PAUSE
  0xAAAAAAAA, //PAUSE
  0x00000000  //NOTHING
};

void step_sequencer(void);

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

void Led_Sequence(int8_t *new_sequence) {
  uint8_t i = 0;

  while (new_sequence[i] >= 0) {
    sequence[sequence_head] = new_sequence[i];
    sequence_head++;
    sequence_head %= SEQUENCE_LEN;
    i++;
  }
  sequence[sequence_head] = new_sequence[i];
}

void Led_Blink() {
  uint8_t blink;

  if (blink_off_counter == 0) {
    blink = (codes[sequence[sequence_index]] >> (2 * blink_index)) & 0x00000003;
    
    if (blink & LED_MASK) {
      Led(ON);
    } else {
      Led(OFF);
    } 

    if ((blink & VALID_MASK) && (blink_index <= MAX_BLINK)) {
      blink_index++;
    } else {
      blink_index = 0;
      step_sequencer();
    }
    
    blink_off_counter = BLINK_OFF_TIME;
  } else {
    Led(OFF);
    blink_off_counter--;
  }
}

void step_sequencer() {
  sequence_index++;
  if (sequence_index >= SEQUENCE_LEN) {
    sequence_index = 0;
  };

  if (sequence[sequence_index] < 0) {
    if (sequence_index + sequence[sequence_index] < 0) {
      sequence_index += sequence[sequence_index] + SEQUENCE_LEN;
    } else {
      sequence_index += sequence[sequence_index];
    }
  }
}


