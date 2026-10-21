#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_intr_alloc.h>
#include <esp_log.h>
#include <driver/uart.h>
#include <cstdint.h>
#include <array>

#define UART_NUM UART_NUM_1
#define UART_IRQ_NUM UART_NUM_1
#define UART_TX_BUF_SIZE 256
#define UART_RX_BUF_SIZE 256

static SemaphoreHandle_t uart_sem;
static SemaphoreHandle_t buffer_sem;
#define TX_BUFFER_SIZE 10
#define RX_BUFFER_SIZE 10

// Buffer to hold incoming and outgoing data
static char tx_buffer[TX_BUFFER_SIZE];
static char rx_buffer[RX_BUFFER_SIZE];
static size_t tx_head = 0;
static size_t tx_tail = 0;
static size_t rx_head = 0;
static size_t rx_tail = 0;

static esp_err_t uart_isr_handle(esp_err_t err, int inf_id, uint16_t len) {
    if (err == ESP_OK) {
        // Handle the received data (if needed)
        // You can process the data here and then call task_send() to send it further
        esp_log_debug("ISR: Received data: %.*s", len, (char*) &rx_buffer[rx_head]);
    }
    return err;
}

void app_main() {
    // Initialize UART
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_NONE,
        .stop_bits = UART_STOP_1_BITS,
        .mtu = UART_MTU_8_BITS
    };
    uart_init(UART_NUM, &uart_config);
    uart_register_handler(UART_IRQ_NUM, uart_isr_handle);

    // Create semaphores
    uart_sem = xSemaphoreCreateBinary();
    buffer_sem = xSemaphoreCreateBinary();

    // Create tasks
    xTaskCreate(vuart_task, "UART TX", 4096, &tx_buffer, sizeof(tx_buffer), 1, NULL);
    xTaskCreate(vuart_task, "UART RX", 4096, &rx_buffer, sizeof(rx_buffer), 1, NULL);

    // Start the scheduler
    vTaskStartScheduler();
}

void uart_isr_handle(esp_err_t err, int inf_id, uint16_t len) {
    // Give the interrupt handler a chance to complete
    xSemaphoreGiveFromISR(uart_sem, NULL);

    // Wait for the semaphore to become available
    while (xSemaphoreTake(uart_sem, 0, 0) != NULL) {}

    if (len > 0) {
        // Write data to the tx buffer
        for (uint16_t i = 0; i < len; i++) {
            tx_buffer[tx_tail] = (char)esp_intr_alloc_get_inf_data(inf_id)[0];
            tx_tail = (tx_tail + 1) % TX_BUFFER_SIZE;
            xSemaphoreGiveFromISR(buffer_sem, NULL);
        }
    }
}

void uart_task(void *pvParameters) {
    char tx_char;
    while (1) {
        // Wait for the semaphore to become available
        xSemaphoreTakeFromISR(buffer_sem, &eTaskSentinel, portMAX_DELAY);
        while (tx_head != tx_tail) {
            tx_char = tx_buffer[tx_head];
            tx_head = (tx_head + 1) % TX_BUFFER_SIZE;
            xSemaphoreGiveFromISR(buffer_sem, &eTaskSentinel);
            // Send data via UART here if needed
            // esp_log_debug("TX: %c", tx_char);
        }
    }
}

void uart_task_rx(void *pvParameters) {
    char rx_char;
    while (1) {
        xSemaphoreTakeFromISR(buffer_sem, &eTaskSentinel, portMAX_DELAY);
        while (rx_head != rx_tail) {
            rx_char = rx_buffer[rx_head];
            rx_head = (rx_head + 1) % RX_BUFFER_SIZE;
            xSemaphoreGiveFromISR(buffer_sem, &eTaskSentinel);
            // Send data via UART here if needed
            // esp_log_debug("RX: %c", rx_char);
        }
    }
}

