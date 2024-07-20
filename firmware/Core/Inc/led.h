#ifndef LED_H
#define LED_H

#include "stdint.h"

#define BLINK_OFF_TIME 17
#define SEQUENCE_LEN 12

#define SHORT_PAUSE 10
#define PAUSE 11
#define NOTHING 12
#define END_OF_SEQUENCE 0x80

typedef enum {
  OFF, 
  ON, 
  TOGGLE
} LedState;

void set_led(LedState state);
void add_led_sequence(int8_t *seq);
void blink();
#endif
