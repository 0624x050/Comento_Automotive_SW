/*
 * EEPROM.c
 *
 *  Created on: Aug 13, 2025
 *      Author: 김나윤
 */

#include "EEPROM.h"

/* ========================================
 * Write Enable (Datasheet p.6)
 * ======================================== */
HAL_StatusTypeDef EEPROM_WriteEnable(SPI_HandleTypeDef *hspi)
{
    uint8_t cmd = EEPROM_CMD_WREN;
    return HAL_SPI_Transmit(hspi, &cmd, 1, HAL_MAX_DELAY);
}

/* ========================================
 * Status Register Read (Datasheet p.7)
 * RDSR 명령어 이후 Dummy Byte를 전송해야 Status Register가 출력됨
 * ======================================== */
HAL_StatusTypeDef EEPROM_ReadStatus(SPI_HandleTypeDef *hspi, uint8_t *status)
{
    uint8_t tx[2] = { EEPROM_CMD_RDSR, 0xFF };
    uint8_t rx[2] = { 0 };

    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(hspi, tx, rx, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return ret;

    *status = rx[1];
    return HAL_OK;
}

/* ========================================
 * Write 완료 대기 (WIP 비트 클리어될 때까지)
 * ======================================== */
HAL_StatusTypeDef EEPROM_WaitForWrite(SPI_HandleTypeDef *hspi)
{
    uint8_t status = 0;
    do {
        EEPROM_ReadStatus(hspi, &status);
    } while (status & EEPROM_SR_WIP);
    return HAL_OK;
}

/* ========================================
 * 데이터 쓰기 (최대 1 Page)
 * - WREN → WRITE → Wait
 * ======================================== */
HAL_StatusTypeDef EEPROM_WriteData(SPI_HandleTypeDef *hspi, uint16_t addr, uint8_t *data, uint16_t len)
{
    if (len > EEPROM_PAGE_SIZE) return HAL_ERROR;

    EEPROM_WriteEnable(hspi);

    uint8_t tx[3];
    tx[0] = EEPROM_CMD_WRITE;
    tx[1] = (addr >> 8) & 0xFF;
    tx[2] = addr & 0xFF;

    /* 주소 전송 */
    HAL_StatusTypeDef ret = HAL_SPI_Transmit(hspi, tx, 3, HAL_MAX_DELAY);
    if (ret != HAL_OK) return ret;

    /* 데이터 전송 (Blocking) */
    ret = HAL_SPI_Transmit(hspi, data, len, HAL_MAX_DELAY);
    if (ret != HAL_OK) return ret;

    /* 쓰기 완료 대기 */
    return EEPROM_WaitForWrite(hspi);
}

/* ========================================
 * 데이터 읽기
 * - READ → Addr → Data
 * ======================================== */
HAL_StatusTypeDef EEPROM_ReadData(SPI_HandleTypeDef *hspi, uint16_t addr, uint8_t *data, uint16_t len)
{
    uint8_t tx[3];
    tx[0] = EEPROM_CMD_READ;
    tx[1] = (addr >> 8) & 0xFF;
    tx[2] = addr & 0xFF;

    /* READ 명령어 + 주소 전송 */
    HAL_StatusTypeDef ret = HAL_SPI_Transmit(hspi, tx, 3, HAL_MAX_DELAY);
    if (ret != HAL_OK) return ret;

    /* 데이터 수신 */
    return HAL_SPI_Receive(hspi, data, len, HAL_MAX_DELAY);
}

