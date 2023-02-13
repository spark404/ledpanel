//
// Created by Hugo Trippaers on 13/02/2023.
//


#include <malloc.h>
#include <framebuffer.h>
#include <hardware/gpio.h>
#include <hardware/timer.h>
#include <string.h>

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
    gpio_set_pulls(framebuffer->config.pin_r1, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_g1, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_b1, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_clk, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_lat, 0, 1);
    gpio_set_pulls(framebuffer->config.pin_oe, 0, 1);

    size_t buffer_size = config.w * config.h * (config.bpp / 8);
    void *fb = malloc(buffer_size);
    if (fb == NULL) {
        return FRAMEBUFFER_ERROR;
    }
    bzero(fb, buffer_size);

    framebuffer->buffer = fb;
    framebuffer->config = config;
    framebuffer->pwm = 0;
    return FRAMEBUFFER_OK;
}

int framebuffer_sync(framebuffer_t *framebuffer) {
    uint8_t *ptr = framebuffer->buffer;

    for (int y = 0; y < framebuffer->config.h; y++) {
        if (y > 7) {
            // TODO Hack for the two lines
            continue;
        }

        int y_idx = y * framebuffer->config.w * 2;
        int y_b_idx = y_idx + (framebuffer->config.w * 2 * 8);
        for (int x = 0; x < framebuffer->config.w; x++) {
            int x_idx = x * 2;

            // Unpack a top_pixel
            uint16_t top_pixel = ptr[y_idx + x_idx] << 8 | ptr[y_idx + x_idx + 1];
            int t_r = ((top_pixel >> 11) & 0x1F) << 3;
            int t_g = ((top_pixel >> 5) & 0x3F) << 2;
            int t_b = (top_pixel & 0x1F) << 3;

            uint16_t bottom_pixel = ptr[y_b_idx + x_idx] << 8 | ptr[y_b_idx + x_idx + 1];
            int b_r = ((bottom_pixel >> 11) & 0x1F) << 3;
            int b_g = ((bottom_pixel >> 5) & 0x3F) << 2;
            int b_b = (bottom_pixel & 0x1F) << 3;

            if (framebuffer->pwm > (t_r / 4)) {
                t_r = 0;
            }
            if (framebuffer->pwm > (t_g / 4)) {
                t_g = 0;
            }
            if (framebuffer->pwm > (t_b / 4)) {
                t_b = 0;
            }
            if (framebuffer->pwm > (b_r / 4)) {
                b_r = 0;
            }
            if (framebuffer->pwm > (b_g / 4)) {
                b_g = 0;
            }
            if (framebuffer->pwm > (b_b / 4)) {
                b_b = 0;
            }

            // Set the y values for the panel top half
            gpio_put(framebuffer->config.pin_r0, t_r);
            gpio_put(framebuffer->config.pin_g0, t_g);
            gpio_put(framebuffer->config.pin_b0, t_b);

            // Set the y values for the panel bottom half
            gpio_put(framebuffer->config.pin_r1, b_r);
            gpio_put(framebuffer->config.pin_g1, b_g);
            gpio_put(framebuffer->config.pin_b1, b_b);

            // Shift the register into the shifter
            gpio_put(framebuffer->config.pin_clk, 1);
            // busy_wait_us(10); // TODO this could use some tweaking
            gpio_put(framebuffer->config.pin_clk, 0);
        }

        // Increase pwm cycle (16 steps)
        framebuffer->pwm++;
        if (framebuffer->pwm > 63) {
            framebuffer->pwm = 0;
        }

        // Trigger the latch
        latch(framebuffer, y);
    }

    return FRAMEBUFFER_OK;
}

int framebuffer_drawpixel(framebuffer_t *framebuffer, int x, int y, uint16_t rgb565_color) {
    if (x < 0 || x >= framebuffer->config.w) {
        return FRAMEBUFFER_ERROR;
    }

    if (y < 0 || y >= framebuffer->config.h) {
        return FRAMEBUFFER_ERROR;
    }

    uint8_t *ptr = framebuffer->buffer;
    int y_idx = y * framebuffer->config.w * 2;
    int x_idx = x * 2;

    ptr[y_idx + x_idx]  = rgb565_color >> 8;
    ptr[y_idx + x_idx + 1] = rgb565_color & 0xff;

    return FRAMEBUFFER_OK;
}

static void latch(framebuffer_t *framebuffer, int line) {
    // Set output enable LOW to turn off the display
    gpio_put(framebuffer->config.pin_oe, 0);

    // Select line to latch
    gpio_put(framebuffer->config.pin_a, line & 0x1);
    gpio_put(framebuffer->config.pin_b, (line & 0x2) >> 1);
    gpio_put(framebuffer->config.pin_c, (line & 0x4) >> 2);

    // Trigger the latch by pulsing LAT to HIGH
    gpio_put(framebuffer->config.pin_lat, 1);
    busy_wait_us(5);
    gpio_put(framebuffer->config.pin_lat, 0);

    // Set output enable HIGH to turn on the display
    gpio_put(framebuffer->config.pin_oe, 1);
}
