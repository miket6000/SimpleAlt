#include "altimeter.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <libserialport.h>

#define FIRST_ADDRESS 0x10000
#define ALTIMETER_BUFFER_SIZE 64
#define UID_LENGTH  8
#define RECORD_LENGTH 5
#define RECORD_ALIGNED_BUFFER_SIZE (ALTIMETER_BUFFER_SIZE - (ALTIMETER_BUFFER_SIZE % RECORD_LENGTH))

static const unsigned int timeout = 1000;
static struct sp_port *port;
static uint32_t addresses[MAX_NUM_ADDRESSES];
static uint32_t num_addresses = 0;
static LogState log_state;
static char uid[UID_LENGTH + 1] = {0}; // +1 for null terminated string

static int altimeter_get_block(uint8_t *buffer, uint32_t start_address, uint8_t len);

char *altimeter_connect(const char * const port_name) {
  if (sp_get_port_by_name(port_name, &port) != SP_OK) {
    return (NULL);
  }

  if (sp_open(port, SP_MODE_READ_WRITE) != SP_OK) {
    return (NULL);
  }

  sp_set_baudrate(port, 115200);
  sp_set_bits(port, 8);
  sp_set_parity(port, SP_PARITY_NONE);
  sp_set_stopbits(port, 1);
  sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);
  
  //Not sure why this fails occasionally, but until I figure it out, I can fudge it...
  uint8_t retries = 0;
  while (uid[0] == '\0' && retries < 3) {
    sp_blocking_write(port, "\n", 1, timeout);    // flush altimeter input buffer
    sp_blocking_write(port, "i\n", 2, timeout);   // turn off interactive mode
    sp_blocking_write(port, "UID\n", 4, timeout); // get UID
    sp_blocking_read(port, &uid, UID_LENGTH, timeout);
    retries++;
  }

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

inline static uint32_t swap32(const uint8_t * const bytes) {
  return (uint32_t)((bytes[3] << 24) + (bytes[2] << 16) + (bytes[1] << 8) + bytes[0]);
}

int altimeter_get_addresses() {
  static bool valid_address = true;
  unsigned int block_start_address = 0;
  uint8_t buf[65];
  
  addresses[0] = FIRST_ADDRESS;
  
  /* Do block reads 64 bytes at a time to get the addresses of flight logs */
  while (valid_address) {
    altimeter_get_block(buf, block_start_address, 64);
  
    for (int i = 0; i < 64; i+=4) {
      uint32_t address = swap32(&buf[i]); 
      if (address < ((1 << 24) - 1)) {
        num_addresses++;
        addresses[num_addresses] = address;
      } else {
        valid_address = false;
      }
    }
  
    block_start_address += 64;
  }
  return 0;
}

int altimeter_get_recording_count() {
  return num_addresses;
}

int altimeter_get_recording_length(const uint32_t recording) {
  if (recording > num_addresses) {
    return (-1);
  }

  return (addresses[recording + 1] - addresses[recording]) / RECORD_LENGTH;
}

int altimeter_get_data(uint8_t *buffer, const uint32_t start_address, const uint32_t end_address) {
  uint32_t address = start_address;
  uint32_t bytes_to_get = 0;
  uint32_t buffer_address = 0;
  while (address < end_address) {
    bytes_to_get = end_address - address;
    if (bytes_to_get > RECORD_ALIGNED_BUFFER_SIZE) {
      bytes_to_get = RECORD_ALIGNED_BUFFER_SIZE;
    }
    altimeter_get_block(&buffer[buffer_address], address, bytes_to_get);
    buffer_address += bytes_to_get;
    address += bytes_to_get;
  }
  return 0;
}

int altimeter_get_recording(int32_t *buffer, const uint32_t recording, const uint32_t len) {
  if (recording > num_addresses) {
    return (-1);
  }

  uint8_t buf[ALTIMETER_BUFFER_SIZE + 1];
  const uint32_t start_address = addresses[recording];
  uint32_t end_address = start_address; 
  uint32_t address = start_address;
  uint32_t result_counter = 0;
  uint32_t bytes_to_get = 0;

  if (altimeter_get_recording_length(recording) > len * RECORD_LENGTH) {
    end_address = start_address + (len * RECORD_LENGTH);
  } else {
    end_address = addresses[recording + 1];
  }
  
  //printf("start_address: %i, end_address: %i\n", start_address, end_address);


  while (address < end_address) {
    bytes_to_get = end_address - address;
    if (bytes_to_get > RECORD_ALIGNED_BUFFER_SIZE) {
      bytes_to_get = RECORD_ALIGNED_BUFFER_SIZE;
    }
    
    //printf("address: %i, end_address: %i, bytes_to_get: %i\n", address, end_address, bytes_to_get);


    altimeter_get_block(buf, address, bytes_to_get);

    for (int i = 0; i < bytes_to_get; i+=RECORD_LENGTH) {
//      if (result_counter > 910 && result_counter < 919) {
//        printf("result_counter: %i, address: %i, bytes_to_get: %i, i: %i\n", result_counter, address, bytes_to_get, i);
//        printf("buf[i]: %c, swap32(buf[i+1..3]): %i\n\n", buf[i], swap32(&buf[i+1]));
//      }
      if (buf[i] == 'A') {
        buffer[result_counter++] = swap32(&buf[i+1]);
      }
    }
    
    address += bytes_to_get;
  }
  return 0;
}
