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
} led_state_t;

void Led(led_state_t state);
void Led_Sequence(int8_t *seq);
void Led_Blink();
#endif
