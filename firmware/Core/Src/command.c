#include "command.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "stm32f0xx_hal.h"

#define RX_BUFFER_LEN 32U
#define TERM_CHAR '\n'
#define DELIM " \0"
#define MAX_NUM_COMMANDS 10

static char rx_buffer[RX_BUFFER_LEN] = {0};
static uint8_t buffer_index = 0;
static char *last;
static char *token;
static uint8_t num_commands = 0;
static Command commands[MAX_NUM_COMMANDS];
char prompt[] = "\n> ";
char no_match[] = "\n  Command not recognised.";
bool interactive = true;

void (*cmd_print)(char *str, uint16_t len);

void cmd_set_print_function(void (*function)(char*,uint16_t)) {
  cmd_print = function;
}

void cmd_clear_buffer() {
  for (int i = 0; i < RX_BUFFER_LEN; i++) {
    rx_buffer[i] = '\0';
  }
  buffer_index = 0;
  if (interactive) {
    cmd_print(prompt, sizeof(prompt));
  }
}

void cmd_add(const char *command, void (*callback)(void)) {
  if (num_commands < MAX_NUM_COMMANDS) {
    strncpy(commands[num_commands].command, command, COMMAND_LEN);
    commands[num_commands].callback = callback;
    num_commands++;
  } else {
    // silently fail to add the command...
  }
}

void cmd_read_input(char *buffer, uint8_t len) {
  char inchar;
  bool matched = false;
  for (int i = 0; i < len; i++) {
    inchar = buffer[i];
    if (inchar == TERM_CHAR) {
      buffer_index = 0;
      token = strtok_r(rx_buffer, DELIM, &last);
      if (token == NULL) return;
      matched = false;
      for (int cmd = 0; cmd < num_commands; cmd++) {
        if (strncmp(token, commands[cmd].command, RX_BUFFER_LEN) == 0) {
          (*commands[cmd].callback)();
          cmd_clear_buffer();
          buffer_index = 0;
          matched = true;
          break;
        }
      }
      if (matched == false) {
        //invalid command error handler
        cmd_print(no_match, sizeof(no_match));
        cmd_clear_buffer();
      }
    } else if (isprint(inchar)) {
      cmd_print(&inchar,1);
      rx_buffer[buffer_index++] = inchar;
      rx_buffer[buffer_index] = '\0';
      if (buffer_index >= RX_BUFFER_LEN) {
        buffer_index = 0;
      }
    }
  }
}
