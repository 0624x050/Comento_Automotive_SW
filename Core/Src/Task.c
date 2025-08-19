/*
 * Task.c  — HAL only / Mutex single / Ordered pipeline (I2C→SPI→CAN→UART)
 */

#include "Task.h"
#include "eeprom.h"
#include "pmic.h"
#include "uds_can.h"

// 내부 파이프라인 버퍼
static uint8_t rxFaultBuf[3];   // PMIC Fault raw
static uint8_t dtcBuf[2];       // EEPROM 저장용 (DTC 코드 2B)
static uint8_t eepromReadBuf[2];// CAN/USART 송신용

// 실행 단계 (0:I2C → 1:SPI → 2:CAN → 3:UART)
static volatile uint8_t currentStep = 0;

void StartDefaultTask(void *argument)
{
    for (;;) {
        osDelay(1);
    }
}

void StartI2CTask(void *argument)
{
    PMIC_Faults_t faults;

    for (;;)
    {
        osMutexAcquire(CommMutexHandle, osWaitForever);

        if (currentStep == 0)
        {
            // 1) PMIC Fault 읽기 (I2C + DMA)
            (void)HAL_I2C_Mem_Read_DMA(&hi2c1,
                                       I2C_SLAVE_ADDRESS,
                                       PMIC_REG_UV_OV,
                                       I2C_MEMADD_SIZE_8BIT,
                                       rxFaultBuf,
                                       sizeof(rxFaultBuf));

            // NOTE: 간단 대기(예시). 실제로는 I2C DMA 콜백에서 완료 동기화 권장.
            osDelay(5);

            // 2) Fault 판정
            faults.uv_ov   = rxFaultBuf[0];
            faults.oc_warn = rxFaultBuf[1];
            faults.system  = rxFaultBuf[2];

            if (PMIC_HasVoltageFault(&faults) || PMIC_HasCurrentFault(&faults)) {
                // 예시 DTC 코드 (2바이트)
                const uint16_t dtcCode = 0xC123;
                dtcBuf[0] = (uint8_t)(dtcCode >> 8);
                dtcBuf[1] = (uint8_t)(dtcCode & 0xFF);

                // 3) EEPROM에 기록 (SPI)
                (void)EEPROM_WriteData(&hspi1, 0x0000, dtcBuf, 2);
            }

            // 다음 단계로
            currentStep = 1;
        }

        osMutexRelease(CommMutexHandle);
        osDelay(1);
    }
}

void StartSPITask(void *argument)
{
    for (;;)
    {
        osMutexAcquire(CommMutexHandle, osWaitForever);

        if (currentStep == 1)
        {
            // EEPROM에서 DTC 2바이트 읽기 (SPI)
            (void)EEPROM_ReadData(&hspi1, 0x0000, eepromReadBuf, 2);

            currentStep = 2; // 다음: CAN
        }

        osMutexRelease(CommMutexHandle);
        osDelay(1);
    }
}

void StartCANTask(void *argument)
{
    for (;;)
    {
        osMutexAcquire(CommMutexHandle, osWaitForever);

        if (currentStep == 2)
        {
            // CAN으로 2바이트 DTC 전송 (UDS 상위 계층은 uds_can 쪽에서 구성)
            // 여기서는 HAL CAN 기본 송신만 수행
            (void)HAL_CAN_AddTxMessage(&hcan1, &TxHeader, eepromReadBuf, &TxMailbox);

            currentStep = 3; // 다음: UART
        }

        osMutexRelease(CommMutexHandle);
        osDelay(1);
    }
}

void StartUARTTask(void *argument)
{
    for (;;)
    {
        osMutexAcquire(CommMutexHandle, osWaitForever);

        if (currentStep == 3)
        {
            // HAL UART로 2바이트 원시 DTC 전송 (문자열 포맷 X)
            (void)HAL_UART_Transmit(&huart4, eepromReadBuf, 2u, HAL_MAX_DELAY);

            currentStep = 0; // 파이프라인 한 바퀴 완료 → 다시 I2C
        }

        osMutexRelease(CommMutexHandle);
        osDelay(1);
    }
}
