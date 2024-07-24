#include "bmp280.h"
#include "gpio.h"
#include "spi_wrapper.h"
#include <stdint.h>
#include "altitude.h"

/* Globals */
const uint32_t standard_pressure = 101325;
CSPin bmp_cs;
BMPCalibrationData cal;
uint32_t pressure;
uint32_t temperature;
uint32_t altitude;
uint16_t velocity;
uint8_t spi_rx_buffer[26];

/* Private functions prototypes */
void bmp_get_calibration(void);
static int32_t bmp280_compensate_T_int32(int32_t adc_T);
static uint32_t bmp280_compensate_P_int32(int32_t adc_P);

/* Function definitions */
void bmp_init(GPIO_TypeDef *port, uint16_t pin) {
  uint8_t _config = BMP_ODR << 5 | BMP_FILTER << 2 | BMP_SPI;
  uint8_t _ctrl_meas = BMP_TEMP_OS << 5 | BMP_PRES_OS << 2 | BMP_POWER_MODE;
  
  bmp_cs.port = port;
  bmp_cs.pin = pin;

  //HAL_SPI_Transmit_DMA(&hspi1, spi_tx_buffer, SPI_BUFFER_SIZE);
  //HAL_SPI_Receive_DMA(&hspi1, spi_rx_buffer, SPI_BUFFER_SIZE);
  
  bmp_get_calibration();  

  spi_write_address(bmp_cs, BMP_CONFIG & ~0x80, &_config, 1);
  spi_write_address(bmp_cs, BMP_CTRL_MEAS & ~0x80, &_ctrl_meas, 1);  
}

int32_t bmp_get_temperature(void) {
  spi_read_address(bmp_cs, BMP_TEMP_MSB, spi_rx_buffer, 3);
  uint32_t uncompensated_temp = (uint32_t)(spi_rx_buffer[0] << 12) | 
                                (uint32_t)(spi_rx_buffer[1] << 4) | 
                                (uint32_t)(spi_rx_buffer[2] >> 4); 
  temperature = bmp280_compensate_T_int32(uncompensated_temp);
  return temperature;
}

uint32_t bmp_get_pressure(void) {
  spi_read_address(bmp_cs, BMP_PRES_MSB, spi_rx_buffer, 3);
  uint32_t uncompensated_pressure = (uint32_t)(spi_rx_buffer[0] << 12) | 
                                    (uint32_t)(spi_rx_buffer[1] << 4) | 
                                    (uint32_t)(spi_rx_buffer[2] >> 4); 
  pressure = bmp280_compensate_P_int32(uncompensated_pressure);
  return pressure;
}

uint32_t bmp_get_altitude(void) {
  uint8_t n = (MAX_PRESSURE - pressure) / PRESSURE_STEP;
  uint16_t f = (MAX_PRESSURE - pressure) % PRESSURE_STEP;
  altitude = altitude_lut[n] + f * (altitude_lut[n+1] - altitude_lut[n]) / PRESSURE_STEP;  
  return altitude;
}

void bmp_get_calibration(void) {	
  spi_read_address(bmp_cs, BMP_CALIB00, spi_rx_buffer, 24);
  cal.dig_T1 = ((uint16_t)spi_rx_buffer[1] << 8) | spi_rx_buffer[0];
  cal.dig_T2 = ((int16_t)spi_rx_buffer[3] << 8) | spi_rx_buffer[2];
  cal.dig_T3 = ((int16_t)spi_rx_buffer[5] << 8) | spi_rx_buffer[4];
  cal.dig_P1 = ((uint16_t)spi_rx_buffer[7] << 8) | spi_rx_buffer[6];
  cal.dig_P2 = ((int16_t)spi_rx_buffer[9] << 8) | spi_rx_buffer[8];
  cal.dig_P3 = ((int16_t)spi_rx_buffer[11] << 8) | spi_rx_buffer[10];
  cal.dig_P4 = ((int16_t)spi_rx_buffer[13] << 8) | spi_rx_buffer[12];
  cal.dig_P5 = ((int16_t)spi_rx_buffer[15] << 8) | spi_rx_buffer[14];
  cal.dig_P6 = ((int16_t)spi_rx_buffer[17] << 8) | spi_rx_buffer[16];
  cal.dig_P7 = ((int16_t)spi_rx_buffer[19] << 8) | spi_rx_buffer[18];
  cal.dig_P8 = ((int16_t)spi_rx_buffer[21] << 8) | spi_rx_buffer[20];
  cal.dig_P9 = ((int16_t)spi_rx_buffer[23] << 8) | spi_rx_buffer[22];
}

// START: from Bosch Sensortec BMP280 Data sheet : BST-BMP280-DS001-26 Revision_1.26_102021
// ------>8-------------------------------------------------------------------------
// Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
// t_fine carries fine temperature as global value
int32_t t_fine;
static int32_t bmp280_compensate_T_int32(int32_t adc_T)
{
    int32_t var1, var2, T;
    var1 = ((((adc_T>>3) - ((int32_t)cal.dig_T1<<1))) * ((int32_t)cal.dig_T2)) >> 11;
    var2 = (((((adc_T>>4) - ((int32_t)cal.dig_T1)) * ((adc_T>>4) - ((int32_t)cal.dig_T1))) >> 12) *
            ((int32_t)cal.dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

// Returns pressure in Pa as unsigned 32 bit integer. Output value of “96386” equals 96386 Pa = 963.86 hPa
static uint32_t bmp280_compensate_P_int32(int32_t adc_P)
{
    int32_t var1, var2;
    uint32_t p;
    var1 = (((int32_t)t_fine)>>1) - (int32_t)64000;
    var2 = (((var1>>2) * (var1>>2)) >> 11 ) * ((int32_t)cal.dig_P6);
    var2 = var2 + ((var1*((int32_t)cal.dig_P5))<<1);
    var2 = (var2>>2)+(((int32_t)cal.dig_P4)<<16);
    var1 = (((cal.dig_P3 * (((var1>>2) * (var1>>2)) >> 13 )) >> 3) + ((((int32_t)cal.dig_P2) * var1)>>1))>>18;
    var1 =((((32768+var1))*((int32_t)cal.dig_P1))>>15);
    if (var1 == 0)
    {
        return 0; // avoid exception caused by division by zero
    }
    p = (((uint32_t)(((int32_t)1048576)-adc_P)-(var2>>12)))*3125;
    if (p < 0x80000000)
    {
        p = (p << 1) / ((uint32_t)var1);
    }
    else
    {
        p = (p / (uint32_t)var1) * 2;
    }
    var1 = (((int32_t)cal.dig_P9) * ((int32_t)(((p>>3) * (p>>3))>>13)))>>12;
    var2 = (((int32_t)(p>>2)) * ((int32_t)cal.dig_P8))>>13;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + cal.dig_P7) >> 4));
    return p;
}
// ------>8-------------------------------------------------------------------------
// END: from Bosch Sensortec BMP280 Data sheet : BST-BMP280-DS001-26 Revision_1.26_102021
