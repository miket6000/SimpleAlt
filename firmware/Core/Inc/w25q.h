#ifndef W25Q_H
#define W25Q_H
#include <stdint.h>
#include <gpio.h>

#define W25Q_SLEEP 0
#define W25Q_WAKE 1

// initialise flash. Returns device ID if successful, or 0 if an error occured.
uint8_t w25q_init(GPIO_TypeDef *port, uint16_t pin);
void w25q_write_enable(void);
void w25q_write(uint32_t address, uint8_t *tx_buffer, uint16_t len);
void w25q_read(uint32_t address, uint8_t *rx_buffer, uint16_t len);
void w25q_power(uint8_t state);
#endif //W25Q_H
