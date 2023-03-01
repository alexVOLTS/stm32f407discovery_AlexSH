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




static void io_set_rx_handler(char rx);
static void io_logs_rx_handler(char rx);
static void io_console_rx_handler(char rx);
/******************************************************************************/




void io_init(void)
{
    io_set_mode(IO_LOGS);

    RxTaskHandle = osThreadNew(IO_RxTask, NULL, &RxTask_attributes);
    TxTaskHandle = osThreadNew(IO_TxTask, NULL, &TxTask_attributes);
    uartRxQueueHandle = osMessageQueueNew(512, sizeof(uint8_t), &uartRxQueueAttributes);
}


void io_set_mode(IOSYS_MODE mode)
{
    io_system.mode = mode;
}
/******************************************************************************/




IOSYS_MODE io_get_mode(void)
{
    return io_system.mode;
}
/******************************************************************************/




void IO_RxTask(void *argument)
{
    uint8_t rx = 0;
    uint8_t data = 0;

    log_init();
    console_init();
    log_print_welcome_msg();

    for(;;)
    {
        if (lwrb_get_free(&data_uart.lwrb_rx) != 0) {
            lwrb_read(&data_uart.lwrb_rx, &data, sizeof(char));
            osMessageQueuePut(uartRxQueueHandle, &data, 0, 200);
        }

        if (!(io_get_byte(&rx, 100))) {
            continue;
        }

        io_set_rx_handler(rx);
        if (io_system.rx_handler != NULL) {
            io_system.rx_handler(rx);
        }

        osDelay(1);
    }

    osThreadTerminate(NULL);
}
/******************************************************************************/




void IO_TxTask(void *argument)
{
    for(;;)
    {
        if (io_get_mode() == IO_CONSOLE) {
            if (lwrb_get_free(&data_uart.lwrb_rx) != 0) {

                uint8_t msg = 0x00;
                osStatus_t event = osMessageQueueGet(consoleQueueHandle, &msg, NULL, 200);

                if (event != osOK) {
                    continue;
                }
                lwrb_write(&data_uart.lwrb_tx, &msg, sizeof(uint8_t));
            }
            uart_send_byte_tx_buff(&huart3);
        }
        else if (io_get_mode() == IO_LOGS) {
            if (lwrb_get_free(&data_uart.lwrb_rx) != 0) {

                uint8_t msg = 0x00;
                osStatus_t event = osMessageQueueGet(logsQueueHandle, &msg, NULL, 200);

                if (event != osOK) {
                    continue;
                }
                lwrb_write(&data_uart.lwrb_tx, &msg, sizeof(uint8_t));
            }
            uart_send_byte_tx_buff(&huart3);
        }
        osDelay(1);
    }

    osThreadTerminate(NULL);
}
/******************************************************************************/




bool io_get_byte(uint8_t *data, uint32_t timeout_ms)
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




void io_set_rx_handler(char rx)
{
    if (io_get_mode() == IO_CONSOLE) {
        io_system.rx_handler = io_console_rx_handler;
        return;
    }

    if (io_get_mode() == IO_IDLE) {
        return;
    }

    if (io_get_mode() == IO_LOGS) {
        io_system.rx_handler = io_logs_rx_handler;
        return;
    }

    io_set_mode(IO_IDLE);
    osTimerStart(io_system.soft_timeout, SOFT_TIMEOUT_MS);
}
/******************************************************************************/




void io_console_rx_handler(char rx)
{
    console_insert_char(rx);
}
/******************************************************************************/




void io_logs_rx_handler(char rx)
{
    if ((rx == 'T') || (rx == 't')) {
        io_set_mode(IO_CONSOLE);
        console_start();
        return;
    }
}



void io_clear_rx_queue(void)
{
    osMessageQueueReset(uartRxQueueHandle);
}









