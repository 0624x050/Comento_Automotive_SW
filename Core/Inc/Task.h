/*
 * Task.h
 *
 *  Created on: Aug 17, 2025
 *      Author: 김나윤
 */

#ifndef INC_TASK_H_
#define INC_TASK_H_


#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

// main.c에서 생성/정의
extern osMutexId_t CommMutexHandle;
extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi1;
extern CAN_HandleTypeDef hcan1;
extern UART_HandleTypeDef huart4;

// UDS/CAN 드라이버 쪽에서 준비된 전송 헤더/메일박스
extern CAN_TxHeaderTypeDef TxHeader;
extern uint32_t TxMailbox;

// RTOS task entry
void StartDefaultTask(void *argument);
void StartI2CTask(void *argument);
void StartSPITask(void *argument);
void StartCANTask(void *argument);
void StartUARTTask(void *argument);


#endif /* INC_TASK_H_ */
