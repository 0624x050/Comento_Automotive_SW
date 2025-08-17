/*
 * PMIC.h
 *
 *  Created on: Aug 13, 2025
 *      Author: 김나윤
 */

#ifndef INC_PMIC_H_
#define INC_PMIC_H_

#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif


#define MP5475_I2C_ADDR_7bit   (0x60)               	    // 0xC0, MP5475GU Address (5p)
#define I2C_SLAVE_ADDRESS      (MP5475_I2C_ADDR_7bit << 1)  // HAL용 8-bit


// Register Address
typedef enum {
	PMIC_REG_FSM_PWR = 0X05,  // Control certain system configuration (42p)
	PMIC_REG_PG      = 0X06,  // Buck's output and filter's output (42p)
    PMIC_REG_UV_OV   = 0x07,  // Under voltage, Over voltage (42p)
    PMIC_REG_OC_WAR  = 0x08,  // Over current, Over current warning (42p)
	PMIC_REG_SYSTEM  = 0x09,   // Error status (43p)

    /* VOUT 설정 레지스터 (Datasheet p.41) */
    PMIC_REG_BUCKA_VOUT = 0x16, // Buck A VOUT_COMMAND
    PMIC_REG_BUCKB_VOUT = 0x17, // Buck B VOUT_COMMAND
    PMIC_REG_BUCKC_VOUT = 0x18, // Buck C VOUT_COMMAND
    PMIC_REG_BUCKD_VOUT = 0x19  // Buck D VOUT_COMMAND

} PMIC_Register_t;


// 0x07: UV / OV Faults (43p)
#define PMIC_UV_A_Msk   (1u << 7)
#define PMIC_UV_B_Msk   (1u << 6)
#define PMIC_UV_C_Msk   (1u << 5)
#define PMIC_UV_D_Msk   (1u << 4)
#define PMIC_OV_A_Msk   (1u << 3)
#define PMIC_OV_B_Msk   (1u << 2)
#define PMIC_OV_C_Msk   (1u << 1)
#define PMIC_OV_D_Msk   (1u << 0)

// 0x08: OC / OC Warnings (43p)
#define PMIC_OC_A_Msk   (1u << 7)
#define PMIC_OC_B_Msk   (1u << 6)
#define PMIC_OC_C_Msk   (1u << 5)
#define PMIC_OC_D_Msk   (1u << 4)
#define PMIC_OCW_A_Msk  (1u << 3)
#define PMIC_OCW_B_Msk  (1u << 2)
#define PMIC_OCW_C_Msk  (1u << 1)
#define PMIC_OCW_D_Msk  (1u << 0)

// 0x09: SYSTEM Faults (44p)
#define PMIC_SYS_1V8LDO_FAULT_Msk       (1u << 6)
#define PMIC_SYS_1V1LDO_FAULT_Msk       (1u << 5)
#define PMIC_SYS_VR_FAULT_Msk           (1u << 4)
#define PMIC_SYS_VBULK_OV_Msk           (1u << 3)
#define PMIC_SYS_VDRV_OV_Msk            (1u << 2)
#define PMIC_SYS_TEMP_WARN_Msk          (1u << 1)
#define PMIC_SYS_TEMP_SHDN_Msk          (1u << 0)


// Fault 전체 읽기 구조체
typedef struct
{
    uint8_t uv_ov;     // Reg 0x07
    uint8_t oc_warn;   // Reg 0x08
    uint8_t system;    // Reg 0x09
} PMIC_Faults_t;

/* ================================
 * 함수 프로토타입
 * ================================ */
HAL_StatusTypeDef PMIC_ReadFaultRegister(I2C_HandleTypeDef *hi2c, PMIC_Register_t reg, uint8_t *data);
HAL_StatusTypeDef PMIC_ReadAllFaults(I2C_HandleTypeDef *hi2c, PMIC_Faults_t *faults);
HAL_StatusTypeDef PMIC_SetBuckVoltage(I2C_HandleTypeDef *hi2c, PMIC_Register_t buckReg, float voltage_mV);

uint8_t PMIC_HasVoltageFault(const PMIC_Faults_t *faults);
uint8_t PMIC_HasCurrentFault(const PMIC_Faults_t *faults);
uint8_t PMIC_HasTempFault(const PMIC_Faults_t *faults);

#ifdef __cplusplus
}
#endif
#endif /* INC_PMIC_H_ */
