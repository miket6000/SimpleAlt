#include "w25q.h"
#include "spi_wrapper.h"

//void spi_write_address(CSPin cs, uint8_t address, uint8_t *tx_buffer, uint8_t len);

static uint8_t rx_buffer[8] = {0};
static uint8_t device_id = 0;
static CSPin flash_cs;

// initialise flash. Returns device ID if successful, or 0 if an error occured.
uint8_t w25q_init(GPIO_TypeDef *port, uint16_t pin) {
  //spi_write(wr_en_set,1);
  flash_cs.port = port;
  flash_cs.pin = pin; 
  w25q_power(W25Q_WAKE); // just in case we were asleep  
  spi_read_address(flash_cs, 0x90, rx_buffer, 5);
  return rx_buffer[3];
}

//0x05 = status register 1
//0x35 = status register 2
//0x15 = status register 3
//
uint8_t w25q_read_register(uint8_t address) {
  uint8_t reg = 0;
  spi_read_address(flash_cs, address, &reg, 1); 
  return reg;
}

uint8_t w25q_read_status(void) {
  uint8_t status = 0;
  spi_read_address(flash_cs, 0x05, &status, 1);
  return status;
}

void w25q_write_enable(void) {
  spi_write_byte(flash_cs, 0x06); 
}

void w25q_erase_chip() {
  w25q_write_enable();
  spi_write_byte(flash_cs, 0xC7);
}

void w25q_write(uint32_t address, uint8_t *tx_buffer, uint16_t len) {
  address &= 0x00FFFFFF; // SPI only has a 24 bit address space
  address |= 0x02000000; // upper byte is command, 0x02 = write.
  spi_write_32bit_address(flash_cs, address, tx_buffer, len);
}

void w25q_read(uint32_t address, uint8_t *rx_buffer, uint16_t len) {
  address &= 0x00FFFFFF; // SPI only has a 24 bit address space
  address |= 0x03000000; // upper byte is command, 0x03 = read.
  spi_read_32bit_address(flash_cs, address, rx_buffer, len);
}

void w25q_power(uint8_t state) {
  if (state == W25Q_SLEEP) {
    spi_write_byte(flash_cs, 0xB9);
  } else {
    spi_write_byte(flash_cs, 0xAB);
  }
}

