#include "i2c_slave.hpp"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/i2c.h>

static Msg rx;                            // собирается в прерывании
static Msg received;                      // готовое сообщение для main
static volatile bool received_ready = false;

void i2c_slave_init(uint8_t address) {
     rcc_periph_clock_enable(RCC_GPIOB);
     rcc_periph_clock_enable(RCC_I2C2);

     // Внутренние подтяжки слабые (~40 кОм), лучше поставить внешние 4.7 кОм к 3.3 В
     gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO10 | GPIO11);
     gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, GPIO10 | GPIO11);
     gpio_set_af(GPIOB, GPIO_AF4, GPIO10 | GPIO11);

     rcc_periph_reset_pulse(RST_I2C2);
     i2c_set_speed(I2C2, i2c_speed_sm_100k, rcc_apb1_frequency / 1'000'000);
     i2c_set_own_7bit_slave_address(I2C2, address);
     i2c_enable_interrupt(I2C2, I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);
     nvic_enable_irq(NVIC_I2C2_EV_IRQ);
     nvic_enable_irq(NVIC_I2C2_ER_IRQ);

     i2c_peripheral_enable(I2C2);
     i2c_enable_ack(I2C2); // ACK можно включить только при включённом модуле
}

bool i2c_slave_take(Msg& out) {
     if (!received_ready) {
          return false;
     }
     compiler_barrier();
     out = received;
     compiler_barrier();
     received_ready = false;
     return true;
}

extern "C" void i2c2_ev_isr() {
     const uint32_t sr1 = I2C_SR1(I2C2);

     if (sr1 & I2C_SR1_ADDR) { // мастер обратился к нам: начало сообщения
          (void)I2C_SR2(I2C2); // чтение SR1 + SR2 сбрасывает ADDR
          rx.len = 0;
     }

     if (sr1 & I2C_SR1_RxNE) {
          const auto byte = static_cast<uint8_t>(I2C_DR(I2C2));
          if (rx.len < MSG_MAX) {
               rx.data[rx.len++] = byte;
          }
     }

     if (sr1 & I2C_SR1_TxE) { // мастер читает — отдавать нечего
          I2C_DR(I2C2) = 0xFF;
     }

     if (sr1 & I2C_SR1_STOPF) { // конец сообщения
          I2C_CR1(I2C2) |= I2C_CR1_PE; // чтение SR1 + запись CR1 сбрасывает STOPF
          if (rx.len > 0 && !received_ready) { // если main ещё не забрал прошлое — новое теряется
               received = rx;
               received_ready = true;
          }
          rx.len = 0;
     }
}

extern "C" void i2c2_er_isr() {
     const uint32_t sr1 = I2C_SR1(I2C2);
     if (sr1 & (I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR)) {
          rx.len = 0; // сообщение испорчено
     }
     // AF — нормальное окончание, когда мастер читал с нас
     I2C_SR1(I2C2) = sr1 & ~(I2C_SR1_AF | I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR);
}
