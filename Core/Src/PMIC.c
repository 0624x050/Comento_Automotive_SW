/*
 * PMIC.c
 *
 *  Created on: Aug 15, 2025
 *      Author: 김나윤
 */

#include "PMIC.h"
#include <string.h>

/* 단일 Fault 레지스터 읽기 */
HAL_StatusTypeDef PMIC_ReadFaultRegister(I2C_HandleTypeDef *hi2c,
                                         PMIC_Register_t reg,
                                         uint8_t *data)
{
    return HAL_I2C_Mem_Read(hi2c,
    						MP5475_I2C_ADDR_7bit,
                            reg,
                            I2C_MEMADD_SIZE_8BIT,
                            data,
                            1,
                            HAL_MAX_DELAY);
}

/* Fault 전체 읽기 */
HAL_StatusTypeDef PMIC_ReadAllFaults(I2C_HandleTypeDef *hi2c,
                                     PMIC_Faults_t *faults)
{
    if (PMIC_ReadFaultRegister(hi2c, PMIC_REG_UV_OV, &faults->uv_ov) != HAL_OK) return HAL_ERROR;
    if (PMIC_ReadFaultRegister(hi2c, PMIC_REG_OC_WAR, &faults->oc_warn) != HAL_OK) return HAL_ERROR;
    if (PMIC_ReadFaultRegister(hi2c, PMIC_REG_SYSTEM, &faults->system) != HAL_OK) return HAL_ERROR;
    return HAL_OK;
}

/* Buck 출력 전압 설정
 * @param hi2c      I2C 핸들
 * @param buckReg   BUCKx VOUT 레지스터 (0x16~0x19)
 * @param voltage_mV 출력 전압 (mV 단위, 예: 1200.0f → 1.2V)
 * @return HAL_OK / HAL_ERROR
 */
HAL_StatusTypeDef PMIC_SetBuckVoltage(I2C_HandleTypeDef *hi2c, PMIC_Register_t buckReg, float voltage_mV)
{
    if (buckReg < PMIC_REG_BUCKA_VOUT || buckReg > PMIC_REG_BUCKD_VOUT)
        return HAL_ERROR;

    // 1 LSB = 10 mV
    uint8_t regValue = (uint8_t)(voltage_mV / 10.0f);

    return HAL_I2C_Mem_Write(hi2c,
                             I2C_SLAVE_ADDRESS,
                             (uint16_t)buckReg,
                             I2C_MEMADD_SIZE_8BIT,
                             &regValue,
                             1,
                             HAL_MAX_DELAY);
}


/* Fault 체크 함수 */
uint8_t PMIC_HasVoltageFault(const PMIC_Faults_t *faults)
{
    return (faults->uv_ov & (PMIC_UV_A_Msk | PMIC_UV_B_Msk |
                             PMIC_UV_C_Msk | PMIC_UV_D_Msk |
                             PMIC_OV_A_Msk | PMIC_OV_B_Msk |
                             PMIC_OV_C_Msk | PMIC_OV_D_Msk));
}

uint8_t PMIC_HasCurrentFault(const PMIC_Faults_t *faults)
{
    return (faults->oc_warn & (PMIC_OC_A_Msk | PMIC_OC_B_Msk |
                               PMIC_OC_C_Msk | PMIC_OC_D_Msk));
}

uint8_t PMIC_HasTempFault(const PMIC_Faults_t *faults)
{
    return (faults->system & (PMIC_SYS_TEMP_WARN_Msk | PMIC_SYS_TEMP_SHDN_Msk));
}
