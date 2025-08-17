/*
 * DTC.c
 *
 *  Created on: Aug 13, 2025
 *      Author: 김나윤
 */



#include "DTC.h"

/* ===== 내부 헬퍼 ===== */
static HAL_StatusTypeDef CAN_SendUDS(DTC_Ctx_t* ctx,
                                     uint32_t canid,
                                     const uint8_t* data, uint8_t len)
{
    CAN_TxHeaderTypeDef txh = {0};
    uint32_t mbox;
    txh.StdId = canid;
    txh.IDE   = CAN_ID_STD;
    txh.RTR   = CAN_RTR_DATA;
    txh.DLC   = len;                 // 단일프레임 전제(<=8)
    return HAL_CAN_AddTxMessage(ctx->hcan, &txh, (uint8_t*)data, &mbox);
}

static HAL_StatusTypeDef CAN_RecvUDS(DTC_Ctx_t* ctx,
                                     uint32_t expected_canid,
                                     uint8_t* out, uint8_t* outlen,
                                     uint32_t timeout_ms)
{
    uint32_t tick = HAL_GetTick();
    while ((HAL_GetTick() - tick) < timeout_ms) {
        CAN_RxHeaderTypeDef rxh;
        if (HAL_CAN_GetRxFifoFillLevel(ctx->hcan, CAN_RX_FIFO0) > 0) {
            if (HAL_CAN_GetRxMessage(ctx->hcan, CAN_RX_FIFO0, &rxh, out) == HAL_OK) {
                if (rxh.IDE == CAN_ID_STD && rxh.StdId == expected_canid) {
                    *outlen = rxh.DLC;
                    return HAL_OK;
                }
            }
        }
    }
    return HAL_TIMEOUT;
}

/* ===== TJA1051 제어 (데이터시트 p.5) ===== */
HAL_StatusTypeDef DTC_SetTransceiverNormal(DTC_Ctx_t* ctx)
{
    HAL_GPIO_WritePin(ctx->transceiver.S_Port, ctx->transceiver.S_Pin, GPIO_PIN_RESET); // S=LOW -> Normal
    return HAL_OK;
}
HAL_StatusTypeDef DTC_SetTransceiverSilent(DTC_Ctx_t* ctx)
{
    HAL_GPIO_WritePin(ctx->transceiver.S_Port, ctx->transceiver.S_Pin, GPIO_PIN_SET); // S=HIGH -> Silent
    return HAL_OK;
}

HAL_StatusTypeDef DTC_Init(DTC_Ctx_t* ctx)
{
    /* CAN 활성화 */
    if (HAL_CAN_Start(ctx->hcan) != HAL_OK) return HAL_ERROR;
    if (HAL_CAN_ActivateNotification(ctx->hcan,
        CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_TX_MAILBOX_EMPTY) != HAL_OK) return HAL_ERROR;

    /* TJA1051 Normal mode 권장 (S=LOW) – p.5 Operating modes */
    return DTC_SetTransceiverNormal(ctx);
}

/* ===== UDS: Clear DTC (0x14) =====
   Request: [0x14][DTC1][DTC2][DTC3]  (all 0xFF -> 모두 삭제)
   Positive Response: 0x54
*/
HAL_StatusTypeDef DTC_UDS_Clear(DTC_Ctx_t* ctx, const uint8_t dtc3[3])
{
    uint8_t req[4];
    uint8_t resp[8]; uint8_t rlen=0;

    req[0] = UDS_SVC_CLEAR_DIAG_INFO;
    if (dtc3) { req[1]=dtc3[0]; req[2]=dtc3[1]; req[3]=dtc3[2]; }
    else      { req[1]=0xFF; req[2]=0xFF; req[3]=0xFF; } // 전체 삭제 (Softing 포스터 예시) :contentReference[oaicite:5]{index=5}

    HAL_StatusTypeDef st = CAN_SendUDS(ctx, UDS_REQ_CANID, req, 4);
    if (st != HAL_OK) return st;

    st = CAN_RecvUDS(ctx, UDS_RES_CANID, resp, &rlen, 50);
    if (st != HAL_OK) return st;

    /* 기대 응답: 0x54 */
    return (rlen >= 1 && resp[0] == (UDS_SVC_CLEAR_DIAG_INFO | 0x40)) ? HAL_OK : HAL_ERROR;
}

/* ===== UDS: ReadDTCInformation (0x19/0x02) =====
   Request: [0x19][0x02][statusMask]
   Response: [0x59][0x02][statusAvailabilityMask][DTC(3)][Status]...  (단일 프레임 가정)
*/
HAL_StatusTypeDef DTC_UDS_ReadByStatusMask(DTC_Ctx_t* ctx,
                                           uint8_t statusMask,
                                           DTC_Record_t* outList,
                                           uint8_t* inoutCount)
{
    uint8_t req[3] = { UDS_SVC_READ_DTC_INFO, UDS_RDI_REPORT_DTC_BY_STATUS_MASK, statusMask };
    uint8_t resp[8]; uint8_t rlen=0;

    HAL_StatusTypeDef st = CAN_SendUDS(ctx, UDS_REQ_CANID, req, 3);
    if (st != HAL_OK) return st;

    st = CAN_RecvUDS(ctx, UDS_RES_CANID, resp, &rlen, 50);
    if (st != HAL_OK) return st;

    if (!(rlen >= 3 && resp[0] == (UDS_SVC_READ_DTC_INFO | 0x40) && resp[1] == UDS_RDI_REPORT_DTC_BY_STATUS_MASK))
        return HAL_ERROR;

    /* 파싱: resp[2]=statusAvailabilityMask, 이후 4바이트 단위(DTC3+status) */
    uint8_t avail = resp[2];
    (void)avail; // 필요 시 상위에서 사용
    uint8_t remain = rlen - 3;
    uint8_t n = 0;

    while (remain >= 4 && n < *inoutCount) {
        outList[n].dtc[0] = resp[3 + n*4 + 0];
        outList[n].dtc[1] = resp[3 + n*4 + 1];
        outList[n].dtc[2] = resp[3 + n*4 + 2];
        outList[n].status  = resp[3 + n*4 + 3];
        n++; remain -= 4;
    }
    *inoutCount = n;
    return HAL_OK;
}

/* ===== EEPROM: 25LC256 단일 레코드 저장/로드 =====
   - WREN -> WRITE -> WIP 폴링 (Status Register)
   - p.6: Status Register(WIP/WEL) 및 명령/타이밍 개요
*/
#define EE_INS_WREN   0x06u
#define EE_INS_WRDI   0x04u
#define EE_INS_RDSR   0x05u
#define EE_INS_WRITE  0x02u
#define EE_INS_READ   0x03u

static HAL_StatusTypeDef EE_ReadStatus(SPI_HandleTypeDef* h, uint8_t* sr)
{
    uint8_t tx[2] = { EE_INS_RDSR, 0xFF }, rx[2] = {0};
    HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(h, tx, rx, 2, HAL_MAX_DELAY);
    if (st != HAL_OK) return st;
    *sr = rx[1]; // 두 번째 바이트가 STATUS
    return HAL_OK;
}

static HAL_StatusTypeDef EE_WaitWriteComplete(SPI_HandleTypeDef* h)
{
    uint8_t sr;
    uint32_t t0 = HAL_GetTick();
    do {
        if (EE_ReadStatus(h, &sr) != HAL_OK) return HAL_ERROR;
        if ((sr & 0x01u) == 0) return HAL_OK; // WIP=0 (p.6)
    } while ((HAL_GetTick() - t0) < 10);      // 보수적 10ms
    return HAL_TIMEOUT;
}

HAL_StatusTypeDef DTC_SaveToEEPROM(DTC_Ctx_t* ctx, const DTC_Entry_t* e)
{
    uint8_t cmd[3 + sizeof(DTC_Entry_t)] = { EE_INS_WRITE, 0x00, 0x00 }; // 주소 0 고정 예시
    memcpy(&cmd[3], e, sizeof(DTC_Entry_t));

    uint8_t wren = EE_INS_WREN;
    if (HAL_SPI_Transmit(ctx->hspi, &wren, 1, HAL_MAX_DELAY) != HAL_OK) return HAL_ERROR;
    if (HAL_SPI_Transmit(ctx->hspi, cmd, 3 + sizeof(DTC_Entry_t), HAL_MAX_DELAY) != HAL_OK) return HAL_ERROR;
    return EE_WaitWriteComplete(ctx->hspi); // p.6 WIP 폴링
}

HAL_StatusTypeDef DTC_LoadFromEEPROM(DTC_Ctx_t* ctx, DTC_Entry_t* e)
{
    uint8_t cmd[3] = { EE_INS_READ, 0x00, 0x00 };
    uint8_t rx[sizeof(DTC_Entry_t)] = {0};

    if (HAL_SPI_Transmit(ctx->hspi, cmd, 3, HAL_MAX_DELAY) != HAL_OK) return HAL_ERROR;
    if (HAL_SPI_Receive(ctx->hspi, rx, sizeof(rx), HAL_MAX_DELAY) != HAL_OK) return HAL_ERROR;

    memcpy(e, rx, sizeof(DTC_Entry_t));
    /* CRC 검증 필요 시 상위에서 DTC_CalcCRC32 호출 */
    return HAL_OK;
}

