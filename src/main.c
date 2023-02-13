#include <pico/stdio.h>
#include <hardware/gpio.h>
#include <pico/time.h>
#include <pico/printf.h>
#include "framebuffer.h"
#include "rgb565.h"

#define B0 7
#define G0 8
#define R0 9
#define B1 11
#define G1 12
#define R1 13
#define CLK 16
#define C 18
#define B 19
#define A 20
#define OE 21
#define LAT 22

#define DISPLAY_W 32
#define DISPLAY_H 16
#define DISPLAY_BPP 16

framebuffer_t _fb;
framebuffer_config_t framebuffer_config = {
    R0,G0, B0,
    R1, G1, B1,
    CLK, LAT, OE,
    A, B, C,
    DISPLAY_W, DISPLAY_H, DISPLAY_BPP
};

// Rainbow
void rainbow_init(framebuffer_t *framebuffer);
void bright_init(framebuffer_t *framebuffer);
void bright_test_init(framebuffer_t *framebuffer);
void bright_test_update(framebuffer_t *framebuffer);

bool timer_callback(repeating_timer_t *user_data) {
    bright_test_update(&_fb);
    return true;
}

int main(void) {
    stdio_init_all();

    if (framebuffer_init(framebuffer_config, &_fb) != FRAMEBUFFER_OK) {
        return -1;
    }

    // Enable the boot
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    // rainbow_init(&_fb);
     bright_test_init(&_fb);
     repeating_timer_t timer;
    add_repeating_timer_ms(100, &timer_callback, NULL, &timer);

    while (1) {
        framebuffer_sync(&_fb);
    }
}

void rainbow_init(framebuffer_t *framebuffer) {
    for (int i=0 ; i<32; i++) {
        framebuffer_drawpixel(framebuffer, i, 0, White);
        framebuffer_drawpixel(framebuffer, i, 1, Red);
        framebuffer_drawpixel(framebuffer, i, 2, Green);
        framebuffer_drawpixel(framebuffer, i, 3, Blue);
        framebuffer_drawpixel(framebuffer, i, 4, White);
        framebuffer_drawpixel(framebuffer, i, 5, Cyan);
        framebuffer_drawpixel(framebuffer, i, 6, Magenta);
        framebuffer_drawpixel(framebuffer, i, 7, Yellow);
        framebuffer_drawpixel(framebuffer, i, 8, Olive);
        framebuffer_drawpixel(framebuffer, i, 9, Purple);
        framebuffer_drawpixel(framebuffer, i, 10, DarkCyan);
        framebuffer_drawpixel(framebuffer, i, 11, DarkGrey);
        framebuffer_drawpixel(framebuffer, i, 12, Navy);
        framebuffer_drawpixel(framebuffer, i, 13, DarkGreen);
        framebuffer_drawpixel(framebuffer, i, 14, Maroon);
        framebuffer_drawpixel(framebuffer, i, 15, DarkGrey);
    }
}

void bright_init(framebuffer_t *framebuffer) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 32; x++) {
            uint8_t color = y * 16;
            uint16_t rgb565 = ((color >> 3) << 11) |
                    ((color >> 2) << 5) |
                    (color >> 3);
            framebuffer_drawpixel(framebuffer, x, y, rgb565);
        }
    }
}

void bright_test_init(framebuffer_t *framebuffer) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 32; x++) {
            uint8_t color = y * 32;
            uint16_t rgb565 = ((color >> 3) << 11) |
                              ((color >> 2) << 5) |
                              (color >> 3);
            framebuffer_drawpixel(framebuffer, x, y, rgb565);
            framebuffer_drawpixel(framebuffer, x, y+8, rgb565);
        }
    }
}

int step = 0;
void bright_test_update(framebuffer_t *framebuffer) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 32; x++) {
            uint8_t color = y * 32;
            int y_pos = (y + step) % 8;
//            uint16_t rgb565 = ((color >> 3) << 11) |
//                              ((color >> 2) << 5) |
//                              (color >> 3);
            uint16_t rgb565 = (color >> 3);
            framebuffer_drawpixel(framebuffer, x, y_pos, rgb565);
            framebuffer_drawpixel(framebuffer, x, 15-y_pos, rgb565);
        }
    }
    step = (step + 1) % 8;
}