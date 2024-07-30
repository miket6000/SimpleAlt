#ifndef BMP280_H
#define BMP280_H
#include <stdint.h>
#include "gpio.h"

/* Configuration Options
 * See BMP280 datasheet for values based on system requirements
 */
#define BMP_ODR         0x00
#define BMP_FILTER      0x02
#define BMP_SPI         0x00
#define BMP_TEMP_OS     0x01
#define BMP_PRES_OS     0x05
#define BMP_POWER_MODE  0x03

typedef enum {
  BMP_ID = 0xD0,
  BMP_RESET = 0xE0,
  BMP_STATUS = 0xF3,
  BMP_CTRL_MEAS = 0xF4,
  BMP_CONFIG = 0xF5,
  BMP_PRES_MSB = 0xF7,
  BMP_PRES_LSB = 0xF8,
  BMP_PRES_XLSB = 0xF9,
  BMP_TEMP_MSB = 0xFA,
  BMP_TEMP_LSB = 0xFB,
  BMP_TEMP_XLSB = 0xFC,
  BMP_CALIB00 = 0x88
} BMP280Register;

typedef struct {
  uint16_t dig_T1;
  int16_t dig_T2;
  int16_t dig_T3;
  uint16_t dig_P1;
  int16_t dig_P2;
  int16_t dig_P3;
  int16_t dig_P4;
  int16_t dig_P5;
  int16_t dig_P6;
  int16_t dig_P7;
  int16_t dig_P8;
  int16_t dig_P9;
} BMPCalibrationData;

void bmp_init(GPIO_TypeDef *port, uint16_t pin);
int32_t bmp_get_temperature(void);
uint32_t bmp_get_pressure(void);
int32_t bmp_get_altitude(void);
void bmp_set_ground_level(void);
#endif //BMP280_H
