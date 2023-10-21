#include <pico/stdio.h>
#include <pico/printf.h>
#include <hardware/gpio.h>
#include <pico/time.h>
#include <math.h>
#include <hardware/i2c.h>
#include <pico/stdio_uart.h>
#include "framebuffer.h"
#include "animations/animations.h"
#include "i2c_slave.h"

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

#define I2C_BAUDRATE 100000
#define I2C_ADDRESS 0x50
#define I2C_1_SCL 15
#define I2C_1_SDA 14

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
static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event);

static uint64_t last_i2c_transmission;
static uint8_t i2c_timeout;

bool timer_callback(repeating_timer_t *user_data) {
    gif_animation_update(&fb);
    return true;
}

int main(void) {
    stdio_uart_init();
    printf("PicoPlayer Starting\n");

    // Enable led on boot
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);


    // Start all the framebuffer work on the second core
    //multicore_launch_core1(core1_entry);

    gif_animation_init(&fb);

    repeating_timer_t timer;
    alarm_pool_t *alarm_pool = alarm_pool_create_with_unused_hardware_alarm(1);
    alarm_pool_add_repeating_timer_us(alarm_pool, 60 * (int64_t)1000, timer_callback, NULL, &timer);

    i2c_init(i2c1, I2C_BAUDRATE);
    gpio_init(I2C_1_SCL);
    gpio_set_function(I2C_1_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_1_SCL);
    gpio_init(I2C_1_SDA);
    gpio_set_function(I2C_1_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_1_SDA);
    i2c_slave_init(i2c1, I2C_ADDRESS, &i2c_slave_handler);
    last_i2c_transmission = time_us_64(); // Initialize the timeout measurement

    // Core issues with timers, run on main core for now
    if (framebuffer_init(framebuffer_config, &fb) != FRAMEBUFFER_OK) {
        panic("Framebuffer issue");
    }

    while (1) {
        if (time_us_64() - last_i2c_transmission > 10 * 1000 * 1000) {
            if (!i2c_timeout) {
                i2c_timeout = 1;
                gif_animation_play(0, 3);
            }
        }
        else {
            i2c_timeout = 0;
        }
        framebuffer_sync(&fb);
    }
}

static void core1_entry() {
    if (framebuffer_init(framebuffer_config, &fb) != FRAMEBUFFER_OK) {
        return;
    }

    while (1) {
        framebuffer_sync(&fb);
    }
}

static uint8_t i2c_bytes_received = 0;
static uint8_t i2c_bytes_sent = 0;
static uint8_t i2c_register;
static uint8_t buffer[64];
static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    uint8_t byte;
    last_i2c_transmission = time_us_64();

    switch(event) {
    case I2C_SLAVE_RECEIVE:
        buffer[i2c_bytes_received] = i2c_read_byte_raw(i2c);
        if (i2c_bytes_received == 0) {
            // First byte after START or RESTART is the register id
            i2c_register = buffer[0];
        }

        if (i2c_bytes_received == 2) {
            if (buffer[1] > 7 || buffer[2] > 3) {
                // Safety
                gif_animation_play(1, 3);
                return;
            }

            if (buffer[1] != gif_animation_get_sequence()) {
                gif_animation_play(buffer[1], buffer[2]);
            }
            switch (buffer[2]) {
                case 0:
                    gif_animation_stop();
                    break;
                case 1:
                    gif_animation_pause();
                    break;
                case 2:
                    gif_animation_resume();
            }
        }

        i2c_bytes_received++;

        break;
    case I2C_SLAVE_REQUEST:
        if (i2c_register != 0x42) {
            i2c_write_byte_raw(i2c, 0x0);
            return;
        }

        if (i2c_bytes_sent == 0) { // Sequence is the first byte
            i2c_write_byte_raw(i2c, gif_animation_get_sequence());
            i2c_bytes_sent++;
        }
        else if (i2c_bytes_sent == 1) { // State is the second byte
            i2c_write_byte_raw(i2c, gif_animation_get_state());
            i2c_bytes_sent++;
        } else {
            i2c_write_byte_raw(i2c, 0x0);
        }
        break;
    case I2C_SLAVE_FINISH:
        i2c_bytes_sent = 0;
        i2c_bytes_received = 0;
        break;
    default:
        break;
    }
}

