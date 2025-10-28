
#include "protocol.h"
#include "system/bus/uart.h"
#include "system/led.h"
#include "system/system.h"
#include "util/error.h"
#include "util/logging.h"
#include "util/util.h"

#include <string.h>

int main(void)
{
    SYSTEM_init();
    LOG_INIT("Main application");

    // UART passthrough test
    // Connect UART1 TX to UART2 RX
    // Connect UART4 TX to UART5 RX
    // UART1 init
    uint8_t uart1_rx_buffer[256], uart1_tx_buffer[256];
    CircularBuffer uart1_rx_circular_buffer, uart1_tx_circular_buffer;
    circular_buffer_init(&uart1_rx_circular_buffer, uart1_rx_buffer, sizeof(uart1_rx_buffer));
    circular_buffer_init(&uart1_tx_circular_buffer, uart1_tx_buffer, sizeof(uart1_tx_buffer));
    UART_Handle *uart1 = UART_init(0, &uart1_rx_circular_buffer, &uart1_tx_circular_buffer);

    // UART2 init
    uint8_t uart2_rx_buffer[256], uart2_tx_buffer[256];
    CircularBuffer uart2_rx_circular_buffer, uart2_tx_circular_buffer;
    circular_buffer_init(&uart2_rx_circular_buffer, uart2_rx_buffer, sizeof(uart2_rx_buffer));
    circular_buffer_init(&uart2_tx_circular_buffer, uart2_tx_buffer, sizeof(uart2_tx_buffer));
    UART_Handle *uart2 = UART_init(1, &uart2_rx_circular_buffer, &uart2_tx_circular_buffer);

    // UART4 init
    uint8_t uart4_rx_buffer[256], uart4_tx_buffer[256];
    CircularBuffer uart4_rx_circular_buffer, uart4_tx_circular_buffer;
    circular_buffer_init(&uart4_rx_circular_buffer, uart4_rx_buffer, sizeof(uart4_rx_buffer));
    circular_buffer_init(&uart4_tx_circular_buffer, uart4_tx_buffer, sizeof(uart4_tx_buffer));
    UART_Handle *uart4 = UART_init(3, &uart4_rx_circular_buffer, &uart4_tx_circular_buffer);

    // UART5 init
    uint8_t uart5_rx_buffer[256], uart5_tx_buffer[256];
    CircularBuffer uart5_rx_circular_buffer, uart5_tx_circular_buffer;
    circular_buffer_init(&uart5_rx_circular_buffer, uart5_rx_buffer, sizeof(uart5_rx_buffer));
    circular_buffer_init(&uart5_tx_circular_buffer, uart5_tx_buffer, sizeof(uart5_tx_buffer));
    UART_Handle *uart5 = UART_init(4, &uart5_rx_circular_buffer, &uart5_tx_circular_buffer);

    bool connection_ok = true;

    // Verify physical UART1 TX -> UART2 RX
    UART_write(uart1, "Hello, from UART1!", 25);
    uint32_t start = SYSTEM_get_tick();
    while (UART_rx_available(uart2) < 25) {
        if (SYSTEM_get_tick() - start > 1000) {
            LOG_ERROR("Timeout waiting for data on UART2");
            connection_ok = false;
            break;
        }
    }
    uint8_t recv_buf[25] = {0};
    UART_read(uart2, recv_buf, sizeof(recv_buf));
    LOG_INFO("Received UART data on UART2: %.*s", sizeof(recv_buf), recv_buf);

    // Verify physical UART4 TX -> UART5 RX
    UART_write(uart4, "Hello, from UART4!", 25);
    start = SYSTEM_get_tick();
    while (UART_rx_available(uart5) < 25) {
        if (SYSTEM_get_tick() - start > 1000) {
            LOG_ERROR("Timeout waiting for data on UART5");
            connection_ok = false;
            break;
        }
    }
    memset(recv_buf, 0, sizeof(recv_buf));
    UART_read(uart5, recv_buf, sizeof(recv_buf));
    LOG_INFO("Received UART data on UART5: %.*s", sizeof(recv_buf), recv_buf);

    if (!connection_ok) {
        LOG_ERROR("UART connection test failed");
        goto wait;
    }

    // Enable passthrough from UART2 to UART4
    LOG_INFO("Enabling passthrough from UART2 to UART4");
    UART_enable_passthrough(uart2, uart4);
    // Write data to UART1 -> UART2 -> (passthrough) -> UART4 -> UART5
    UART_write(uart1, "Hello, from UART1!", 25);
    start = SYSTEM_get_tick();
    while (UART_rx_available(uart5) < 25) {
        if (SYSTEM_get_tick() - start > 1000) {
            LOG_ERROR("PASSTHROUGH: Timeout waiting for data on UART5");
            connection_ok = false;
            break;
        }
    }
    memset(recv_buf, 0, sizeof(recv_buf));
    UART_read(uart5, recv_buf, sizeof(recv_buf));
    LOG_INFO("PASSTHROUGH: Received UART data on UART5: %.*s", sizeof(recv_buf), recv_buf);

    wait:
    while (1) { LOG_task(1); }

    // Initialize the protocol
    if (!protocol_init()) {
        LOG_ERROR("Failed to initialize protocol");
        return -1;
    }

    // Main application loop
    while (1) {
        // Process protocol tasks
        protocol_task();

        LOG_task(0xF);

        static uint32_t last_toggle = 0;
        uint32_t const blink_period = 1000; // 1 second
        if (SYSTEM_get_tick() - last_toggle >= blink_period) {
            LED_toggle(LED_YELLOW);
            last_toggle = SYSTEM_get_tick();
        }
    }

    __builtin_unreachable();
}
