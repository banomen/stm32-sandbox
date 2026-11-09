#include <cstdint>

#include <libopencm3/stm32/rcc.h>

static void init_clock() {
     rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
}

int main() {
     init_clock();

     while (true) {
     }
}
