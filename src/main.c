/*
 * Versão corrigida para FRDM-KL25Z - UART funcionando
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

#define UART_DEVICE_NODE DT_NODELABEL(uart0)
#define MAX_TX_LEN 64

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);
LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

/* Função para enviar string via UART */
static void uart_printf(const char *fmt, ...)
{
    char buffer[MAX_TX_LEN];
    va_list args;
    int len;

    va_start(args, fmt);
    len = vsnprintk(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len > 0) {
        for (int i = 0; i < len; i++) {
            uart_poll_out(uart_dev, buffer[i]);
        }
    }
}

int main(void)
{
    int loop_counter = 0;

    LOG_INF("Inicializando UART no FRDM-KL25Z...");

    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not ready!");
        return 0;
    }

    LOG_INF("Sistema inicializado. Enviando dados a cada 5 segundos...");
    
    /* Mensagem inicial */
    uart_printf("\r\n=== Sistema UART FRDM-KL25Z Iniciado ===\r\n");

    while (1) {
        k_sleep(K_SECONDS(5));

        /* Número de pacotes baseado no contador */
        uint8_t num_packets = (loop_counter % 3) + 1; /* 1, 2 ou 3 pacotes */
        
        LOG_INF("Loop %d: Enviando %d pacotes", loop_counter, num_packets);
        uart_printf("--- Loop %d: Enviando %d pacotes ---\r\n", loop_counter, num_packets);

        for (int i = 0; i < num_packets; i++) {
            uart_printf("Pacote %d/%d: Tempo desde inicio: %d segundos\r\n", 
                       i + 1, num_packets, loop_counter * 5);
            
            k_msleep(100); /* Pequena pausa entre pacotes */
        }

        uart_printf("--- Fim do loop %d ---\r\n\r\n", loop_counter);
        loop_counter++;
    }
}
