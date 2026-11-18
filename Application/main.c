
 // SPDX-License-Identifier: MIT
 // Copyright (c) 2020-2025 RVX Project Contributors

#include "libsteel.h"

#define DEFAULT_UART (UartController *)0x80000000
#define DEFAULT_GPIO (GpioController *)0x80020000

#define BLINK_DELAY 500000

volatile char uart_command = 0;
volatile uint32_t command_received = 0;

volatile uint32_t led_state = 0;
volatile uint32_t blink_enable = 0;

// --------------------------------------------------
// UART Fast IRQ #0
// --------------------------------------------------
__NAKED void fast0_irq_handler(void)
{
    char rx = uart_read(DEFAULT_UART);

    if (rx == '\r' || rx == '\n')
    {
        uart_command = '\n';
        command_received = 1;
    }
    else if (rx >= 32 && rx <= 126)
    {
        uart_command = rx;
        command_received = 1;
    }

    __ASM_VOLATILE("mret");
}

// --------------------------------------------------
// Display UART commands
// --------------------------------------------------
void display_help(void)
{
    uart_write_string(DEFAULT_UART,
        "\nAvailable Commands:\n"
        "  1 - LED ON\n"
        "  0 - LED OFF\n"
        "  T - Toggle LED\n"
        "  B - Start LED Blinking\n"
        "  X - Stop LED Blinking\n"
        "  S - LED Status\n"
        "  H - Display Help\n");
}

// --------------------------------------------------
// Main program
// --------------------------------------------------
void main(void)
{
    uint32_t blink_counter = 0;

    // GPIO initialization
    // GPIO[0] = Enable
    // GPIO[1] = LED
    // GPIO[2] = Input

    gpio_set_output(DEFAULT_GPIO, 0);
    gpio_set_output(DEFAULT_GPIO, 1);
    gpio_set_input(DEFAULT_GPIO, 2);

    // Keep GPIO enable HIGH
    gpio_write(DEFAULT_GPIO, 0, HIGH);

    // LED initially OFF
    gpio_write(DEFAULT_GPIO, 1, LOW);

    // UART startup message
    uart_write_string(DEFAULT_UART,
        "\n================================\n"
        "       RISC-V STEEL DEMO\n"
        "================================\n"
        "\nUART GPIO LED CONTROL\n");

    uart_write_string(DEFAULT_UART,
        "\nGPIO Configuration:\n"
        "  GPIO[0] - Enable\n"
        "  GPIO[1] - LED\n"
        "  GPIO[2] - Input\n");

    display_help();

    uart_write_string(DEFAULT_UART,
        "\nEnter Command: ");

    // Enable UART Fast IRQ #0
    csr_enable_vectored_mode_irq();
    CSR_SET(CSR_MIE, MIP_MIE_MASK_F0I);
    csr_global_enable_irq();

    while (1)
    {
        // ------------------------------------------
        // Process UART commands
        // ------------------------------------------
        if (command_received)
        {
            char cmd = uart_command;
            command_received = 0;

            switch (cmd)
            {
                // LED ON
                case '1':
                    blink_enable = 0;
                    led_state = 1;

                    gpio_write(DEFAULT_GPIO, 1, HIGH);

                    uart_write_string(DEFAULT_UART,
                        "\n[GPIO] LED ON\n");
                    break;

                // LED OFF
                case '0':
                    blink_enable = 0;
                    led_state = 0;

                    gpio_write(DEFAULT_GPIO, 1, LOW);

                    uart_write_string(DEFAULT_UART,
                        "\n[GPIO] LED OFF\n");
                    break;

                // Toggle LED
                case 'T':
                case 't':
                    blink_enable = 0;
                    led_state = !led_state;

                    gpio_write(DEFAULT_GPIO, 1,
                        led_state ? HIGH : LOW);

                    uart_write_string(DEFAULT_UART,
                        led_state ?
                        "\n[GPIO] LED ON\n" :
                        "\n[GPIO] LED OFF\n");
                    break;

                // Start blinking
                case 'B':
                case 'b':
                    blink_enable = 1;
                    blink_counter = 0;

                    uart_write_string(DEFAULT_UART,
                        "\n[GPIO] LED BLINKING STARTED\n");
                    break;

                // Stop blinking
                case 'X':
                case 'x':
                    blink_enable = 0;
                    led_state = 0;

                    gpio_write(DEFAULT_GPIO, 1, LOW);

                    uart_write_string(DEFAULT_UART,
                        "\n[GPIO] LED BLINKING STOPPED\n");
                    break;

                // LED status
                case 'S':
                case 's':
                    uart_write_string(DEFAULT_UART,
                        blink_enable ?
                        "\n[GPIO] LED Mode: BLINKING\n" :
                        (led_state ?
                        "\n[GPIO] LED Status: ON\n" :
                        "\n[GPIO] LED Status: OFF\n"));
                    break;

                // Display help
                case 'H':
                case 'h':
                    display_help();
                    break;

                // Enter key
                case '\n':
                    break;

                // Invalid command
                default:
                    uart_write_string(DEFAULT_UART,
                        "\n[UART] Invalid Command. Press H for Help.\n");
                    break;
            }

            uart_write_string(DEFAULT_UART,
                "\nEnter Command: ");
        }

        // ------------------------------------------
        // Automatic LED blinking
        // ------------------------------------------
        if (blink_enable)
        {
            blink_counter++;

            if (blink_counter >= BLINK_DELAY)
            {
                blink_counter = 0;

                led_state = !led_state;

                gpio_write(DEFAULT_GPIO, 1,
                    led_state ? HIGH : LOW);
            }
        }
    }
}
