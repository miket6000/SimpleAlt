/**
 ******************************************************************************
 * @file           : w25qxx.h
 * @brief          : Minimal W25Qxx Library Header
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022, 2023 Lars Boegild Thomsen <lbthomsen@gmail.com>
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#ifndef W25QXX_H_
#define W25QXX_H_

#include "gpio.h"

#ifdef DEBUGxxx
#define W25_DBG(...) printf(__VA_ARGS__);\
                     printf("\n")
#else
#define W25_DBG(...)
#endif

#define W25QXX_MANUFACTURER_GIGADEVICE 0xC8
#define W25QXX_MANUFACTURER_WINBOND 0xEF

#define W25QXX_DUMMY_BYTE         0xA5
#define W25QXX_GET_ID             0x9F
#define W25QXX_GET_UID            0x4B
#define W25QXX_READ_DATA          0x03
#define W25QXX_WRITE_ENABLE       0x06
#define W25QXX_PAGE_PROGRAM       0x02
#define W25QXX_SECTOR_ERASE	  0x20
#define W25QXX_CHIP_ERASE         0xc7
#define W25QXX_READ_REGISTER_1    0x05

typedef struct {
    SPI_HandleTypeDef *spiHandle;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    uint8_t manufacturer_id;
    uint16_t device_id;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t sector_size;
    uint32_t sectors_in_block;
    uint32_t page_size;
    uint32_t pages_in_sector;
} W25QXX_HandleTypeDef;

typedef enum {
    W25QXX_Ok,     // 0
    W25QXX_Err,    // 1
    W25QXX_Timeout // 2
} W25QXX_result_t;

void w25qxx_sleep(W25QXX_HandleTypeDef *w25qxx);
void w25qxx_wake(W25QXX_HandleTypeDef *w25qxx);

W25QXX_result_t w25qxx_init(W25QXX_HandleTypeDef *w25qxx, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
W25QXX_result_t w25qxx_read(W25QXX_HandleTypeDef *w25qxx, uint32_t address, uint8_t *buf, uint32_t len);
W25QXX_result_t w25qxx_write(W25QXX_HandleTypeDef *w25qxx, uint32_t address, uint8_t *buf, uint32_t len);
W25QXX_result_t w25qxx_erase(W25QXX_HandleTypeDef *w25qxx, uint32_t address, uint32_t len);
W25QXX_result_t w25qxx_chip_erase(W25QXX_HandleTypeDef *w25qxx);
uint32_t w25qxx_read_uid(W25QXX_HandleTypeDef *w25qxx);
#endif /* W25QXX_H_ */

