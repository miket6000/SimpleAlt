#include "filesystem.h"
#include "w25qxx.h"

/* Memory Map
 * 0x000000 - 0x000fff Recording start addresses (4 byte addresses)
 * 0x001000 - 0x001fff Config data (1 byte label, 4 byte value)
 * 0x002000 - 0x00ffff Reserved
 * 0x010000 - 0x200000 Recordings
*/

#define INDEX_START_ADDRESS     0x000000
#define INDEX_END_ADDRESS       0x00FFFF
#define RECORDING_START_ADDRESS 0x010000
#define RECORDING_END_ADDRESS   0x1FFFFF

#define BLANK       0xFFFFFFFF

static FSState fs_state = FS_STOPPED;
static W25QXX_HandleTypeDef w25qxx; 
static uint32_t next_free_index = INDEX_START_ADDRESS;
static uint32_t next_free_address = RECORDING_START_ADDRESS;
static const uint8_t block_size = 5;

FSResult fs_init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin) {
  uint8_t data[block_size];

  w25qxx_init(&w25qxx, hspi, cs_port, cs_pin); 
  w25qxx_wake(&w25qxx); // just in case we were recently asleep

  w25qxx_read(&w25qxx, next_free_index, data, block_size);
  while ((next_free_index + block_size) <= INDEX_END_ADDRESS && data[0] != 0xFF) {
    next_free_index += block_size;
    w25qxx_read(&w25qxx, next_free_index, data, block_size);  
    if (data[0] == LABEL_RECORD_END) {
      next_free_address = *(uint32_t *)&data[1];
    }
  }
  
  fs_state = FS_CLEAN;
  return FS_OK;
}

FSResult fs_stop() {
  fs_flush();
  w25qxx_sleep(&w25qxx);
  fs_state = FS_STOPPED;
  return FS_OK;
}

FSResult fs_flush() {
  if (fs_state == FS_DIRTY) {
    fs_save_config(LABEL_RECORD_END, &next_free_address);
  }
  fs_state = FS_CLEAN;
  return FS_OK;
}

uint32_t fs_get_uid() {
  return w25qxx_read_uid(&w25qxx);
}

FSResult fs_save(char label, void *data, uint16_t len) {
  if (next_free_address + len + 1 >= RECORDING_END_ADDRESS) {
    return FS_ERR;
  }

  fs_state = FS_DIRTY;

  w25qxx_write(&w25qxx, next_free_address, (uint8_t *)&label, 1);
  next_free_address += 1;
  w25qxx_write(&w25qxx, next_free_address, (uint8_t *)data, len);
  next_free_address += len;
  return FS_OK;
}

FSResult fs_read_config(char label, uint32_t *variable) {
  uint32_t address = INDEX_START_ADDRESS; 
  uint8_t buffer[5] = {0};

  while (address < INDEX_END_ADDRESS && buffer[0] != 0xff) {
    w25qxx_read(&w25qxx, address, buffer, 5);
    if ((char)buffer[0] == label) {
      *variable = *(uint32_t *)&buffer[1];
    }
    address += 5;
  }
  
  return FS_OK;
}

FSResult fs_save_config(char label, void *data) {
  if ((next_free_index + 5) < INDEX_END_ADDRESS) {
    w25qxx_write(&w25qxx, next_free_index, (uint8_t *)&label, 1);
    w25qxx_write(&w25qxx, next_free_index + 1, (uint8_t *)data, 4);
    next_free_index += 5;
    return FS_OK;
  }
  return FS_ERR;
}

FSResult fs_raw_read(uint32_t address, uint8_t *buffer, uint16_t len) {
  if (w25qxx_read(&w25qxx, address, buffer, len) != W25QXX_Ok) {
    return FS_ERR;
  }
  return FS_OK;
}

FSResult fs_raw_write(uint32_t address, uint8_t *buffer, uint16_t len) {
  if (w25qxx_write(&w25qxx, address, buffer, len) != W25QXX_Ok) {
    return FS_ERR;
  }
  return FS_OK;
}

FSResult fs_erase() {
  if (w25qxx_chip_erase(&w25qxx) != W25QXX_Ok) {
    return FS_ERR;
  }
  next_free_index = INDEX_START_ADDRESS;
  next_free_address = RECORDING_START_ADDRESS;

  return FS_OK;
}

