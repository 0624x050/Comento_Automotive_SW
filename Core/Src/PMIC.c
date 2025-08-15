/*
 * PMIC.c
 *
 *  Created on: Aug 15, 2025
 *      Author: 김나윤
 */

#include "PMIC.h"
#include <string.h>


/**
 * @brief  16-bit Telemetry 레지스터 읽기
 * @param  hi2c     HAL I2C 핸들
 * @param  reg      레지스터 주소
 * @param  value    읽은 값 저장 변수 (16-bit)
 * @retval HAL_StatusTypeDef
 */
static HAL_StatusTypeDef PMIC_ReadTelemetry16(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t *value)
{
    uint8_t buf[2];
    HAL_StatusTypeDef ret;

    ret = HAL_I2C_Mem_Read(hi2c, I2C_SLAVE_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, buf, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK) return ret;

    *value = (buf[0] << 8) | buf[1]; // MSB first
    return HAL_OK;
}

/**
 * @brief  전압(mV) 읽기
 * @param  hi2c   HAL I2C 핸들
 * @param  voltage_mV 읽은 전압 (mV 단위)
 */
HAL_StatusTypeDef PMIC_ReadVoltage(I2C_HandleTypeDef *hi2c, float *voltage_mV)
{
    uint16_t raw;
    HAL_StatusTypeDef ret = PMIC_ReadTelemetry16(hi2c, PMIC_REG_READ_VOUT, &raw);
    if (ret != HAL_OK) return ret;

    // 데이터시트 변환 공식 예시: VOUT = raw * 0.002 (단위: V)
    *voltage_mV = raw * 2.0f; // 0.002V -> mV 변환
    return HAL_OK;
}

/**
 * @brief  전류(mA) 읽기
 * @param  hi2c   HAL I2C 핸들
 * @param  current_mA 읽은 전류 (mA 단위)
 */
HAL_StatusTypeDef PMIC_ReadCurrent(I2C_HandleTypeDef *hi2c, float *current_mA)
{
    uint16_t raw;
    HAL_StatusTypeDef ret = PMIC_ReadTelemetry16(hi2c, PMIC_REG_READ_IOUT, &raw);
    if (ret != HAL_OK) return ret;

    // 데이터시트 변환 공식 예시: IOUT = raw * 0.25 (단위: A)
    *current_mA = raw * 250.0f; // A -> mA
    return HAL_OK;
}

/**
 * @brief  온도(°C) 읽기
 * @param  hi2c   HAL I2C 핸들
 * @param  temp_C 읽은 온도 (섭씨)
 */
HAL_StatusTypeDef PMIC_ReadTemperature(I2C_HandleTypeDef *hi2c, float *temp_C)
{
    uint16_t raw;
    HAL_StatusTypeDef ret = PMIC_ReadTelemetry16(hi2c, PMIC_REG_READ_TEMPERATURE, &raw);
    if (ret != HAL_OK) return ret;

    // 데이터시트 변환 공식 예시: TEMP = raw * 0.125 (단위: °C)
    *temp_C = raw * 0.125f;
    return HAL_OK;
}


/**
 * @brief  단일 Fault 레지스터 읽기
 * @param  hi2c   HAL I2C 핸들
 * @param  reg    읽을 레지스터 (PMIC_FaultRegister_t)
 * @param  data   읽은 값 저장할 변수 포인터
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef PMIC_ReadFaultRegister(I2C_HandleTypeDef *hi2c,
                                         PMIC_FaultRegister_t reg,
                                         uint8_t *data)
{
    return HAL_I2C_Mem_Read(hi2c, I2C_SLAVE_ADDRESS,
    						reg, I2C_MEMADD_SIZE_8BIT,
							data, 1,
							HAL_MAX_DELAY);
}

/**
 * @brief  모든 Fault 레지스터(0x07, 0x08, 0x09) 읽기
 * @param  hi2c    HAL I2C 핸들
 * @param  faults  읽은 값 저장 구조체 포인터
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef PMIC_ReadAllFaults(I2C_HandleTypeDef *hi2c,
                                     PMIC_Faults_t *faults)
{
    HAL_StatusTypeDef ret;

    // UV/OV (0x07)
    ret = PMIC_ReadFaultRegister(hi2c, PMIC_REG_UV_OV, &faults->uv_ov);
    if (ret != HAL_OK) return ret;

    // OC/OC_WARNING (0x08)
    ret = PMIC_ReadFaultRegister(hi2c, PMIC_REG_OC_War, &faults->oc_warn);
    if (ret != HAL_OK) return ret;

    // SYSTEM (0x09)
    ret = PMIC_ReadFaultRegister(hi2c, PMIC_REG_SYSTEM, &faults->system);
    if (ret != HAL_OK) return ret;

    return HAL_OK;
}

/**
 * @brief  전압 Fault 여부 확인
 * @param  faults  PMIC_Faults_t 구조체
 * @retval 1이면 Fault 존재, 0이면 정상
 */
uint8_t PMIC_HasVoltageFault(const PMIC_Faults_t *faults)
{
    return (faults->uv_ov & (PMIC_UV_A_Msk | PMIC_UV_B_Msk | PMIC_UV_C_Msk | PMIC_UV_D_Msk)) ||
           (faults->uv_ov & (PMIC_OV_A_Msk | PMIC_OV_B_Msk | PMIC_OV_C_Msk | PMIC_OV_D_Msk));
}

/**
 * @brief  전류 Fault 여부 확인
 * @param  faults  PMIC_Faults_t 구조체
 * @retval 1이면 Fault 존재, 0이면 정상
 */
uint8_t PMIC_HasCurrentFault(const PMIC_Faults_t *faults)
{
    return (faults->oc_warn & (PMIC_OC_A_Msk | PMIC_OC_B_Msk | PMIC_OC_C_Msk | PMIC_OC_D_Msk));
}

/**
 * @brief  온도 Fault 여부 확인
 * @param  faults  PMIC_Faults_t 구조체
 * @retval 1이면 Fault 존재, 0이면 정상
 */
uint8_t PMIC_HasTempFault(const PMIC_Faults_t *faults)
{
    return (faults->system & (PMIC_SYS_TEMP_WARN_Msk | PMIC_SYS_TEMP_SHDN_Msk));
}
