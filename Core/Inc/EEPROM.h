/*
 * EEPROM.h
 *
 *  Created on: Aug 13, 2025
 *      Author: 김나윤
 */

#ifndef INC_EEPROM_H_
#define INC_EEPROM_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* 25LC256 Command Set (Datasheet p.6~7) */
#define EEPROM_CMD_READ    0x03  // Read data from memory
#define EEPROM_CMD_WRITE   0x02  // Write data to memory
#define EEPROM_CMD_WREN    0x06  // Set Write Enable Latch
#define EEPROM_CMD_WRDI    0x04  // Reset Write Enable Latch
#define EEPROM_CMD_RDSR    0x05  // Read Status Register
#define EEPROM_CMD_WRSR    0x01  // Write Status Register

#define EEPROM_PAGE_SIZE   64     // 64 bytes per page
#define EEPROM_SIZE_BYTES  32768  // 32KB total

/* Status Register Bit (Datasheet p.7) */
#define EEPROM_SR_WIP      (1 << 0)  // Write-In-Progress
#define EEPROM_SR_WEL      (1 << 1)  // Write Enable Latch

/* Function Prototypes */
HAL_StatusTypeDef EEPROM_WriteEnable(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef EEPROM_ReadStatus(SPI_HandleTypeDef *hspi, uint8_t *status);
HAL_StatusTypeDef EEPROM_WaitForWrite(SPI_HandleTypeDef *hspi);

HAL_StatusTypeDef EEPROM_WriteData(SPI_HandleTypeDef *hspi, uint16_t addr, uint8_t *data, uint16_t len);
HAL_StatusTypeDef EEPROM_ReadData(SPI_HandleTypeDef *hspi, uint16_t addr, uint8_t *data, uint16_t len);

#endif /* INC_EEPROM_H_ */

