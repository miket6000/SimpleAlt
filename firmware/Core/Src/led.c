#include "led.h"
#include "gpio.h"

#define MAX_BLINK (32 / 2 - 1)
#define LED_MASK 0x01
#define VALID_MASK 0x02

/* Sequencer state variables */
static uint8_t sequence_index = 0;
static uint8_t blink_index = 0;
static uint8_t blink_off_counter = BLINK_OFF_TIME;
static uint8_t sequence_head = 0;
static int8_t sequence[SEQUENCE_LEN] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

/* Seqence codes are read right to left, two bits at a time. The first bit of each pair
 * indicates that this code is still valid. The second bit indicates whether the LED is 
 * on (1) or off (0) for this cycle.
 * LEDs are on for 1 sub-cycle, then off for BLINK_OFF_TIME sub-cycles. 
 * Cycle length is determined by time between calls to Led_Blink().
 */
static const uint32_t codes[] = {  
  0x02AFFFFF, //0
  0x000000AB, //1 0000000010101011
  0x000002AF, //2 0000001010101111
  0x00000ABF, //3 0000101010111111
  0x00002AFF, //4 0010101011111111
  0x0000ABFF, //5
  0x0002AFFF, //6
  0x000ABFFF, //7
  0x002AFFFF, //8
  0x00ABFFFF, //9
  0x0000AAAA, //SHORT_PAUSE
  0xAAAAAAAA, //PAUSE
  0x00000000  //NOTHING
};

static void step_sequencer(void);

void led(const LedState state) {
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

void led_reset_sequence(void) {
  sequence_index = 0;
  blink_index = 0;
  blink_off_counter = BLINK_OFF_TIME;
  sequence_head = 0;
}

void led_add_number_sequence(const uint16_t number) {
  int8_t s[] = {NOTHING, NOTHING, NOTHING, NOTHING, PAUSE, -5};
  uint16_t whole = 0;

  if ((number / 1000) > 0) {
    s[0] = number / 1000;
    whole = s[0] * 1000;
  }
  if ((number / 100) > 0) {
    s[1] = (number - whole) / 100;
    whole = whole + s[1] * 100;
  }
  if ((number / 10) > 0) {
    s[2] = (number - whole) / 10;
    whole = whole + s[2] * 10;
  }

  s[3] = (number - whole);
  led_add_sequence(s);
}


void led_add_sequence(const int8_t *const new_sequence) {
  uint8_t i = 0;

  while (new_sequence[i] >= 0) {
    sequence[sequence_head] = new_sequence[i];
    sequence_head++;
    sequence_head %= SEQUENCE_LEN;
    i++;
  }
  sequence[sequence_head] = new_sequence[i];
}

void led_blink(void) {
  uint8_t blink;

  if (blink_off_counter == 0) {
    blink = (codes[sequence[sequence_index]] >> (2 * blink_index)) & 0x00000003;
    
    if (blink & LED_MASK) {
      led(ON);
    } else {
      led(OFF);
    } 

    if ((blink & VALID_MASK) && (blink_index <= MAX_BLINK)) {
      blink_index++;
    } else {
      blink_index = 0;
      step_sequencer();
    }
    
    blink_off_counter = BLINK_OFF_TIME;
  } else {
    led(OFF);
    blink_off_counter--;
  }
}

static void step_sequencer() {
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


