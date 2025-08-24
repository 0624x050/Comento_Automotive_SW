/*
 * PMIC.h
 *
 *  Created on: 25.08.13
 *  Updated on: 25.08.24
 *      Author: 김나윤
 */

// Guide : BUCK_A의 UV, OV, OC, OC warning 관리 및 전압레벨 조정

#ifndef INC_PMIC_H_
#define INC_PMIC_H_

#include "stm32f4xx_hal.h"


// === PMIC I2C 주소 ===
#define MP5475_I2C_ADDR_7bit   (0x60)               	    // 0xC0, MP5475GU Address (5p)
#define I2C_SLAVE_ADDRESS      (MP5475_I2C_ADDR_7bit << 1)  // HAL용 8-bit

// === UVLO 전압 ADC 임계치 (5p) ===
#define UVLO_THRESHOLD       3537	  // 12bit ADC


// === Register Map (40p) ===
typedef enum {
    PMIC_REG_VOUT_BUCKA   	= 0x20,   // 가정: BuckA VOUT Control
    PMIC_REG_ENABLE      	= 0x05,
    PMIC_BUCK_PG_FILT   	= 0x06,
    PMIC_BUCK_UV_OV  		= 0x07,
    PMIC_BUCK_OC_OCwar  	= 0x08
} PMIC_Register_t;


// === PMIC_BUCK_PG_FILT (40p) ===
typedef union {
    struct {
        uint8_t pwr_good_raw 	:1;  // D0
        uint8_t pwr_good 		:1;  // D1
        uint8_t fsm_state 		:2;  // D2-D3
    };
    uint8_t bit;
} PMIC_CTRL_STATE_t;


// === PMIC_BUCK_PG_FILT (40p) ===
typedef union {
    struct {
        uint8_t buckd_pg_raw 	:1;  // D0
        uint8_t buckc_pg_raw 	:1;  // D1
        uint8_t buckb_pg_raw 	:1;  // D2
        uint8_t bucka_pg_raw 	:1;  // D3
        uint8_t buckd_pg_filt 	:1;  // D4
        uint8_t buckc_pg_filt 	:1;  // D5
        uint8_t buckb_pg_filt 	:1;  // D6
        uint8_t bucka_pg_filt 	:1;  // D7
    };
    uint8_t bit;
} PMIC_PG_FILT_t;


// === PMIC_BUCK_UV_OV (40p) ===
typedef union {
    struct {
        uint8_t buckd_ov :1;  // D0
        uint8_t buckc_ov :1;  // D1
        uint8_t buckb_ov :1;  // D2
        uint8_t bucka_ov :1;  // D3
        uint8_t buckd_uv :1;  // D4
        uint8_t buckc_uv :1;  // D5
        uint8_t buckb_uv :1;  // D6
        uint8_t bucka_uv :1;  // D7
    };
    uint8_t bit;
} PMIC_UV_OV_t;


// === PMIC_BUCK_OC_OCwar (40p) ===
typedef union {
    struct {
        uint8_t buckd_oc_warning 	:1;  // D0
        uint8_t buckc_oc_warning 	:1;  // D1
        uint8_t buckb_oc_warning 	:1;  // D2
        uint8_t bucka_oc_warning 	:1;  // D3
        uint8_t buckd_oc 			:1;  // D4
        uint8_t buckc_oc 			:1;  // D5
        uint8_t buckb_oc 			:1;  // D6
        uint8_t bucka_oc 			:1;  // D7
    };
    uint8_t bit;
} PMIC_OC_OCwar_t;


// === All struct ===
typedef struct {
	PMIC_CTRL_STATE_t	ctrl_state;
	PMIC_PG_FILT_t		pg_filt;
    PMIC_UV_OV_t    	uv_ov;
    PMIC_OC_OCwar_t  	oc_warn;
    uint8_t         	system;
} PMIC_Faults_t;


// === 전역 변수 ===
extern PMIC_Faults_t g_pmicFaults;
extern volatile uint8_t g_pmicTransferDone;		 // I2C DMA 전송 완료 여부, 0 : 아직 진행 중, 1 : 완료됨

// === API ===
HAL_StatusTypeDef PMIC_Init_DMA(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef PMIC_ReadReg_DMA(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data, uint16_t len);
HAL_StatusTypeDef PMIC_WriteReg_DMA(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data, uint16_t len);
HAL_StatusTypeDef PMIC_ReadAllFaults_DMA(I2C_HandleTypeDef *hi2c);

uint8_t PMIC_HasVoltageFault(PMIC_Faults_t *faults);
uint8_t PMIC_HasCurrentFault(PMIC_Faults_t *faults);
uint8_t PMIC_HasTempFault(PMIC_Faults_t *faults);

// === ADC & Vin Monitoring ===
uint16_t ReadVinADC(void);  // ADC로 Vin 읽기
void PMIC_VinMonitorTask(void);  // 1ms 주기 Task

// === HAL 콜백 ===
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c);
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c);

// === PMIC.h API 섹션 ===
void PMIC_RecordFaultDTC(I2C_HandleTypeDef *hi2c, SPI_HandleTypeDef *hspi);

#ifdef __cplusplus
}
#endif

#endif /* INC_PMIC_H_ */
