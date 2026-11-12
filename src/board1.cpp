// Плата 1: строка с компьютера по UART -> плата 2 по I2C (плата 1 — мастер).
//
//   USART2 RX  PA3  <- TX USB-UART переходника
//   I2C1 SCL   PB6  -> PB6 платы 2
//   I2C1 SDA   PB9  <> PB9 платы 2
//
// Сообщение — строка до '\n' или '\r' (без самого символа), не длиннее MSG_MAX.
// Одно сообщение = одна транзакция записи I2C на адрес платы 2:
// START, адрес+W, байт длины (1..MSG_MAX), байты сообщения, STOP.
// Зелёный светодиод мигает на каждое переданное сообщение,
// красный загорается, если плата 2 не ответила.

#include "common.hpp"
#include "i2c_bus.hpp"

#include <libopencm3/cm3/nvic.h>

static Msg uart_rx;                       // собирается в прерывании
static Msg to_send;                       // готовое сообщение для main
static volatile bool to_send_ready = false;

extern "C" void usart2_isr() {
     if ((USART_SR(USART2) & (USART_SR_RXNE | USART_SR_ORE)) == 0) {
          return;
     }
     const auto byte = static_cast<uint8_t>(usart_recv(USART2));

     const bool line_end = (byte == '\n' || byte == '\r');
     if (!line_end) {
          uart_rx.data[uart_rx.len++] = byte;
     }

     if ((line_end && uart_rx.len > 0) || uart_rx.len == MSG_MAX) {
          if (!to_send_ready) {
               to_send = uart_rx;
               to_send_ready = true;
          }
          uart_rx.len = 0;
     }
}

static void i2c_master_init() {
     i2c_bus_init();
     i2c_peripheral_enable(I2C1);
}

// Ждать, пока в SR1 появится хотя бы один из битов. false — не дождались.
static bool i2c_wait_sr1(uint32_t bits) {
     for (uint32_t i = 0; i < 200'000; i++) { // ~ несколько мс
          if (I2C_SR1(I2C1) & bits) {
               return true;
          }
     }
     return false;
}

// Отправить один байт данных. false — слейв ответил NACK или таймаут.
static bool i2c_tx_byte(uint8_t byte) {
     i2c_send_data(I2C1, byte);
     return i2c_wait_sr1(I2C_SR1_TxE | I2C_SR1_AF) && (I2C_SR1(I2C1) & I2C_SR1_AF) == 0;
}

static bool i2c_write(uint8_t address, const uint8_t* data, size_t len) {
     for (uint32_t i = 0; I2C_SR2(I2C1) & I2C_SR2_BUSY; i++) {
          if (i > 200'000) {
               return false;
          }
     }

     i2c_send_start(I2C1);
     if (!i2c_wait_sr1(I2C_SR1_SB)) {
          return false;
     }

     i2c_send_7bit_address(I2C1, address, I2C_WRITE);
     if (!i2c_wait_sr1(I2C_SR1_ADDR | I2C_SR1_AF) || (I2C_SR1(I2C1) & I2C_SR1_AF)) {
          I2C_SR1(I2C1) &= ~I2C_SR1_AF;
          i2c_send_stop(I2C1);
          return false;
     }
     (void)I2C_SR2(I2C1); // сброс ADDR

     bool ok = i2c_tx_byte(static_cast<uint8_t>(len));
     for (size_t i = 0; ok && i < len; i++) {
          ok = i2c_tx_byte(data[i]);
     }
     if (!ok) {
          I2C_SR1(I2C1) &= ~I2C_SR1_AF;
          i2c_send_stop(I2C1);
          return false;
     }

     ok = i2c_wait_sr1(I2C_SR1_BTF);
     i2c_send_stop(I2C1);
     return ok;
}

int main() {
     clock_init();
     led_init();
     uart_init();
     i2c_master_init();

     usart_enable_rx_interrupt(USART2);
     nvic_enable_irq(NVIC_USART2_IRQ);

     while (true) {
          if (to_send_ready) {
               compiler_barrier();
               const bool ok = i2c_write(BOARD2_ADDR, to_send.data, to_send.len);
               compiler_barrier();
               to_send_ready = false;

               if (ok) {
                    led_toggle(LED_GREEN);
               } else {
                    led_on(LED_RED);
                    i2c_master_init();
               }
          }
     }
}
