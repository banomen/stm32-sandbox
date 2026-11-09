// Плата 2: сообщение от платы 1 по SPI (плата 2 — слейв) -> компьютер по UART.
//
// SPI2 NSS   PB12 <- PB12 платы 1 (аппаратный NSS + прерывание по фронту = конец сообщения)
// SPI2 SCK   PB13 <- PB13 платы 1
// SPI2 MISO  PB14 -> PB14 платы 1 (не используется, слейв шлёт нули)
// SPI2 MOSI  PB15 <- PB15 платы 1
// USART2 TX  PA2  -> RX USB-UART переходника
//
// В ответ на компьютер уходит строка "board2: <сообщение>\r\n".
// Зелёный светодиод мигает на каждое принятое сообщение.

#include "common.hpp"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/spi.h>

static Msg spi_rx;                        // собирается в прерывании
static Msg received;                      // готовое сообщение для main
static volatile bool received_ready = false;

static void spi_take_byte() {
     if (SPI_SR(SPI2) & SPI_SR_RXNE) {
          const auto byte = static_cast<uint8_t>(SPI_DR(SPI2));
          if (spi_rx.len < MSG_MAX) {
               spi_rx.data[spi_rx.len++] = byte;
          }
     }
}

extern "C" void spi2_isr() {
     spi_take_byte();
}

// Фронт NSS (PB12): мастер закончил посылку
extern "C" void exti15_10_isr() {
     exti_reset_request(EXTI12);
     spi_take_byte(); // последний байт, если его прерывание ещё не обработано

     if (spi_rx.len > 0) {
          if (!received_ready) { // если main ещё не забрал прошлое — новое теряется
               received = spi_rx;
               received_ready = true;
          }
          spi_rx.len = 0;
     }
}

static void spi_slave_init() {
     rcc_periph_clock_enable(RCC_GPIOB);
     rcc_periph_clock_enable(RCC_SPI2);
     rcc_periph_clock_enable(RCC_SYSCFG);

     gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO13 | GPIO14 | GPIO15);
     gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO12); // без мастера NSS неактивен
     gpio_set_af(GPIOB, GPIO_AF5, GPIO12 | GPIO13 | GPIO14 | GPIO15);

     rcc_periph_reset_pulse(RST_SPI2);
     spi_set_slave_mode(SPI2);
     spi_set_dff_8bit(SPI2);
     spi_set_clock_polarity_0(SPI2);
     spi_set_clock_phase_0(SPI2);
     spi_send_msb_first(SPI2);
     spi_disable_software_slave_management(SPI2); // NSS с ножки PB12
     spi_enable_rx_buffer_not_empty_interrupt(SPI2);

     // Байты SPI важнее конца посылки: их прерывание с более высоким приоритетом
     nvic_set_priority(NVIC_SPI2_IRQ, 0x00);
     nvic_enable_irq(NVIC_SPI2_IRQ);
     spi_enable(SPI2);

     exti_select_source(EXTI12, GPIOB);
     exti_set_trigger(EXTI12, EXTI_TRIGGER_RISING);
     exti_enable_request(EXTI12);
     nvic_set_priority(NVIC_EXTI15_10_IRQ, 0x40);
     nvic_enable_irq(NVIC_EXTI15_10_IRQ);
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
