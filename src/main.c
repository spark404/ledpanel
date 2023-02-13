#include <pico/stdio.h>
#include <hardware/gpio.h>
#include <hardware/timer.h>
#include <strings.h>
#include <memory.h>
#include "framebuffer.h"

#define B0 11
#define G0 12
#define R0 13
#define B1 7
#define G1 8
#define R1 9
#define CLK 16
#define C 18
#define B 19
#define A 20
#define OE 21
#define LAT 22

#define DISPLAY_W 32
#define DISPLAY_H 16
#define DISPLAY_BPP 16

framebuffer_t framebuffer;
framebuffer_config_t framebuffer_config = {
    R0,G0, B0,
    R1, G1, B1,
    CLK, LAT, OE,
    A, B, C
};

// Rainbow
void rainbow_init(framebuffer_t *framebuffer);

int main(void) {
    stdio_init_all();

    if (framebuffer_init(framebuffer_config, &framebuffer) != FRAMEBUFFER_OK) {
        return -1;
    }

    // Enable the boot
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    rainbow_init(&framebuffer);

    while (1) {
        framebuffer_sync(&framebuffer);
    }
}

void rainbow_init(framebuffer_t *framebuffer) {
    uint8_t *ptr = framebuffer->buffer;
    for (int i=0; i<32; i++) {
        *(ptr+(i*2)) = 0x00; *(ptr+(i*2 + 1)) = 0x00;
        *(ptr+(1*32*2+i*2)) = 0xFF; *(ptr+(1*32*2+i*2 + 1)) = 0xFF;
        *(ptr+(2*32*2+i*2)) = 0xF8; *(ptr+(2*32*2+i*2 + 1)) = 0x00;
        *(ptr+(3*32*2+i*2)) = 0x07; *(ptr+(3*32*2+i*2 + 1)) = 0xE0;
        *(ptr+(4*32*2+i*2)) = 0x00; *(ptr+(4*32*2+i*2 + 1)) = 0x1F;
    }
}