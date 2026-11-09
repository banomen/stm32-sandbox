// Плата 2: сообщение от платы 1 по I2C (плата 2 — слейв) -> компьютер по UART.
//
//   I2C1 SCL   PB6  <- PB6 платы 1
//   I2C1 SDA   PB9  <> PB9 платы 1
//   USART2 TX  PA2  -> RX USB-UART переходника
//
// Одно сообщение = одна транзакция записи мастера (START, адрес+W, байты, STOP).
// В ответ на компьютер уходит строка "board2: <сообщение>\r\n".
// Зелёный светодиод мигает на каждое принятое сообщение.

#include "common.hpp"
#include "i2c_bus.hpp"

#include <libopencm3/cm3/nvic.h>

constexpr uint8_t BOARD2_ADDR = 0x20;

static Msg i2c_rx;                        // собирается в прерывании
static Msg received;                      // готовое сообщение для main
static volatile bool received_ready = false;

extern "C" void i2c1_ev_isr() {
     const uint32_t sr1 = I2C_SR1(I2C1);

     if (sr1 & I2C_SR1_ADDR) { // мастер обратился к нам: начало сообщения
          (void)I2C_SR2(I2C1); // чтение SR1 + SR2 сбрасывает ADDR
          i2c_rx.len = 0;
     }

     if (sr1 & I2C_SR1_RxNE) {
          const auto byte = static_cast<uint8_t>(I2C_DR(I2C1));
          if (i2c_rx.len < MSG_MAX) {
               i2c_rx.data[i2c_rx.len++] = byte;
          }
     }

     if (sr1 & I2C_SR1_TxE) { // мастер читает — отдавать нечего
          I2C_DR(I2C1) = 0xFF;
     }

     if (sr1 & I2C_SR1_STOPF) { // конец сообщения
          I2C_CR1(I2C1) |= I2C_CR1_PE; // чтение SR1 + запись CR1 сбрасывает STOPF
          if (i2c_rx.len > 0 && !received_ready) { // если main ещё не забрал прошлое — новое теряется
               received = i2c_rx;
               received_ready = true;
          }
          i2c_rx.len = 0;
     }
}

extern "C" void i2c1_er_isr() {
     const uint32_t sr1 = I2C_SR1(I2C1);
     if (sr1 & (I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR)) {
          i2c_rx.len = 0; // сообщение испорчено
     }
     // AF — нормальное окончание, когда мастер читал с нас
     I2C_SR1(I2C1) = sr1 & ~(I2C_SR1_AF | I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR);
}

static void i2c_slave_init() {
     i2c_bus_init();
     i2c_set_own_7bit_slave_address(I2C1, BOARD2_ADDR);
     i2c_enable_interrupt(I2C1, I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);
     nvic_enable_irq(NVIC_I2C1_EV_IRQ);
     nvic_enable_irq(NVIC_I2C1_ER_IRQ);

     i2c_peripheral_enable(I2C1);
     i2c_enable_ack(I2C1); // ACK можно включить только при включённом модуле
}

int main() {
     clock_init();
     led_init();
     uart_init();
     i2c_slave_init();

     while (true) {
          if (received_ready) {
               compiler_barrier();
               uart_write_str("board2: ");
               uart_write(received.data, received.len);
               uart_write_str("\r\n");
               compiler_barrier();
               received_ready = false;
               led_toggle(LED_GREEN);
          }
     }
}
