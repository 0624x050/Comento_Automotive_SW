/*
 * PMIC.c
 *
 *  Created on: 25.08.15
 *  Updated on: 25.08.24
 *      Author: 김나윤
 */

#include "PMIC.h"


// 전역 변수
PMIC_Faults_t g_pmicFaults;
volatile uint8_t g_pmicTransferDone = 0;

// === I2C DMA Read ===
HAL_StatusTypeDef PMIC_ReadReg_DMA(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data, uint16_t len)
{
    g_pmicTransferDone = 0;
    return HAL_I2C_Mem_Read_DMA(hi2c,
                                I2C_SLAVE_ADDRESS,
                                reg,
                                I2C_MEMADD_SIZE_8BIT,
                                data,
                                len);
}

// === I2C DMA Write ===
HAL_StatusTypeDef PMIC_WriteReg_DMA(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data, uint16_t len)
{
    g_pmicTransferDone = 0;
    return HAL_I2C_Mem_Write_DMA(hi2c,
                                 I2C_SLAVE_ADDRESS,
                                 reg,
                                 I2C_MEMADD_SIZE_8BIT,
                                 data,
                                 len);
}

// === Fault 읽기 (DMA) ===
HAL_StatusTypeDef PMIC_ReadAllFaults_DMA(I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef st;	// HAL 함수 변환 상태 체크용
    uint8_t v;				// 각 레지스터 값을 일시적으로 저장 할 버퍼

    // PG(Power Good) 상태 읽기
    st = PMIC_ReadReg_DMA(hi2c, PMIC_REG_PG_FILT, &v, 1);	// PMIC_REG_RG_FILT(0x06) 읽기 요청
    if(st != HAL_OK) return st;								// HAL_OK 함수 아니면 종료
    while(!g_pmicTransferDone); 							// DMA 완료 대기, 완료하면 g_pmicTransferDone = 1
    g_pmicFaults.pg_filt.bit = v;							// v값을 g_pmicFaults.pg_filt.raw에 저장

    // UV/OV
    st = PMIC_ReadReg_DMA(hi2c, PMIC_REG_UV_OV, &v, 1);		// PMIC_REG_UV_OV(0x07) 읽기 요청
    if(st != HAL_OK) return st;								// HAL_OK 함수 아니면 종료
    while(!g_pmicTransferDone);								// DMA 완료 대기, 완료하면 g_pmicTransferDone = 1
    g_pmicFaults.uv_ov.bit = v;								// v값을 g_pmicFaults.uv_ov.raw에 저장

    // OC
    st = PMIC_ReadReg_DMA(hi2c, PMIC_REG_OC_OCWAR, &v, 1);	// PMIC_REG_OC_OCWAR(0x08) 읽기 요청
    if(st != HAL_OK) return st;								// HAL_OK 함수 아니면 종료
    while(!g_pmicTransferDone);								// DMA 완료 대기, 완료하면 g_pmicTransferDone = 1
    g_pmicFaults.oc_warn.bit = v;							// v값을 g_pmicFaults.oc_warn.raw에 저장

    return HAL_OK;											// 모든 레지스터 읽기 성공하여 반환
}

// === Fault 판별 ===
uint8_t PMIC_HasVoltageFault(PMIC_Faults_t *f) { return (f->uv_ov.bit != 0); }
uint8_t PMIC_HasCurrentFault(PMIC_Faults_t *f) { return (f->oc_warn.bit != 0); }
uint8_t PMIC_HasTempFault(PMIC_Faults_t *f)    { return 0; } // Thermal Fault 미구현


// === ADC 읽기 (예시) ===
uint16_t ReadVinADC(void)
{
    // TODO: 실제 ADC 채널 읽도록 구현
    // return HAL_ADC_GetValue(&hadc1);
    return 4000; // 예시값
}

// === Vin 모니터링 Task ===
void PMIC_VinMonitorTask(void)
{
    uint16_t vin = ReadVinADC();
    if(vin < UVLO_THRESHOLD)
        HAL_GPIO_WritePin(PMIC_ENABLE_GPIO_Port, PMIC_ENABLE_Pin, GPIO_PIN_RESET);
    else
        HAL_GPIO_WritePin(PMIC_ENABLE_GPIO_Port, PMIC_ENABLE_Pin, GPIO_PIN_SET);
}


// === PMIC 초기화 (DMA 버전) ===
HAL_StatusTypeDef PMIC_Init_DMA(I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef st;
    uint8_t val;

    // 1. PG 핀 및 Vin 확인 (UVLO)
    while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET || ReadVinADC() < UVLO_THRESHOLD)
    {
        HAL_GPIO_WritePin(PMIC_ENABLE_GPIO_Port, PMIC_ENABLE_Pin, GPIO_PIN_RESET);
        HAL_Delay(1);
    }
    HAL_GPIO_WritePin(PMIC_ENABLE_GPIO_Port, PMIC_ENABLE_Pin, GPIO_PIN_SET);

    // 2. Default 값 확인 (BuckA VOUT)
    st = PMIC_ReadReg_DMA(hi2c, PMIC_REG_VOUT_BUCKA, &val, 1);
    if(st != HAL_OK) return st;
    while(!g_pmicTransferDone);
    if(val != 0x00) return HAL_ERROR;

    // 3. VOUT 설정 (예: 1.2V)
    uint8_t vout_val = 0x24;  // 데이터시트 확인 필요
    st = PMIC_WriteReg_DMA(hi2c, PMIC_REG_VOUT_BUCKA, &vout_val, 1);
    if(st != HAL_OK) return st;
    while(!g_pmicTransferDone);

    // 4. Enable BuckA
    uint8_t enable_val = 0x40;
    st = PMIC_WriteReg_DMA(hi2c, PMIC_REG_ENABLE, &enable_val, 1);
    if(st != HAL_OK) return st;
    while(!g_pmicTransferDone);

    // 5. Fault 체크
    st = PMIC_ReadAllFaults_DMA(hi2c);
    if(st != HAL_OK) return st;
    if (PMIC_HasVoltageFault(&g_pmicFaults) || PMIC_HasCurrentFault(&g_pmicFaults))
        return HAL_ERROR;

    return HAL_OK;
}


// === HAL 콜백 구현 ===
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) { g_pmicTransferDone = 1; }
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) { g_pmicTransferDone = 1; }
