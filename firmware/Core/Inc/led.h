#ifndef LED_H
#define LED_H

typedef enum {
  OFF, 
  ON, 
  TOGGLE
} led_state_t;

void Led(led_state_t state);

#endif
