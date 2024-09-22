#include "altimeter.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <libserialport.h>

#define ALTIMETER_BUFFER_SIZE 64
#define UID_LENGTH  10

static const unsigned int timeout = 1000;
static struct sp_port *port;
static uint32_t uid; // +1 for null terminated string
static int altimeter_get_block(uint8_t *buffer, uint32_t start_address, uint8_t len);

uint32_t altimeter_connect(const char * const port_name) {
  char buffer[255];
  
  if (sp_get_port_by_name(port_name, &port) != SP_OK) {
    printf("[error] unable to find %s\n", port_name);
    return 0;
  }

  if (sp_open(port, SP_MODE_READ_WRITE) != SP_OK) {
    printf("[error] unable to open %s\n", port_name);
    return 0;
  }

  if (strcmp(sp_get_port_usb_product(port),  "STM32 Virtual ComPort") != 0) {
    printf("[error] %s is not an STM32 VCP\n", port_name);
    return 0;
  }

  sp_set_baudrate(port, 115200);
  sp_set_bits(port, 8);
  sp_set_parity(port, SP_PARITY_NONE);
  sp_set_stopbits(port, 1);
  sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);
  
  sp_blocking_write(port, "\n", 1, timeout);    // flush altimeter output buffer
  sp_blocking_write(port, "i\n", 2, timeout);   // turn off interactive mode
  sp_blocking_read(port, buffer, sizeof(buffer), timeout); // flush the os rx buffer
  sp_blocking_write(port, "UID\n", 4, timeout); // get UID
  sp_blocking_read(port, buffer, UID_LENGTH, timeout);

  uid = atoi(buffer);

  return uid;
}

int altimeter_get_block(uint8_t *buffer, const uint32_t start_address, const uint8_t len) {
  char command_str[16];
  
  if (len <= ALTIMETER_BUFFER_SIZE) {
    sprintf(command_str, "r %i %i\n", start_address, len);
    sp_blocking_write(port, command_str, strlen(command_str), timeout);
    sp_blocking_read(port, buffer, len, timeout);
    return 0;
  }

  return (-1);
}

int altimeter_get_data(uint8_t *buffer, const uint32_t start_address, const uint32_t len) {
  uint32_t address = start_address;
  uint32_t end_address = start_address + len;
  uint32_t bytes_to_get = 0;
  uint32_t buffer_address = 0;
  while (address < end_address) {
    bytes_to_get = end_address - address;
    if (bytes_to_get > ALTIMETER_BUFFER_SIZE) {
      bytes_to_get = ALTIMETER_BUFFER_SIZE;
    }
    altimeter_get_block(&buffer[buffer_address], address, bytes_to_get);
    buffer_address += bytes_to_get;
    address += bytes_to_get;
  }
  return 0;
}
