#include "command.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

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

void cmd_clear_buffer() {
  for (int i = 0; i < RX_BUFFER_LEN; i++) {
    rx_buffer[i] = '\0';
  }
  buffer_index = 0;
}

void cmd_add(const char *command, void (*callback)(void)) {
  strncpy(commands[num_commands].command, command, COMMAND_LEN);
  commands[num_commands].callback = callback;
  num_commands++;
}

void cmd_read_input(char *buffer, uint8_t len) {
  uint8_t inchar;
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
        cmd_clear_buffer();
      }
    } else if (isprint(inchar)) {
      rx_buffer[buffer_index++] = inchar;
      rx_buffer[buffer_index] = '\0';
      if (buffer_index >= RX_BUFFER_LEN) {
        buffer_index = 0;
      }
    }
  }
}
