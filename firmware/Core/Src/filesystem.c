#include "filesystem.h"
#include "w25qxx.h"

#define DATA_OFFSET 0x10000LLU
#define MAX_ADDRESS 0x1FFFFFLLU

static FilesystemState g_fs_state = FS_STOPPED;
static W25QXX_HandleTypeDef g_w25qxx;
static uint32_t g_next_free_index_slot = 0;
static uint32_t g_next_free_address = 0;


void fs_init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin) {
  w25qxx_init(&g_w25qxx, hspi, cs_port, cs_pin);
  w25qxx_wake(&g_w25qxx); // just in case we were recently asleep
  g_fs_state = FS_CLOSED;
}

void fs_stop() {
  fs_close();
  w25qxx_sleep(&g_w25qxx);
  g_fs_state = FS_STOPPED;
}

void fs_open() {
  uint32_t i = 0;
  uint32_t address = 0;
  uint32_t last_address = DATA_OFFSET;
  w25qxx_read(&g_w25qxx, i, (uint8_t *)&address, 4);
  while (i < DATA_OFFSET && address < MAX_ADDRESS) {
    i += 4;
    last_address = address;
    w25qxx_read(&g_w25qxx, i, (uint8_t *)&address, 4);
  }
  g_next_free_index_slot = i;
  g_next_free_address = last_address;
  g_fs_state = FS_OPEN;
}

void fs_close() {
  if (g_fs_state == FS_OPEN) {
    w25qxx_write(&g_w25qxx, g_next_free_index_slot, (uint8_t *)&g_next_free_address, 4);
  }
  g_fs_state = FS_CLOSED;
}

//TODO add error state to return value so that this can be monitored and corrected
// or indicated to the user.

void fs_save(char const label, void * const data, uint16_t const len) {
  if (g_fs_state == FS_OPEN && g_next_free_address + len + 1 < MAX_ADDRESS) {
    w25qxx_write(&g_w25qxx, g_next_free_address, (uint8_t *)&label, 1);
    w25qxx_write(&g_w25qxx, g_next_free_address + 1, (uint8_t *)data, len);
    g_next_free_address += (len + 1);
  }
}

void fs_raw_read(uint32_t address, uint8_t *buffer, uint16_t len) {
  w25qxx_read(&g_w25qxx, address, buffer, len);
}

void fs_raw_write(uint32_t address, uint8_t *buffer, uint16_t len) {
  w25qxx_write(&g_w25qxx, address, buffer, len);
}

void fs_erase() {
  w25qxx_chip_erase(&g_w25qxx);
}

