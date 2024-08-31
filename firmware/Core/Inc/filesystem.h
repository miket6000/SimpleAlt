#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include "spi.h"
#include "gpio.h"

typedef enum {
  FS_STOPPED = 0,
  FS_CLOSED,
  FS_OPEN,
} FilesystemState;

void fs_init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
void fs_stop();
void fs_open();
void fs_close();

/**!
 *  @brief Save a data record
 *  @param label Single character label for the data item.
 *  @param data  Pointer to the data to be saved
 *  @param len   size of the data in bytes
*/
void fs_save(char const label, void * const  data, uint16_t const len);

void fs_raw_read(uint32_t address, uint8_t *buffer, uint16_t len);
void fs_raw_write(uint32_t address, uint8_t *buffer, uint16_t len);
void fs_erase();

#endif //FILESYSTEM_H
