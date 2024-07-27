#include "spi_wrapper.h"
#include "spi.h"

void spi_read_address(CSPin cs, uint8_t address, uint8_t *rx_buffer, uint8_t len) {
  HAL_GPIO_WritePin(cs.port, cs.pin, GPIO_PIN_RESET);

  HAL_SPI_Transmit(&hspi1, &address, 1, 100);
  HAL_SPI_Receive(&hspi1, rx_buffer, len, 100); 

  HAL_GPIO_WritePin(cs.port, cs.pin, GPIO_PIN_SET);
}

void spi_read_32bit_address(CSPin cs, uint32_t address, uint8_t *rx_buffer, uint8_t len){
  HAL_GPIO_WritePin(cs.port, cs.pin, GPIO_PIN_RESET);

  HAL_SPI_Transmit(&hspi1, (uint8_t *)&address, 4, 100);
  HAL_SPI_Receive(&hspi1, rx_buffer, len, 100); 

  HAL_GPIO_WritePin(cs.port, cs.pin, GPIO_PIN_SET);
}

void spi_write_byte(CSPin cs, uint8_t byte) {
  HAL_GPIO_WritePin(cs.port, cs.pin, GPIO_PIN_RESET);

  HAL_SPI_Transmit(&hspi1, &byte, 1, 100);

  HAL_GPIO_WritePin(cs.port, cs.pin, GPIO_PIN_SET); 
}


void spi_write_address(CSPin cs, uint8_t address, uint8_t *tx_buffer, uint8_t len) {
  HAL_GPIO_WritePin(cs.port, cs.pin, GPIO_PIN_RESET);

  HAL_SPI_Transmit(&hspi1, &address, 1, 100);
  HAL_SPI_Transmit(&hspi1, tx_buffer, len, 100); 

  HAL_GPIO_WritePin(cs.port, cs.pin, GPIO_PIN_SET);
}

void spi_write_32bit_address(CSPin cs, uint32_t address, uint8_t *tx_buffer, uint8_t len) {
  HAL_GPIO_WritePin(cs.port, cs.pin, GPIO_PIN_RESET);

  HAL_SPI_Transmit(&hspi1, (uint8_t *)&address, 4, 100);
  HAL_SPI_Transmit(&hspi1, tx_buffer, len, 100); 

  HAL_GPIO_WritePin(cs.port, cs.pin, GPIO_PIN_SET);
}


