#include <pico/stdio.h>
#include <hardware/gpio.h>
#include <pico/time.h>
#include <pico/printf.h>
#include "pico/multicore.h"
#include <math.h>
#include <malloc.h>
#include "framebuffer.h"
#include "gif_decoder.h"
#include "animations/animations.h"

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
#define DISPLAY_BPP 32

framebuffer_t fb;
framebuffer_config_t framebuffer_config = {
    R0,G0, B0,
    R1, G1, B1,
    CLK, LAT, OE,
    A, B, C,
    DISPLAY_W, DISPLAY_H, DISPLAY_BPP,
    .oe_inverted = false // LOW = off
};

static void core1_entry();

bool timer_callback(repeating_timer_t *user_data) {
    // plasma_update(&_fb);
    gif_animation_update(&fb);

    return true;
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

    if (framebuffer_init(framebuffer_config, &fb) != FRAMEBUFFER_OK) {
        return -1;
    }

    // Enable led on boot
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    //plasma_init(&_fb);
    gif_animation_init(&fb);

    repeating_timer_t timer;
    add_repeating_timer_ms(60, &timer_callback, NULL, &timer);

    // multicore_launch_core1(core1_entry);
    while (1) {
        framebuffer_sync(&fb);
        tight_loop_contents();
    }
}

static void core1_entry() {
    while (1) {
        framebuffer_sync(&fb);
    }
}

