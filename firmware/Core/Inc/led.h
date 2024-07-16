#ifndef LED_H
#define LED_H

#include "stdint.h"

#define BLINK_OFF_TIME 17
#define MAX_SEQUENCE_LEN 10

#define SHORT_PAUSE 10
#define PAUSE 11
#define END_OF_SEQUENCE 0xFF

typedef enum {
  OFF, 
  ON, 
  TOGGLE
} led_state_t;

void Led(led_state_t state);
void Led_Blink(uint32_t *sequence);
#endif
