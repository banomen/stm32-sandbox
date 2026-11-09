// Шина I2C между платами: I2C1, PB6 = SCL, PB9 = SDA, 100 кГц.
//
// На STM32F4DISCOVERY эти ножки уже подтянуты к 3.3 В резисторами на плате
// (на шине висит аудиокодек CS43L22), поэтому внешние резисторы не нужны.
// Кодек держим в сбросе (PD4 = 0), чтобы он не мешал. Его адрес 0x4A не занимать.
#pragma once

#include "common.hpp"

#include <libopencm3/stm32/i2c.h>

inline void i2c_bus_init() {
     rcc_periph_clock_enable(RCC_GPIOB);
     rcc_periph_clock_enable(RCC_GPIOD);
     rcc_periph_clock_enable(RCC_I2C1);

     gpio_clear(GPIOD, GPIO4); // сброс CS43L22
     gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4);

     gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO6 | GPIO9);
     gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, GPIO6 | GPIO9);
     gpio_set_af(GPIOB, GPIO_AF4, GPIO6 | GPIO9);

     rcc_periph_reset_pulse(RST_I2C1);
     i2c_set_speed(I2C1, i2c_speed_sm_100k, rcc_apb1_frequency / 1'000'000);
}
