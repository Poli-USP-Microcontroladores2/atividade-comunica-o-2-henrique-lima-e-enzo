/*
 * Copyright (c) 2022 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

#include <string.h>

/* Register log module */
LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
#define MSG_SIZE 32
#define MSG_NUMBER 10

K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, MSG_NUMBER, 4);
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static char rx_buf[MSG_SIZE];
static int rx_buf_pos;
static bool in_rx_mode = true;  // Começa em RX mode

void serial_cb(const struct device *dev, void *user_data)
{
    uint8_t c;

    // DEBUG: Log sempre que o callback é chamado
    LOG_DBG("Callback called - RX mode: %d", in_rx_mode);

    if (!uart_irq_update(uart_dev)) return;
    if (!uart_irq_rx_ready(uart_dev)) return;

    while (uart_fifo_read(uart_dev, &c, 1) == 1) {
        // DEBUG: Log cada caractere recebido
        LOG_DBG("Char received: 0x%02x, RX mode: %d", c, in_rx_mode);
        
        if (!in_rx_mode) {
            // Modo TX: simplesmente ignora o caractere
            LOG_DBG("Ignoring char in TX mode");
            continue;
        }

        // Modo RX: processa normalmente
        if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
            rx_buf[rx_buf_pos] = '\0';

            LOG_INF("Data received: %s", rx_buf);

            char msg_copy[MSG_SIZE];
            strncpy(msg_copy, rx_buf, MSG_SIZE - 1);
            msg_copy[MSG_SIZE - 1] = '\0';

            k_msgq_put(&uart_msgq, &msg_copy, K_NO_WAIT);
            rx_buf_pos = 0;
        } else if (rx_buf_pos < (MSG_SIZE - 1)) {
            if (c != '\n' && c != '\r') {
                rx_buf[rx_buf_pos++] = c;
            }
        } else {
            rx_buf_pos = 0;
        }
    }
}

void print_uart(char *buf)
{
    if (buf == NULL) return;
    int msg_len = strlen(buf);
    for (int i = 0; i < msg_len; i++) {
        uart_poll_out(uart_dev, buf[i]);
    }
}

int main(void)
{
    char tx_buf[MSG_SIZE];

    if (!device_is_ready(uart_dev)) {
        printk("UART device not found!");
        return 0;
    }

    rx_buf_pos = 0;
    memset(rx_buf, 0, sizeof(rx_buf));

    int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
    if (ret < 0) {
        printk("Error setting UART callback: %d\n", ret);
        return 0;
    }

    uart_irq_rx_enable(uart_dev);

    while (1) {
        static int cycle_count = 0;
        cycle_count++;

        // === RX MODE ===
        in_rx_mode = true;
        LOG_INF("RX is now enabled - Cycle %d", cycle_count);
        LOG_INF("UART callback: RX_RDY");
        k_sleep(K_SECONDS(5));

        // === TX MODE ===  
        in_rx_mode = false;
        LOG_INF("RX is now disabled - Cycle %d", cycle_count);

        int message_count = 0;
        while (k_msgq_get(&uart_msgq, &tx_buf, K_NO_WAIT) == 0) {
            message_count++;
            print_uart("Data ");
            char count_str[12];
            snprintf(count_str, sizeof(count_str), "%d", message_count);
            print_uart(count_str);
            print_uart(": ");
            print_uart(tx_buf);
            print_uart("\r\n");
        }

        LOG_INF("UART callback: TX_DONE");


        // Espera 5s no TX mode
        k_sleep(K_SECONDS(5));

        k_sleep(K_MSEC(100));
    }
    
    return 0;
}