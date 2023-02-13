//
// Created by Hugo Trippaers on 13/02/2023.
//


#include <malloc.h>
#include <framebuffer.h>
#include <hardware/gpio.h>
#include <hardware/timer.h>

static void latch(framebuffer_t *framebuffer, int line);

int framebuffer_init(framebuffer_config_t config, framebuffer_t *framebuffer) {
    uint pins_mask = 1 << config.pin_r0 |
            1 << config.pin_g0 |
            1 << config.pin_b0 |
            1 << config.pin_r1 |
            1 << config.pin_g1 |
            1 << config.pin_b1 |
            1 << config.pin_clk |
            1 << config.pin_lat |
            1 << config.pin_oe |
            1 << config.pin_a |
            1 << config.pin_b |
            1 << config.pin_c;

    gpio_init_mask(pins_mask);
    gpio_set_dir_out_masked(pins_mask);
    gpio_clr_mask(pins_mask);

    gpio_set_pulls(framebuffer->config.pin_r0, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_g0, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_b0, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_clk, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_lat, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_oe, 0, 1);

    void *fb = malloc(config.w * config.h * (config.bpp / 8));
    if (fb == NULL) {
        return FRAMEBUFFER_ERROR;
    }
    framebuffer->buffer = fb;
    framebuffer->config = config;
    return FRAMEBUFFER_OK;
}

int framebuffer_sync(framebuffer_t *framebuffer) {
    for (int line = 0; line < (framebuffer->config.h / 2); line++) {
        // Unpack a pixel
        uint16_t pixel = *(uint16_t *)(framebuffer->buffer+line*32);
        int r = (pixel >> 11) & 0x1F;
        int g = (pixel >> 5) & 0x3F;
        int b = pixel & 0x1F;

        // Set the line values for the panel top half
        gpio_put(framebuffer->config.pin_r0, r);
        gpio_put(framebuffer->config.pin_b0, g);
        gpio_put(framebuffer->config.pin_g0, b);

        // Shift the register into the shifter
        gpio_put(framebuffer->config.pin_clk, 1);
        busy_wait_us(10); // TODO this could use some tweaking
        gpio_put(framebuffer->config.pin_clk, 0);

        // Trigger the latch
        latch(framebuffer, line);
    }

    return FRAMEBUFFER_OK;
}

static void latch(framebuffer_t *framebuffer, int line) {
    // Set output enable HIGH to turn of the display
    gpio_put(framebuffer->config.pin_oe, 0);

    // Select line to latch
    gpio_put(framebuffer->config.pin_a, line & 0x1);
    gpio_put(framebuffer->config.pin_b, (line & 0x2) >> 1);
    gpio_put(framebuffer->config.pin_c, (line & 0x4) >> 2);

    // Trigger the latch by pulsing LAT to HIGH
    gpio_put(framebuffer->config.pin_lat, 1);
    busy_wait_us(50);
    gpio_put(framebuffer->config.pin_lat, 0);

    // Set output enable HIGH to turn on the display
    gpio_put(framebuffer->config.pin_oe, 1);
}
