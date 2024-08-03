#include "led.h"
#include "command.h"
#include "altitude.h"
#include <stdio.h>

#define TEST_LED
//#define TEST_COMMAND
//#define TEST_ALTITUDE


int32_t altitude = 0;
uint32_t pressure = 100200;

uint32_t bmp_get_altitude(void) {
  uint8_t n = (MAX_PRESSURE - pressure) / PRESSURE_STEP;
  uint16_t f = (MAX_PRESSURE - pressure) % PRESSURE_STEP;
  if (pressure <= MAX_PRESSURE && pressure >= MIN_PRESSURE) {
    altitude = altitude_lut[n] + f * (altitude_lut[n+1] - altitude_lut[n]) / PRESSURE_STEP;
  };
  return altitude;
}


void one() {
  printf("Command One\n");
  char *param;
  param = cmd_get_param();

  while (param != NULL) { 
    printf("Param: %s\n", param);
    param = cmd_get_param();
  }
}

void two() {
  printf("Command Two\n");
  char *param;
  param = cmd_get_param();

  while (param != NULL) { 
    printf("Param: %s\n", param);
    param = cmd_get_param();
  }
}

void print(char *buf, uint16_t len) {
  //buf[len] = '\0';
  printf("%.*s", len, buf);
}


int main(void) {
#ifdef TEST_LED
  int8_t seq[] = {1, SHORT_PAUSE,-2}; 
  led_add_sequence(seq);
  led_add_number_sequence(235);
  for(int i = 0; i < 2000; i++) {
    led_blink();
  }
#endif //TEST_LED

#ifdef TEST_ALTITUDE
  for (int p = MIN_PRESSURE; p < MAX_PRESSURE; p += 3050) {
    pressure = p;
    bmp_get_altitude();
    printf("P:%d, A:%d\n", pressure, altitude);
  }

  pressure = MAX_PRESSURE;
  bmp_get_altitude();
  printf("P:%d, A:%d\n", pressure, altitude);

#endif //TEST_ALTITUDE

#ifdef TEST_COMMAND
  cmd_add("ONE", one);
  cmd_add("TWO", two);
  cmd_set_print_function(print);

  char str1[] = "ONE param\n";
  char str2[] = "TW";
  char str3[] = "O another param\n";

  cmd_read_input(str1, sizeof(str1));
  cmd_read_input(str2, sizeof(str2));
  cmd_read_input(str3, sizeof(str3));

#endif //TEST_COMMAND
  return 0;
}


