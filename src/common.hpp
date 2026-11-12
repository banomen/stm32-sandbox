// Общее для обеих плат: тактирование, светодиоды, UART к компьютеру.
#pragma once

#include <cstddef>
#include <cstdint>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

// Максимальная длина одного сообщения
constexpr size_t MSG_MAX = 64;

// Адрес платы 2 на шине I2C (7 бит). 0x4A занят аудиокодеком на Discovery.
constexpr uint8_t BOARD2_ADDR = 0x20;

struct Msg {
     uint8_t data[MSG_MAX];
     size_t len;
};

// Барьер для компилятора: данные, записанные в прерывании, читаются заново
inline void compiler_barrier() {
     __asm__ volatile("" ::: "memory");
}

// 168 МГц от кварца 8 МГц (STM32F4DISCOVERY). APB1 = 42 МГц, APB2 = 84 МГц.
inline void clock_init() {
     rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
}

// Светодиоды платы: PD12 зелёный, PD13 оранжевый, PD14 красный, PD15 синий
constexpr uint16_t LED_GREEN = GPIO12;
constexpr uint16_t LED_ORANGE = GPIO13;
constexpr uint16_t LED_RED = GPIO14;
constexpr uint16_t LED_BLUE = GPIO15;

inline void led_init() {
     rcc_periph_clock_enable(RCC_GPIOD);
     gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_GREEN | LED_ORANGE | LED_RED | LED_BLUE);
}

inline void led_toggle(uint16_t led) {
     gpio_toggle(GPIOD, led);
}

inline void led_on(uint16_t led) {
     gpio_set(GPIOD, led);
}

// USART2 к компьютеру (через USB-UART переходник): PA2 = TX, PA3 = RX, 115200 8N1
constexpr uint32_t UART_BAUD = 115'200;

inline void uart_init() {
     rcc_periph_clock_enable(RCC_GPIOA);
     rcc_periph_clock_enable(RCC_USART2);

     gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO2 | GPIO3);
     gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);

     usart_set_baudrate(USART2, UART_BAUD);
     usart_set_databits(USART2, 8);
     usart_set_stopbits(USART2, USART_STOPBITS_1);
     usart_set_parity(USART2, USART_PARITY_NONE);
     usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
     usart_set_mode(USART2, USART_MODE_TX_RX);
     usart_enable(USART2);
}

inline void uart_write(const uint8_t* data, size_t len) {
     for (size_t i = 0; i < len; i++) {
          usart_send_blocking(USART2, data[i]);
     }
}

inline void uart_write_str(const char* s) {
     while (*s != '\0') {
          usart_send_blocking(USART2, static_cast<uint8_t>(*s++));
     }
}
