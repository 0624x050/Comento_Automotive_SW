/*
 * PMIC.c
 *
 *  Created on: 25.08.15
 *  Updated on: 25.08.20
 *      Author: 김나윤
 */

#include "PMIC.h"


HAL_StatusTypeDef PMIC_ReadReg_DMA(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data, uint16_t len)
{
    return HAL_I2C_Mem_Read_DMA(hi2c,
                                I2C_SLAVE_ADDRESS,
                                reg,
                                I2C_MEMADD_SIZE_8BIT,
                                data,
                                len);
}

HAL_StatusTypeDef PMIC_WriteReg_DMA(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data, uint16_t len)
{
    return HAL_I2C_Mem_Write_DMA(hi2c,
                                 I2C_SLAVE_ADDRESS,
                                 reg,
                                 I2C_MEMADD_SIZE_8BIT,
                                 data,
                                 len);
}

HAL_StatusTypeDef PMIC_ReadAllFaults(I2C_HandleTypeDef *hi2c, PMIC_Faults_t *faults)
{
    HAL_StatusTypeDef st;
    uint8_t v;

    // 0x06 : PG raw/filter 상태
    st = PMIC_ReadReg_DMA(hi2c, PMIC_BUCK_PG_FILT, &v, 1);
    if(st != HAL_OK) return st;
    faults->pg_filt.raw = v;

    // 0x07 : UV/OV 상태
    st = PMIC_ReadReg_DMA(hi2c, PMIC_BUCK_UV_OV, &v, 1);
    if(st != HAL_OK) return st;
    faults->uv_ov.raw = v;

    // 0x08 : OC warning / OC 상태
    st = PMIC_ReadReg_DMA(hi2c, PMIC_BUCK_OC_OCwar, &v, 1);
    if(st != HAL_OK) return st;
    faults->oc_warn.raw = v;

    return HAL_OK;
}

uint8_t PMIC_HasVoltageFault(PMIC_Faults_t *f)
{
    // UV 또는 OV 한 개라도 서면 1
    return (f->uv_ov.raw != 0);
}

uint8_t PMIC_HasCurrentFault(PMIC_Faults_t *f)
{
    // OC warn 또는 OC 한 개라도 서면 1
    return (f->oc_warn.raw != 0);
}

uint8_t PMIC_HasTempFault(PMIC_Faults_t *f)
{
    // System에 온도 플래그가 따로 있으면 거기서 체크.
    // 현재 네 헤더엔 system 필드 값/비트 정의가 없으니 0 반환.
    return 0;
}
