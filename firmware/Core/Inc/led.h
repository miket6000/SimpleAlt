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

void led(LedState state);
void led_add_sequence(const int8_t *const seq);
void led_blink();
#endif
