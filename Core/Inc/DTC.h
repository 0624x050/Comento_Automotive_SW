/*
 * DTC.h
 *
 *  Created on: Aug 13, 2025
 *      Author: 김나윤
 */

#ifndef INC_DTC_H_
#define INC_DTC_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ===== UDS 기본 ===== */
#define UDS_SVC_CLEAR_DIAG_INFO            0x14u
#define UDS_SVC_READ_DTC_INFO              0x19u

/* ReadDTCInformation SubFunctions (Softing 포스터 참조) */
#define UDS_RDI_REPORT_NUM_BY_STATUS_MASK  0x01u
#define UDS_RDI_REPORT_DTC_BY_STATUS_MASK  0x02u

/* CAN ID (예시: 파워트레인 기본) */
#define UDS_REQ_CANID                      0x7E0u
#define UDS_RES_CANID                      0x7E8u

/* DTC 포맷: ISO 14229 3-byte DTC + 1-byte status */
typedef struct {
    uint8_t dtc[3];        // e.g. 0x01,0x23,0x45
    uint8_t status;        // UDS status availability mask bits
} DTC_Record_t;

/* 단일 DTC 스토리지(EEPROM) */
typedef struct {
    DTC_Record_t rec;
    uint32_t     timestamp_ms; // 선택: 저장 시각
    uint32_t     crc32;        // 간단 무결성(옵션)
} DTC_Entry_t;

/* TJA1051 제어 핀(보드에 맞게 주입) */
typedef struct {
    GPIO_TypeDef* S_Port;  uint16_t S_Pin;   // S=LOW: Normal, HIGH: Silent (p.5)
    GPIO_TypeDef* EN_Port; uint16_t EN_Pin;  // T/E 버전만 사용 (Off mode)
} TJA1051_IO_t;

/* 초기화/전송에 필요한 핸들 */
typedef struct {
    CAN_HandleTypeDef*  hcan;
    SPI_HandleTypeDef*  hspi;       // 25LC256
    TJA1051_IO_t        transceiver;
    uint8_t             eeprom_cs_active_low; // CS 라인은 보드 HAL 레벨에서 처리 가정
} DTC_Ctx_t;

/* ===== API ===== */
HAL_StatusTypeDef DTC_Init(DTC_Ctx_t* ctx);
HAL_StatusTypeDef DTC_SetTransceiverNormal(DTC_Ctx_t* ctx);
HAL_StatusTypeDef DTC_SetTransceiverSilent(DTC_Ctx_t* ctx);

/* UDS - Clear DTC (0x14)  : dtc==NULL이면 전체 삭제 */
HAL_StatusTypeDef DTC_UDS_Clear(DTC_Ctx_t* ctx, const uint8_t dtc3[3]);

/* UDS - ReadDTCInformation(0x19/0x02): statusMask로 필터, 최대 n개 파싱 */
HAL_StatusTypeDef DTC_UDS_ReadByStatusMask(DTC_Ctx_t* ctx,
                                           uint8_t statusMask,
                                           DTC_Record_t* outList,
                                           uint8_t* inoutCount);

/* EEPROM 연동: 단일 DTC 저장/로드 */
HAL_StatusTypeDef DTC_SaveToEEPROM(DTC_Ctx_t* ctx, const DTC_Entry_t* e);
HAL_StatusTypeDef DTC_LoadFromEEPROM(DTC_Ctx_t* ctx, DTC_Entry_t* e);

/* 유틸 */
uint32_t DTC_CalcCRC32(const void* data, uint32_t len);

#endif /* INC_DTC_H_ */
