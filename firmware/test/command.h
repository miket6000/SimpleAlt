#ifndef COMMAND_H
#define COMMAND_H
#include <stdint.h>

#define COMMAND_LEN 8U

void cmd_add(const char *command, void (*callback)(void));
void cmd_read_input(char *buffer, uint8_t len);

typedef struct {
  char command[COMMAND_LEN];
  void (*callback)(void);
} Command;

#endif //COMMAND_H
