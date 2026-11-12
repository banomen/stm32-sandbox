// Плата 2: сообщение от платы 1 по SPI (плата 2 — слейв) -> компьютер по UART.
//
// SPI2 NSS   PB12 <- PB12 платы 1 (пока NSS = 1, SPI не принимает такты — защита от помех)
// SPI2 SCK   PB13 <- PB13 платы 1
// SPI2 MISO  PB14 -> PB14 платы 1 (не используется, слейв шлёт нули)
// SPI2 MOSI  PB15 <- PB15 платы 1
// USART2 TX  PA2  -> RX USB-UART переходника
//
// Посылка: байт длины (1..MSG_MAX), затем столько же байтов сообщения.
// В ответ на компьютер уходит строка "board2: <сообщение>\r\n".
// Зелёный светодиод мигает на каждое принятое сообщение.

#include "common.hpp"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/spi.h>

static Msg spi_rx;                        // собирается в прерывании
static size_t expected_len = 0;           // 0 — ждём байт длины
static Msg received;                      // готовое сообщение для main
static volatile bool received_ready = false;

extern "C" void spi2_isr() {
     if ((SPI_SR(SPI2) & SPI_SR_RXNE) == 0) {
          return;
     }
     const auto byte = static_cast<uint8_t>(SPI_DR(SPI2));

     if (expected_len == 0) {
          if (byte == 0 || byte > MSG_MAX) {
               return;
          }
          expected_len = byte;
          spi_rx.len = 0;
          return;
     }

     spi_rx.data[spi_rx.len++] = byte;
     if (spi_rx.len == expected_len) {
          if (!received_ready) {
               received = spi_rx;
               received_ready = true;
          }
          expected_len = 0;
     }
}

static void spi_slave_init() {
     rcc_periph_clock_enable(RCC_GPIOB);
     rcc_periph_clock_enable(RCC_SPI2);

     gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO13 | GPIO14 | GPIO15);
     gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO12);
     gpio_set_af(GPIOB, GPIO_AF5, GPIO12 | GPIO13 | GPIO14 | GPIO15);

     rcc_periph_reset_pulse(RST_SPI2);
     spi_set_slave_mode(SPI2);
     spi_set_dff_8bit(SPI2);
     spi_set_clock_polarity_0(SPI2);
     spi_set_clock_phase_0(SPI2);
     spi_send_msb_first(SPI2);
     spi_disable_software_slave_management(SPI2);
     spi_enable_rx_buffer_not_empty_interrupt(SPI2);

     nvic_enable_irq(NVIC_SPI2_IRQ);
     spi_enable(SPI2);
}

int main() {
     clock_init();
     led_init();
     uart_init();
     spi_slave_init();

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
