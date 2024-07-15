#include "bmp280.h"
#include "gpio.h"

typdef enum (
  ID = 0xD0,
  RESET = 0xE0,
  STATUS = 0xF3,
  CTRL_MEAS = 0xF4,
  CONFIG = 0xF5,
  PRES_MSB = 0xF7,
  PRES_LSB = 0xF8,
  PRES_XLSB = 0xF9,
  TEMP_MSB = 0xFA,
  TEMP_LSB = 0xFB,
  TEMP_XLSB = 0xFC,
  CALIB00 = 0x88
) bmp_280_register_t;

uint8_t spiRxBuffer[26];
uint8_t spiTxBuffer[2];
    
void BMP280_InitiateTransfer(uint8_t rx_len) {
  HAL_GPIO_WritePin(BMP280_CS_Port, BMP280_CS_Pin, GPIO_PIN_RESET);
}



void BMP280_CompleteTransfer() {
  HAL_GPIO_WritePin(BMP280_CS_Port, BMP280_CS_Pin, GPIO_PIN_SET);
}

void BMP280_SetupTransfer(uint8_t *tx_buffer, uint8_t* rx_buffer,  uint8_t rx_bytes) {
  
}


void BMP280_Init() {

}


