#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart.h"

// Includes from Modbus library documentation
#include "esp_modbus_common.h"
#include "esp_modbus_slave.h"

// Define the tag for logging
static const char *TAG = "MODBUS_SLAVE";

// Define register sizes for our slave device
#define MB_REG_DISCRETE_INPUT_CNT   (9)
#define MB_REG_COILS_CNT            (10)
#define MB_REG_INPUT_CNT            (4)
#define MB_REG_HOLD_CNT             (2)

// Define the starting offset for each register area (usually 0)
#define MB_REG_DISCRETE_INPUT_START (0)
#define MB_REG_COILS_START          (0)
#define MB_REG_INPUT_START          (0)
#define MB_REG_HOLD_START           (0)

// Define Modbus communication settings
#define MB_SLAVE_ADDR      (3)       // This slave's unique address
#define MB_PORT_NUM        (UART_NUM_2) // UART port to use
#define MB_BAUD_RATE       (19200)   // Baud rate for serial communication

// Define GPIO pins for UART
#define MB_UART_TXD (17)
#define MB_UART_RXD (16)
#define MB_UART_RTS (UART_PIN_NO_CHANGE) // RTS pin not used
#define MB_UART_CTS (UART_PIN_NO_CHANGE) // CTS pin not used

// This bit mask is used to listen for any read or write events from the master.
#define MB_EVENT_MASK      (MB_EVENT_HOLDING_REG_WR | MB_EVENT_HOLDING_REG_RD | \
                            MB_EVENT_INPUT_REG_RD   | MB_EVENT_COILS_WR       | \
                            MB_EVENT_COILS_RD       | MB_EVENT_DISCRETE_RD)

// Declare the storage arrays for the four register types
// These are the actual memory areas where the slave's data is held.
static uint8_t  discrete_input_area[MB_REG_DISCRETE_INPUT_CNT] = {0};
static uint8_t  coil_area[MB_REG_COILS_CNT] = {0};
static uint16_t input_reg_area[MB_REG_INPUT_CNT] = {0};
static uint16_t holding_reg_area[MB_REG_HOLD_CNT] = {0};


void app_main(void)
{
    // A handle for the Modbus controller
    void* mbc_slave_handler = NULL;

    // Initialize the Modbus controller for a serial slave
    esp_err_t err = mbc_slave_init(MB_PORT_SERIAL_SLAVE, &mbc_slave_handler);
    if (mbc_slave_handler == NULL || err != ESP_OK) {
        ESP_LOGE(TAG, "Modbus controller initialization failed.");
    }

    // Define communication parameters for the serial port
    mb_communication_info_t comm_info = {
        .mode       = MB_MODE_RTU,
        .slave_addr = MB_SLAVE_ADDR,
        .port       = MB_PORT_NUM,
        .baudrate   = MB_BAUD_RATE,
        .parity     = MB_PARITY_NONE
    };

    // Set the communication parameters
    ESP_ERROR_CHECK(mbc_slave_setup((void*)&comm_info));
    
    // Structure to describe a register area
    mb_register_area_descriptor_t reg_area;

    // Set up the Discrete Inputs area (%IX)
    reg_area.type = MB_PARAM_DISCRETE;
    reg_area.start_offset = MB_REG_DISCRETE_INPUT_START;
    reg_area.address = (void*)&discrete_input_area[0];
    reg_area.size = MB_REG_DISCRETE_INPUT_CNT; // Use the register count for size
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    // Set up the Coils area (%QX)
    reg_area.type = MB_PARAM_COIL;
    reg_area.start_offset = MB_REG_COILS_START;
    reg_area.address = (void*)&coil_area[0];
    reg_area.size = MB_REG_COILS_CNT;
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    // Set up the Input Registers area (%IW)
    reg_area.type = MB_PARAM_INPUT;
    reg_area.start_offset = MB_REG_INPUT_START;
    reg_area.address = (void*)&input_reg_area[0];
    reg_area.size = MB_REG_INPUT_CNT;
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    // Set up the Holding Registers area (%QW)
    reg_area.type = MB_PARAM_HOLDING;
    reg_area.start_offset = MB_REG_HOLD_START;
    reg_area.address = (void*)&holding_reg_area[0];
    reg_area.size = MB_REG_HOLD_CNT;
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    // Start the Modbus slave controller
    ESP_ERROR_CHECK(mbc_slave_start());

    // Configure UART pins for the selected port
    ESP_ERROR_CHECK(uart_set_pin(MB_PORT_NUM, MB_UART_TXD, MB_UART_RXD,
                                  MB_UART_RTS, MB_UART_CTS));

    // Set UART mode to RS485 half-duplex
    ESP_ERROR_CHECK(uart_set_mode(MB_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX));

    ESP_LOGI(TAG, "Modbus slave started with ID %d.", MB_SLAVE_ADDR);

    // This is the main loop where the slave listens for requests.
    // In a real application, you would update your sensor data here.
    while (1) {
        // Wait for a master to access one of the register areas
        mb_event_group_t event = mbc_slave_check_event(MB_EVENT_MASK);

        // If a write event occurred, log it
        if (event & (MB_EVENT_HOLDING_REG_WR | MB_EVENT_COILS_WR)) {
            mb_param_info_t reg_info;
            // Get information about the accessed register
            ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, 10));
            ESP_LOGI(TAG, "Master WRITE event: type=%d, offset=0x%x, size=%d",
                     (int)reg_info.type,
                     (int)reg_info.mb_offset,
                     (int)reg_info.size);
        }
    }
}