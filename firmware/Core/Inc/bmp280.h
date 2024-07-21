#ifndef BMP280_H
#define BMP280_H
#include <stdint.h>
#include "gpio.h"

void bmp_init(GPIO_TypeDef *port, uint16_t pin);
int32_t bmp_get_temperature(void);
uint32_t bmp_get_pressure(void);

#endif //BMP280_H
