/*
 * io_system.c
 *
 *  Created on: 26 февр. 2023 г.
 *      Author: АлексанDOOR
 */

#include "io_system.h"
/******************************************************************************/



/******************************************************************************/
/* Private defines ---------------------------------------------------------- */
/******************************************************************************/
#define SOFT_TIMEOUT_MS             (1000U)



static IO_SYSTEM io_system;
/******************************************************************************/




osThreadId_t RxTaskHandle;
osThreadId_t TxTaskHandle;

osMessageQueueId_t uartRxQueueHandle;
/******************************************************************************/




const osThreadAttr_t RxTask_attributes = {
        .name = "RxTask",
        .stack_size = 128 * 4,
        .priority = (osPriority_t) osPriorityNormal1,
};

const osThreadAttr_t TxTask_attributes = {
        .name = "TxTask",
        .stack_size = 128 * 4,
        .priority = (osPriority_t) osPriorityNormal1,
};

const osMessageQueueAttr_t uartRxQueueAttributes = {
        .name = "uartRxQueue",
};
/******************************************************************************/




static void prvIoSystemSetRxHandler(char rx);
static void prvIoLogsRxHandler(char rx);
static void prvIoConsoleRxHandler(char rx);
/******************************************************************************/




void IoSystemInit(void)
{
    IoSystemSetMode(IO_LOGS);

    RxTaskHandle = osThreadNew(IoSystemRxTask, NULL, &RxTask_attributes);
    TxTaskHandle = osThreadNew(IoSystemTxTask, NULL, &TxTask_attributes);
    uartRxQueueHandle = osMessageQueueNew(512, sizeof(uint8_t), &uartRxQueueAttributes);
}
/******************************************************************************/




void IoSystemSetMode(IOSYS_MODE mode)
{
    io_system.mode = mode;
}
/******************************************************************************/




IOSYS_MODE IoSystemGetMode(void)
{
    return io_system.mode;
}
/******************************************************************************/




void IoSystemRxTask(void *argument)
{
    uint8_t rx = 0x00;

    LogInit();
    ConsoleInit();
    LogPrintWelcomeMsg();

    for(;;)
    {
        uint8_t data = 0x00;

        if (lwrb_get_free(&data_uart.lwrb_rx) != 0) {
            lwrb_read(&data_uart.lwrb_rx, &data, sizeof(char));
            osMessageQueuePut(uartRxQueueHandle, &data, 0, 100);
        }

        if (!(IoSystemGetByte(&rx, 100))) {
            continue;
        }

        IoSystemClearRxQueue();

        prvIoSystemSetRxHandler(rx);
        if (io_system.rx_handler != NULL) {
            io_system.rx_handler(rx);
        }

        osDelay(1);
    }

    osThreadTerminate(NULL);
}
/******************************************************************************/




void IoSystemTxTask(void *argument)
{
    for(;;)
    {
        if (IoSystemGetMode() == IO_CONSOLE) {
            if (!(IoSystemIsTxBufferFull())) {

                uint8_t msg = 0x00;
                osStatus_t event = osMessageQueueGet(consoleQueueHandle, &msg, NULL, 200);

                if (event != osOK) {
                    continue;
                }
                IoSystemPutDataToTxBuffer(&msg, sizeof(uint8_t));
            }
            UARTSendByteTxBuff(&huart3);
        }
        else if (IoSystemGetMode() == IO_LOGS) {
            if (!(IoSystemIsTxBufferFull())) {

                uint8_t msg = 0x00;
                osStatus_t event = osMessageQueueGet(logsQueueHandle, &msg, NULL, 200);

                if (event != osOK) {
                    continue;
                }
                IoSystemPutDataToTxBuffer(&msg, sizeof(uint8_t));
            }
            UARTSendByteTxBuff(&huart3);
        }
        osDelay(1);
    }

    osThreadTerminate(NULL);
}
/******************************************************************************/




bool IoSystemGetByte(uint8_t *data, uint32_t timeout_ms)
{
    *data = 0x00;

    osStatus_t event = osMessageQueueGet(uartRxQueueHandle, data, NULL, timeout_ms);

    if (event == osOK) {
        return true;
    }
    else {
        return false;
    }
}
/******************************************************************************/




void prvIoSystemSetRxHandler(char rx)
{
    if (IoSystemGetMode() == IO_CONSOLE) {
        io_system.rx_handler = prvIoConsoleRxHandler;
        return;
    }

//    if (IoSystemGetMode() == IO_LOGS) {
//        io_system.rx_handler = io_logs_rx_handler;
//        return;
//    }

    IoSystemSetMode(IO_LOGS);
    io_system.rx_handler = prvIoLogsRxHandler;
}
/******************************************************************************/




void prvIoConsoleRxHandler(char rx)
{
//    if (!data_uart.flag) {
//        return;
//    }

    ConsoleInsertChar(rx);

//    data_uart.flag = false;
}
/******************************************************************************/




void prvIoLogsRxHandler(char rx)
{
    if ((rx == 'T') || (rx == 't')) {
        IoSystemSetMode(IO_CONSOLE);
        ConsoleStart();
        return;
    }

    if ((rx == 'L') || (rx == 'l')) {
        IoSystemSetMode(IO_LOGS);
    }
//    else if ((rx == 'X') || (rx == 'x')) {
//        io_set_mode(IO_IDLE);
//        log_clear_queues();
//    }
}
/******************************************************************************/




void IoSystemClearRxQueue(void)
{
    osMessageQueueReset(uartRxQueueHandle);
}
/******************************************************************************/




bool IoSystemIsTxBufferFull(void)
{
    return (lwrb_get_free(&data_uart.lwrb_tx) == 0 ? true : false);
}
/******************************************************************************/




bool IoSystemPutDataToTxBuffer(const void* data, size_t len)
{
    if (lwrb_get_free(&data_uart.lwrb_tx) == 0) {
        return false;
    }

    return (lwrb_write(&data_uart.lwrb_tx, data, len) > 0 ? true : false);
}
/******************************************************************************/




bool IoSystemPutDataToRxBuffer(const void* data, size_t len)
{
    if (lwrb_get_free(&data_uart.lwrb_rx) == 0) {
        return false;
    }

    return (lwrb_write(&data_uart.lwrb_rx, data, len) > 0 ? true : false);
}
/******************************************************************************/

