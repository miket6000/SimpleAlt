#ifndef COMMANDS_H
#define COMMANDS_H
#include <stdint.h>

void print(char *tx_buffer, uint16_t len);
void print_uint32(void *parameter);
void print_int32(void *parameter);
void print_uint16(void *parameter);
void print_int16(void *parameter);
void print_uint8(void *parameter);
void print_int8(void *parameter);
void write_flash(void *parameter);
void read_flash(void *parameter);
void read_flash_binary(void *parameter);
void erase_flash(void *parameter);
void factory_reset(void *parameter);
void reboot(void *parameter);
void set_config(void *parameter);
void get_config(void *parameter);
void get_uid(void *parameter);

#endif // COMMANDS_H
