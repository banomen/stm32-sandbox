// Плата 2: сообщение от платы 1 по I2C (плата 2 — слейв) -> компьютер по UART.
//
//   I2C2 SCL   PB10 <- PA8 платы 1
//   I2C2 SDA   PB11 <> PC9 платы 1
//   USART2 TX  PA2  -> RX USB-UART переходника
//
// В ответ на компьютер уходит строка "board2: <сообщение>\r\n".
// Зелёный светодиод мигает на каждое принятое сообщение.

#include "common.hpp"
#include "i2c_slave.hpp"

constexpr uint8_t BOARD2_ADDR = 0x20;

int main() {
     clock_init();
     led_init();
     uart_init();
     i2c_slave_init(BOARD2_ADDR);

     Msg msg{};
     while (true) {
          if (i2c_slave_take(msg)) {
               uart_write_str("board2: ");
               uart_write(msg.data, msg.len);
               uart_write_str("\r\n");
               led_toggle(LED_GREEN);
          }
     }
}
