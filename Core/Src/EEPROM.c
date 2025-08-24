/*
 * EEPROM.c
 *
 *  Created on: 25.08.13
 *  Updated on: 25.08.25
 *      Author: 김나윤
 */

#include "EEPROM.h"


/* DMA 완료 전역 변수 */
volatile uint8_t g_eepromTransferDone = 0;

/* Write Enable (DMA) */
HAL_StatusTypeDef EEPROM_WriteEnable_DMA(SPI_HandleTypeDef *hspi)
{
    uint8_t cmd = EEPROM_CMD_WREN;				// WREN 명령 보냄
    g_eepromTransferDone = 0;					// 전송 이전, 플래그 0 (초기화)
    return HAL_SPI_Transmit_DMA(hspi, &cmd, 1);	// 전송 완료, HAL 콜백에서 플래그 1
}

/* 데이터 쓰기 (DMA, 최대 1 Page) */
HAL_StatusTypeDef EEPROM_WriteData_DMA(SPI_HandleTypeDef *hspi, uint16_t addr, uint8_t *data, uint16_t len)
{
    if (len > EEPROM_PAGE_SIZE) return HAL_ERROR;

    /* WREN */
    HAL_StatusTypeDef st = EEPROM_WriteEnable_DMA(hspi);
    if(st != HAL_OK) return st;
    while(!g_eepromTransferDone);

    /* WRITE 명령 + 주소 */
    uint8_t tx[3];
    tx[0] = EEPROM_CMD_WRITE;
    tx[1] = (addr >> 8) & 0xFF;
    tx[2] = addr & 0xFF;

    g_eepromTransferDone = 0;
    st = HAL_SPI_Transmit_DMA(hspi, tx, 3);
    if(st != HAL_OK) return st;
    while(!g_eepromTransferDone);

    /* 데이터 전송 */
    g_eepromTransferDone = 0;
    st = HAL_SPI_Transmit_DMA(hspi, data, len);
    if(st != HAL_OK) return st;
    while(!g_eepromTransferDone);

    return HAL_OK;
}

/* 데이터 읽기 (DMA) */
HAL_StatusTypeDef EEPROM_ReadData_DMA(SPI_HandleTypeDef *hspi, uint16_t addr, uint8_t *data, uint16_t len)
{
    uint8_t tx[3];
    tx[0] = EEPROM_CMD_READ;
    tx[1] = (addr >> 8) & 0xFF;
    tx[2] = addr & 0xFF;

    g_eepromTransferDone = 0;
    HAL_StatusTypeDef st = HAL_SPI_Transmit_DMA(hspi, tx, 3);
    if(st != HAL_OK) return st;
    while(!g_eepromTransferDone);

    g_eepromTransferDone = 0;
    st = HAL_SPI_Receive_DMA(hspi, data, len);
    if(st != HAL_OK) return st;
    while(!g_eepromTransferDone);

    return HAL_OK;
}

/* HAL DMA 콜백 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) { g_eepromTransferDone = 1; }
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) { g_eepromTransferDone = 1; }
