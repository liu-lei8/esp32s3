#include "uart.h"

void usart_init(uint32_t baudrate)
{
    uart_config_t uart_cfg = {
        .baud_rate = baudrate,
        .data_bits = UART_DATA_8_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .parity = UART_PARITY_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_APB,
        .stop_bits = UART_STOP_BITS_1
    };
    uart_param_config(USART_UX, &uart_cfg);

    uart_set_pin(USART_UX, USART_TX_GPIO_PIN, USART_RX_GPIO_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_driver_install(USART_UX, RX_BUF_SIZE * 2, RX_BUF_SIZE * 2, 20, NULL, 0);
}