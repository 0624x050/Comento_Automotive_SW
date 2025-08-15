/*
 * PMIC.h
 *
 *  Created on: Aug 13, 2025
 *      Author: 김나윤
 */

#ifndef INC_PMIC_H_
#define INC_PMIC_H_


#include "stm32f4xx_hal.h"


#define MP5475_I2C_ADDR_7B     (0x60)                     // 0xC0, MP5475GU Address (5p)
#define I2C_SLAVE_ADDRESS      (MP5475_I2C_ADDR_7B << 1)  // HAL용 8-bit

// Telemetry Register (p.44~45)
#define PMIC_REG_READ_VOUT        0x8B  // READ_VOUT
#define PMIC_REG_READ_IOUT        0x8C  // READ_IOUT
#define PMIC_REG_READ_TEMPERATURE 0x8D  // READ_TEMPERATURE_1



// Register Address
typedef enum {
	PMIC_REG_FSM_PWR = 0X05,  // Control certain system configuration (42p)
	PMIC_REG_PG      = 0X06,  // Buck's output and filter's output (42p)
    PMIC_REG_UV_OV   = 0x07,  // Under voltage, Over voltage (42p)
    PMIC_REG_OC_War  = 0x08,  // Over current, Over current warning (42p)
	PMIC_REG_SYSTEM  = 0x09   // Error status (43p)
} PMIC_FaultRegister_t;


// 0x07: [7:4]=UV(A~D), [3:0]=OV(A~D) (43p)
#define PMIC_UV_A_Msk   (1u << 7)
#define PMIC_UV_B_Msk   (1u << 6)
#define PMIC_UV_C_Msk   (1u << 5)
#define PMIC_UV_D_Msk   (1u << 4)
#define PMIC_OV_A_Msk   (1u << 3)
#define PMIC_OV_B_Msk   (1u << 2)
#define PMIC_OV_C_Msk   (1u << 1)
#define PMIC_OV_D_Msk   (1u << 0)

// 0x08: [7:4]=OC(A~D), [3:0]=OC_WARNING(A~D) (43p)
#define PMIC_OC_A_Msk   (1u << 7)
#define PMIC_OC_B_Msk   (1u << 6)
#define PMIC_OC_C_Msk   (1u << 5)
#define PMIC_OC_D_Msk   (1u << 4)
#define PMIC_OCW_A_Msk  (1u << 3)
#define PMIC_OCW_B_Msk  (1u << 2)
#define PMIC_OCW_C_Msk  (1u << 1)
#define PMIC_OCW_D_Msk  (1u << 0)

/* 0x09: SYSTEM (44p)
 * bit6=1.8VLDO_FAULT, bit5=1.1VLDO_FAULT, bit4=VR_FAULT,
 * bit3=VBULK_OV, bit2=VDRV_OV, bit1=PMIC_HIGH_TEMP_WARNING,
 * bit0=PMIC_HIGH_TEMP_SHUTDOWN
 */
#define PMIC_SYS_1V8LDO_FAULT_Msk       (1u << 6)
#define PMIC_SYS_1V1LDO_FAULT_Msk       (1u << 5)
#define PMIC_SYS_VR_FAULT_Msk           (1u << 4)
#define PMIC_SYS_VBULK_OV_Msk           (1u << 3)
#define PMIC_SYS_VDRV_OV_Msk            (1u << 2)
#define PMIC_SYS_TEMP_WARN_Msk          (1u << 1)
#define PMIC_SYS_TEMP_SHDN_Msk          (1u << 0)

// Data
typedef union
{
    uint8_t raw[8];

    struct
    {
        uint16_t voltage;    // [0-1]
        uint16_t current;    // [2-3]
        uint16_t temp;       // [4-5]
        uint8_t  faultFlags; // [6]
        uint8_t  reserved;   // [7]
    } parsed;

    struct
    {
        uint8_t byte0;
        uint8_t byte1;
        uint8_t byte2;
        uint8_t byte3;
        uint8_t byte4;
        uint8_t byte5;

        union
        {
            uint8_t allFlags;
            struct
            {
                uint8_t voltageFault : 1;
                uint8_t currentFault : 1;
                uint8_t tempFault    : 1;
                uint8_t reservedBits : 5;
            };
        } fault;

        uint8_t reserved;
    } bitfield;

} MP5475_Data_t;

/* 모든 Fault를 한 번에 읽어 담는 구조체 */
typedef struct
{
    uint8_t uv_ov;     // Reg 0x07
    uint8_t oc_warn;   // Reg 0x08
    uint8_t system;    // Reg 0x09
} PMIC_Faults_t;

/* =========================
 * 함수 프로토타입
 * ========================= */
HAL_StatusTypeDef PMIC_ReadFaultRegister(I2C_HandleTypeDef *hi2c, PMIC_FaultRegister_t reg, uint8_t *data);
HAL_StatusTypeDef PMIC_ReadAllFaults(I2C_HandleTypeDef *hi2c, PMIC_Faults_t *faults);
#endif /* INC_PMIC_H_ */
