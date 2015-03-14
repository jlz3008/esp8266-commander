/*
 * some stolen code from
* Copyright (C) 2014 -2016 Espressif System
*
*/
#include "eagle_soc.h"
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "uart.h"


static uint32_t uart_user_task;
static uint32_t uart_user_task_rx_signal;
static uint32_t uart_user_task_rx_tout_signal;
static uint32_t uart_user_task_eot_signal;
static uint32_t uart_user_task_err_signal;


//------------------------------------------------------------------
LOCAL STATUS
uart_tx_one_char(uint8 uart, uint8 TxChar)
{
    while (true) {
        uint32 fifo_cnt = READ_PERI_REG(UART_STATUS(uart)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S);
        if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126) {
            break;
        }
    }
    WRITE_PERI_REG(UART_FIFO(uart) , TxChar);
    return OK;
}
//------------------------------------------------------------------
LOCAL void
uart0_write_char(char c)
{
    if (c == '\n') {
        uart_tx_one_char(UART0, '\r');
        uart_tx_one_char(UART0, '\n');
    } else if (c == '\r') {
    } else {
        uart_tx_one_char(UART0, c);
    }
}
//------------------------------------------------------------------
void ICACHE_FLASH_ATTR
UART_SetWordLength(UART_Port uart_no, UART_WordLength len)
{
    SET_PERI_REG_BITS(UART_CONF0(uart_no), UART_BIT_NUM, len, UART_BIT_NUM_S);
}
//------------------------------------------------------------------
void ICACHE_FLASH_ATTR
UART_SetStopBits(UART_Port uart_no, UART_StopBits bit_num)
{
    SET_PERI_REG_BITS(UART_CONF0(uart_no), UART_STOP_BIT_NUM, bit_num, UART_STOP_BIT_NUM_S);
}
//------------------------------------------------------------------
void ICACHE_FLASH_ATTR
UART_SetLineInverse(UART_Port uart_no, UART_LineLevelInverse inverse_mask)
{
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_LINE_INV_MASK);
    SET_PERI_REG_MASK(UART_CONF0(uart_no), inverse_mask);
}
//------------------------------------------------------------------
void ICACHE_FLASH_ATTR
UART_SetParity(UART_Port uart_no, UART_ParityMode Parity_mode)
{
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_PARITY | UART_PARITY_EN);
    if (Parity_mode == USART_Parity_None) {
    } else {
        SET_PERI_REG_MASK(UART_CONF0(uart_no), Parity_mode | UART_PARITY_EN);
    }
}
//------------------------------------------------------------------
void ICACHE_FLASH_ATTR
UART_SetBaudrate(UART_Port uart_no, uint32 baud_rate)
{
    uart_div_modify(uart_no, UART_CLK_FREQ / baud_rate);
}
//------------------------------------------------------------------
//only when USART_HardwareFlowControl_RTS is set , will the rx_thresh value be set.
void ICACHE_FLASH_ATTR
UART_SetFlowCtrl(UART_Port uart_no, UART_HwFlowCtrl flow_ctrl, uint8 rx_thresh)
{
    if (flow_ctrl & USART_HardwareFlowControl_RTS) {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_U0RTS);
        SET_PERI_REG_BITS(UART_CONF1(uart_no), UART_RX_FLOW_THRHD, rx_thresh, UART_RX_FLOW_THRHD_S);
        SET_PERI_REG_MASK(UART_CONF1(uart_no), UART_RX_FLOW_EN);
    } else {
        CLEAR_PERI_REG_MASK(UART_CONF1(uart_no), UART_RX_FLOW_EN);
    }
    if (flow_ctrl & USART_HardwareFlowControl_CTS) {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_UART0_CTS);
        SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_TX_FLOW_EN);
    } else {
        CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_TX_FLOW_EN);
    }
}

//------------------------------------------------------------------
void ICACHE_FLASH_ATTR
UART_WaitTxFifoEmpty(UART_Port uart_no) //do not use if tx flow control enabled
{
    while (READ_PERI_REG(UART_STATUS(uart_no)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S));
}
//------------------------------------------------------------------
void ICACHE_FLASH_ATTR
UART_ResetFifo(UART_Port uart_no)
{
    SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
}
//------------------------------------------------------------------
void ICACHE_FLASH_ATTR
UART_ClearIntrStatus(UART_Port uart_no, uint32 clr_mask)
{
    WRITE_PERI_REG(UART_INT_CLR(uart_no), clr_mask);
}
//------------------------------------------------------------------
void ICACHE_FLASH_ATTR
UART_SetIntrEna(UART_Port uart_no, uint32 ena_mask)
{
    SET_PERI_REG_MASK(UART_INT_ENA(uart_no), ena_mask);
}
//------------------------------------------------------------------
void ICACHE_FLASH_ATTR
UART_SetPrintPort(UART_Port uart_no)
{
    os_install_putc1(uart0_write_char);
}
//------------------------------------------------------------------
void ICACHE_FLASH_ATTR
UART_ParamConfig(UART_Port uart_no, UART_ConfigTypeDef *pUARTConfig)
{
    if (uart_no == UART1) {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
    } else {
        PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_U0RXD);
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
    }
    UART_SetFlowCtrl(uart_no, pUARTConfig->flow_ctrl, pUARTConfig->UART_RxFlowThresh);
    UART_SetBaudrate(uart_no, pUARTConfig->baud_rate);
    WRITE_PERI_REG(UART_CONF0(uart_no),
                   ((pUARTConfig->parity == USART_Parity_None) ? 0x0 : (UART_PARITY_EN | pUARTConfig->parity))
                   | (pUARTConfig->stop_bits << UART_STOP_BIT_NUM_S)
                   | (pUARTConfig->data_bits << UART_BIT_NUM_S)
                   | ((pUARTConfig->flow_ctrl & USART_HardwareFlowControl_CTS) ? UART_TX_FLOW_EN : 0x0)
                   | pUARTConfig->UART_InverseMask);
    UART_ResetFifo(uart_no);
}
//------------------------------------------------------------------
void ICACHE_FLASH_ATTR
UART_IntrConfig(UART_Port uart_no, UART_IntrConfTypeDef *pUARTIntrConf)
{
    uint32 reg_val = 0;
    UART_ClearIntrStatus(uart_no, UART_INTR_MASK);

    reg_val = READ_PERI_REG(UART_CONF1(uart_no)) & ~((UART_RX_FLOW_THRHD << UART_RX_FLOW_THRHD_S) | UART_RX_FLOW_EN) ;

    reg_val |= ((pUARTIntrConf->UART_IntrEnMask & UART_RXFIFO_TOUT_INT_ENA) ?
                    ((((pUARTIntrConf->UART_RX_TimeOutIntrThresh)&UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S) | UART_RX_TOUT_EN) : 0);

    reg_val |= ((pUARTIntrConf->UART_IntrEnMask & UART_RXFIFO_FULL_INT_ENA) ?
                    (((pUARTIntrConf->UART_RX_FifoFullIntrThresh)&UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S) : 0);

    reg_val |= ((pUARTIntrConf->UART_IntrEnMask & UART_TXFIFO_EMPTY_INT_ENA) ?
                    (((pUARTIntrConf->UART_TX_FifoEmptyIntrThresh)&UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S) : 0);

    WRITE_PERI_REG(UART_CONF1(uart_no), reg_val);
    CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_INTR_MASK);
    SET_PERI_REG_MASK(UART_INT_ENA(uart_no), pUARTIntrConf->UART_IntrEnMask);
}
//------------------------------------------------------------------
LOCAL void
uart0_rx_intr_handler(void *para)
{
    /* uart0 and uart1 intr combine togther, when interrupt occur, see reg 0x3ff20020, bit2, bit0 represents
* uart1 and uart0 respectively
*/
    uint8 RcvChar;
    uint8 fifo_len = 0;
    uint8 buf_idx = 0;
    uint8 fifo_tmp[128] = {0};


    uint32 uart_intr_status = READ_PERI_REG(UART_INT_ST(UART0)) ;

    while (uart_intr_status != 0x0)
    {
        if (UART_FRM_ERR_INT_ST == (uart_intr_status & UART_FRM_ERR_INT_ST))
        {
            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_FRM_ERR_INT_CLR);
            system_os_post(uart_user_task, uart_user_task_err_signal, UART_FRM_ERR_INT_ST);
        }
        else if (UART_BRK_DET_INT_ST == (uart_intr_status & UART_BRK_DET_INT_ST))
        {
            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_BRK_DET_INT_CLR);
            system_os_post(uart_user_task, uart_user_task_err_signal, UART_BRK_DET_INT_ST);
        }
        else if (UART_CTS_CHG_INT_ST == (uart_intr_status & UART_CTS_CHG_INT_ST))
        {
            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_CTS_CHG_INT_CLR);
            system_os_post(uart_user_task, uart_user_task_err_signal, UART_CTS_CHG_INT_ST);
        }
        else if (UART_DSR_CHG_INT_ST == (uart_intr_status & UART_DSR_CHG_INT_ST))
        {
            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_DSR_CHG_INT_CLR);
            system_os_post(uart_user_task, uart_user_task_err_signal, UART_DSR_CHG_INT_ST);
        }
        else if (UART_RXFIFO_OVF_INT_ST == (uart_intr_status & UART_RXFIFO_OVF_INT_ST))
        {
            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_OVF_INT_CLR);
            system_os_post(uart_user_task, uart_user_task_err_signal, UART_RXFIFO_OVF_INT_ST);
        }
        else if (UART_PARITY_ERR_INT_ST == (uart_intr_status & UART_PARITY_ERR_INT_ST))
        {
            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_PARITY_ERR_INT_CLR);
            system_os_post(uart_user_task, uart_user_task_err_signal, UART_PARITY_ERR_INT_ST);
        }
        else if (UART_RXFIFO_TOUT_INT_ST == (uart_intr_status & UART_RXFIFO_TOUT_INT_ST))
        {
            uint8_t RcvChar = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;

            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
            system_os_post(uart_user_task, uart_user_task_rx_tout_signal, RcvChar);
        }
        else if (UART_RXFIFO_FULL_INT_ST == (uart_intr_status & UART_RXFIFO_FULL_INT_ST))
        {
            uint8_t RcvChar = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);

            system_os_post(uart_user_task, uart_user_task_rx_signal, RcvChar);
        }
        else if (UART_TXFIFO_EMPTY_INT_ST == (uart_intr_status & UART_TXFIFO_EMPTY_INT_ST))
        {
            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_TXFIFO_EMPTY_INT_CLR);
            CLEAR_PERI_REG_MASK(UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA);

            system_os_post(uart_user_task, uart_user_task_eot_signal, 0);
        }
        else
        {
            //skip
        }

        uart_intr_status = READ_PERI_REG(UART_INT_ST(UART0)) ;
    }
}
//------------------------------------------------------------------
void ICACHE_FLASH_ATTR
uart_init(uint32_t user_task,
          uint32_t user_task_rx_signal,
          uint32_t user_task_rx_tout_signal,
          uint32_t user_task_err_signal,
          uint32_t user_task_eot_signal)
{
// UART0 only used. No uart 1 not configured. I don't try it and may not work.

   uart_user_task = user_task;
   uart_user_task_rx_signal = user_task_rx_signal;
   uart_user_task_rx_tout_signal = user_task_rx_tout_signal;
   uart_user_task_err_signal = user_task_err_signal;
   uart_user_task_eot_signal = user_task_eot_signal;

    UART_WaitTxFifoEmpty(UART0);

    UART_ConfigTypeDef uart_config;
    uart_config.baud_rate = BIT_RATE_19200;
    uart_config.data_bits = UART_WordLength_8b;
    uart_config.parity = USART_Parity_None;
    uart_config.stop_bits = USART_StopBits_1;
    uart_config.flow_ctrl = USART_HardwareFlowControl_None;
    uart_config.UART_RxFlowThresh = 120;
    uart_config.UART_InverseMask = UART_None_Inverse;

    UART_ParamConfig(UART0, &uart_config);

    UART_IntrConfTypeDef uart_intr;
    uart_intr.UART_IntrEnMask = UART_RXFIFO_TOUT_INT_ENA | UART_FRM_ERR_INT_ENA | UART_RXFIFO_FULL_INT_ENA | UART_TXFIFO_EMPTY_INT_ENA;
    uart_intr.UART_RX_FifoFullIntrThresh = 10;
    uart_intr.UART_RX_TimeOutIntrThresh = 2;
    uart_intr.UART_TX_FifoEmptyIntrThresh = 20;

    ETS_UART_INTR_DISABLE();
    ETS_UART_INTR_ATTACH(uart0_rx_intr_handler,  NULL);
    ETS_UART_INTR_ENABLE();

    UART_IntrConfig(UART0, &uart_intr);

    UART_SetPrintPort(UART0);  // use os_printf() to send characters and user task to receive
}

