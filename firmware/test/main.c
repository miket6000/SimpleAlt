#include "led.h"
#include "command.h"
#include <stdio.h>

//#define TEST_LED
#define TEST_COMMAND


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
  int8_t seq[] = {2, 3, PAUSE,-1}; 
  Led_Sequence(seq);
   
  for(int i = 0; i < 2000; i++) {
    Led_Blink();
  }
#endif //TEST_LED


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


