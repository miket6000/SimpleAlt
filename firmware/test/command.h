#ifndef COMMAND_H
#define COMMAND_H
#include <stdint.h>

#define COMMAND_LEN 8U

void cmd_set_print_function(void(*function)(char *, uint16_t));
void cmd_add(const char *command, void (*callback)(void));
void cmd_read_input(char *buffer, uint8_t len);
char *cmd_get_param(void);

typedef struct {
  char command[COMMAND_LEN];
  void (*callback)(void);
} Command;

#endif //COMMAND_H
