#ifndef BUTTON_H
#define BUTTON_H

typedef enum {
  BUTTON_IDLE = 0,
  BUTTON_DOWN,
  BUTTON_HELD, 
  BUTTON_RELEASE_0,
  BUTTON_RELEASE_1,
  BUTTON_RELEASE_2,
  BUTTON_RELEASE_3
} ButtonState;

ButtonState button_get_state();

#endif //BUTTON_H
