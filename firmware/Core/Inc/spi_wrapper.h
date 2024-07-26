#ifndef SPI_WRAPPER_H
#define SPI_WRAPPER_H

#include "gpio.h"

typedef struct {
  uint16_t pin;
  GPIO_TypeDef *port;
} CSPin;

void spi_read_address(CSPin cs, uint8_t address, uint8_t *rx_buffer, uint8_t len);
void spi_write_address(CSPin cs, uint8_t address, uint8_t *tx_buffer, uint8_t len);

#endif //SPI_WRAPPER_H
