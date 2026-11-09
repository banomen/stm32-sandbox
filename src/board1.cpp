// Плата 1: сообщение с компьютера по I2C -> плата 2 по I2C.
//
// Шина с компьютером (плата 1 — слейв с адресом BOARD1_ADDR):
//   I2C2 SCL  PB10 <- SCL USB-I2C переходника
//   I2C2 SDA  PB11 <> SDA USB-I2C переходника
// Шина с платой 2 (плата 1 — мастер):
//   I2C3 SCL  PA8  -> PB10 платы 2
//   I2C3 SDA  PC9  <> PB11 платы 2
//
// Одна транзакция записи от компьютера пересылается плате 2 одной транзакцией записи.
// Зелёный светодиод мигает на каждое переданное сообщение,
// красный загорается, если плата 2 не ответила.

#include "common.hpp"
#include "i2c_slave.hpp"

#include <libopencm3/stm32/i2c.h>

constexpr uint8_t BOARD1_ADDR = 0x10; // адрес платы 1 для компьютера
constexpr uint8_t BOARD2_ADDR = 0x20; // адрес платы 2

static void i2c_master_init() {
     rcc_periph_clock_enable(RCC_GPIOA);
     rcc_periph_clock_enable(RCC_GPIOC);
     rcc_periph_clock_enable(RCC_I2C3);

     // Внутренние подтяжки слабые (~40 кОм), лучше поставить внешние 4.7 кОм к 3.3 В
     gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO8);
     gpio_set_output_options(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, GPIO8);
     gpio_set_af(GPIOA, GPIO_AF4, GPIO8);
     gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO9);
     gpio_set_output_options(GPIOC, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, GPIO9);
     gpio_set_af(GPIOC, GPIO_AF4, GPIO9);

     rcc_periph_reset_pulse(RST_I2C3);
     i2c_set_speed(I2C3, i2c_speed_sm_100k, rcc_apb1_frequency / 1'000'000);
     i2c_peripheral_enable(I2C3);
}

// Ждать, пока в SR1 появится хотя бы один из битов. false — не дождались.
static bool i2c3_wait_sr1(uint32_t bits) {
     for (uint32_t i = 0; i < 200'000; i++) { // ~ несколько мс
          if (I2C_SR1(I2C3) & bits) {
               return true;
          }
     }
     return false;
}

// Запись сообщения слейву. В отличие от i2c_transfer7 из libopencm3 не зависает,
// если слейв не отвечает: false при NACK или таймауте.
static bool i2c3_write(uint8_t address, const uint8_t* data, size_t len) {
     for (uint32_t i = 0; I2C_SR2(I2C3) & I2C_SR2_BUSY; i++) {
          if (i > 200'000) {
               return false;
          }
     }

     i2c_send_start(I2C3);
     if (!i2c3_wait_sr1(I2C_SR1_SB)) {
          return false;
     }

     i2c_send_7bit_address(I2C3, address, I2C_WRITE);
     if (!i2c3_wait_sr1(I2C_SR1_ADDR | I2C_SR1_AF) || (I2C_SR1(I2C3) & I2C_SR1_AF)) {
          I2C_SR1(I2C3) &= ~I2C_SR1_AF;
          i2c_send_stop(I2C3);
          return false;
     }
     (void)I2C_SR2(I2C3); // сброс ADDR

     for (size_t i = 0; i < len; i++) {
          i2c_send_data(I2C3, data[i]);
          if (!i2c3_wait_sr1(I2C_SR1_TxE | I2C_SR1_AF) || (I2C_SR1(I2C3) & I2C_SR1_AF)) {
               I2C_SR1(I2C3) &= ~I2C_SR1_AF;
               i2c_send_stop(I2C3);
               return false;
          }
     }

     const bool ok = i2c3_wait_sr1(I2C_SR1_BTF); // последний байт ушёл целиком
     i2c_send_stop(I2C3);
     return ok;
}

int main() {
     clock_init();
     led_init();
     i2c_slave_init(BOARD1_ADDR);
     i2c_master_init();

     Msg msg{};
     while (true) {
          if (i2c_slave_take(msg)) {
               if (i2c3_write(BOARD2_ADDR, msg.data, msg.len)) {
                    led_toggle(LED_GREEN);
               } else {
                    led_on(LED_RED);
                    i2c_master_init(); // сброс шины после ошибки
               }
          }
     }
}
