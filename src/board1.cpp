// Плата 1: строка с компьютера по UART -> плата 2 по SPI (плата 1 — мастер).
//
// USART2 RX  PA3  <- TX USB-UART переходника
// SPI2 NSS   PB12 -> PB12 платы 2 (выбор слейва, управляется вручную)
// SPI2 SCK   PB13 -> PB13 платы 2
// SPI2 MISO  PB14 <- PB14 платы 2
// SPI2 MOSI  PB15 -> PB15 платы 2
//
// Сообщение — строка до '\n' или '\r' (без самого символа), не длиннее MSG_MAX.
// Посылка SPI: сначала байт длины (1..MSG_MAX), затем сами байты сообщения.
// На время посылки NSS прижат к земле.
// Зелёный светодиод мигает на каждое отправленное сообщение.

#include "common.hpp"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/spi.h>

static Msg uart_rx;
static Msg to_send;
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

static void spi_master_init() {
     rcc_periph_clock_enable(RCC_GPIOB);
     rcc_periph_clock_enable(RCC_SPI2);

     gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO13 | GPIO14 | GPIO15);
     gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO13 | GPIO15);
     gpio_set_af(GPIOB, GPIO_AF5, GPIO13 | GPIO14 | GPIO15);

     gpio_set(GPIOB, GPIO12);
     gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);

     rcc_periph_reset_pulse(RST_SPI2);
     // 42 МГц / 64 = ~656 кГц
     spi_init_master(SPI2, SPI_CR1_BAUDRATE_FPCLK_DIV_64, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                     SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
     spi_enable_software_slave_management(SPI2);
     spi_set_nss_high(SPI2);
     spi_enable(SPI2);
}

static void spi_send_msg(const Msg& msg) {
     gpio_clear(GPIOB, GPIO12);
     (void)spi_xfer(SPI2, static_cast<uint8_t>(msg.len)); // по нему плата 2 узнает, сколько ждать
     for (size_t i = 0; i < msg.len; i++) {
          (void)spi_xfer(SPI2, msg.data[i]);
     }
     while (SPI_SR(SPI2) & SPI_SR_BSY) {
     }
     gpio_set(GPIOB, GPIO12);
}

int main() {
     clock_init();
     led_init();
     uart_init();
     spi_master_init();

     usart_enable_rx_interrupt(USART2);
     nvic_enable_irq(NVIC_USART2_IRQ);

     while (true) {
          if (to_send_ready) {
               compiler_barrier();
               spi_send_msg(to_send);
               compiler_barrier();
               to_send_ready = false;
               led_toggle(LED_GREEN);
          }
     }
}
