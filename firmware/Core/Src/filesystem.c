#include "filesystem.h"
#include "w25qxx.h"

#define DATA_OFFSET 0x10000LLU
#define MAX_ADDRESS 0x1FFFFFLLU

FilesystemState fs_state = FS_STOPPED;
W25QXX_HandleTypeDef w25qxx;
uint32_t next_free_index_slot = 0;
uint32_t next_free_address = 0;

void fs_init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin) {
  w25qxx_init(&w25qxx, hspi, cs_port, cs_pin);
  w25qxx_wake(&w25qxx); // just in case we were recently asleep
  fs_state = FS_CLOSED;
}

void fs_stop() {
  fs_close();
  w25qxx_sleep(&w25qxx);
  fs_state = FS_STOPPED;
}

void fs_open() {
  uint32_t i = 0;
  uint32_t address = 0;
  uint32_t last_address = DATA_OFFSET;
  w25qxx_read(&w25qxx, i, (uint8_t *)&address, 4);
  while (i < DATA_OFFSET && address < MAX_ADDRESS) {
    i += 4;
    last_address = address;
    w25qxx_read(&w25qxx, i, (uint8_t *)&address, 4);
  }
  next_free_index_slot = i;
  next_free_address = last_address;
  fs_state = FS_OPEN;
}

void fs_close() {
  if (fs_state == FS_OPEN) {
    w25qxx_write(&w25qxx, next_free_index_slot, (uint8_t *)&next_free_address, 4);
  }
  fs_state = FS_CLOSED;
}

//TODO add error state to return value so that this can be monitored and corrected
// or indicated to the user.
void fs_save(char label, void * data, uint16_t len) {
  if (fs_state == FS_OPEN && next_free_address + len + 1 < MAX_ADDRESS) {
    w25qxx_write(&w25qxx, next_free_address, (uint8_t *)&label, 1);
    w25qxx_write(&w25qxx, next_free_address + 1, data, len);
    next_free_address += (len + 1);
  }
}

void fs_raw_read(uint32_t address, uint8_t *buffer, uint16_t len) {
  w25qxx_read(&w25qxx, address, buffer, len);
}

void fs_raw_write(uint32_t address, uint8_t *buffer, uint16_t len) {
  w25qxx_write(&w25qxx, address, buffer, len);
}

void fs_erase() {
  w25qxx_chip_erase(&w25qxx);
}

