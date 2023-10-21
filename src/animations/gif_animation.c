//
// Created by Hugo Trippaers on 23/07/2023.
//

#include <malloc.h>
#include "stdio.h"
#include "gif_decoder.h"

#include "framebuffer.h"

typedef struct {
    uint8_t *start;
    uint8_t *end;
} gif_image_t;

extern uint8_t baloons_gif_start[] asm( "images_baloons_gif_start" );
extern uint8_t baloons_gif_end[]   asm( "images_baloons_gif_end" );
extern uint8_t chevrons_gif_start[] asm( "images_chevrons_gif_start" );
extern uint8_t chevrons_gif_end[]   asm( "images_chevrons_gif_end" );
extern uint8_t fishandcat_gif_start[] asm( "images_fishandcat_gif_start" );
extern uint8_t fishandcat_gif_end[]   asm( "images_fishandcat_gif_end" );
extern uint8_t pattern_gif_start[] asm( "images_pattern_gif_start" );
extern uint8_t pattern_gif_end[]   asm( "images_pattern_gif_end" );
extern uint8_t seasons_gif_start[] asm( "images_seasons_gif_start" );
extern uint8_t seasons_gif_end[]   asm( "images_seasons_gif_end" );
extern uint8_t skull_gif_start[] asm( "images_skull_gif_start" );
extern uint8_t skull_gif_end[]   asm( "images_skull_gif_end" );
extern uint8_t snafu_gif_start[] asm( "images_snafu_gif_start" );
extern uint8_t snafu_gif_end[]   asm( "images_snafu_gif_end" );
extern uint8_t pnp2000_gif_start[] asm( "images_pnp2000_gif_start" );
extern uint8_t pnp2000_gif_end[] asm( "images_pnp2000_gif_end" );

// Stuck?
extern uint8_t lattice_gif_start[] asm("images_lattice_gif_start");
extern uint8_t lattice_gif_end[] asm("images_lattice_gif_end");

// Interlaces images
extern uint8_t numbers_gif_start[] asm( "images_numbers_gif_start" );
extern uint8_t numbers_gif_end[]   asm( "images_numbers_gif_end" );
extern uint8_t snowing_gif_start[] asm( "images_snowing_gif_start" );
extern uint8_t snowing_gif_end[]   asm( "images_snowing_gif_end" );

static gif_image_t sequences[] = {
        { baloons_gif_start, baloons_gif_end },
        { chevrons_gif_start, chevrons_gif_end },
        { fishandcat_gif_start, fishandcat_gif_end },
        { pattern_gif_start, pattern_gif_end },
        { seasons_gif_start, seasons_gif_end },
        { skull_gif_start, skull_gif_end },
        { snafu_gif_start, snafu_gif_end },
        { pnp2000_gif_start, pnp2000_gif_end }
};

#define DEFAULT_SEQUENCE 0

typedef enum {
    STOPPED,
    PAUSED,
    PLAYING,
    PLAYING_LOOP
} git_animation_state_t;

static gif_t gif;
static mutex_t gif_mutex;
static git_animation_state_t state = STOPPED;
static git_animation_state_t pause_state;
static uint8_t current_sequence = DEFAULT_SEQUENCE;

void gif_animation_init(framebuffer_t *framebuffer) {
    mutex_init(&gif_mutex);
    gif_decoder_init(sequences[DEFAULT_SEQUENCE].start, sequences[DEFAULT_SEQUENCE].end - sequences[DEFAULT_SEQUENCE].start, &gif);
    state = PLAYING_LOOP;
}

void gif_animation_play(int sequence_id, int new_state) {
    printf("Animation Play Sequence %d, State %d\n", sequence_id, new_state);
    mutex_enter_blocking(&gif_mutex);
    current_sequence = sequence_id;
    gif_decoder_init(sequences[current_sequence].start, sequences[current_sequence].end - sequences[current_sequence].start, &gif);
    state = new_state;
    mutex_exit(&gif_mutex);
}

void gif_animation_pause() {
    printf("Animation Pause, was %d\n", state);
    mutex_enter_blocking(&gif_mutex);
    pause_state = state;
    state = PAUSED;
    mutex_exit(&gif_mutex);
}

void gif_animation_resume() {
    if (state != PAUSED)
        return;
    printf("Animation Resume, to %d\n", pause_state);
    mutex_enter_blocking(&gif_mutex);
    state = pause_state;
    mutex_exit(&gif_mutex);
}

void gif_animation_stop() {
    printf("Animation Stop\n");
    mutex_enter_blocking(&gif_mutex);
    state = STOPPED;
    mutex_exit(&gif_mutex);
}

uint8_t gif_animation_get_sequence() {
    return current_sequence;
}

uint8_t gif_animation_get_state() {
    return state;
}

void gif_animation_update(framebuffer_t *framebuffer) {
    mutex_enter_blocking(&gif_mutex);
    if (state == PAUSED) {
        mutex_exit(&gif_mutex);
        return;
    }

    if (state == STOPPED) {
        framebuffer_clear(framebuffer);
        mutex_exit(&gif_mutex);
        return;
    }

    frame_t frame;
    gif_error_t res = gif_decoder_read_next_frame(&gif, &frame);
    if (res == GIF_EOF) {
        if (state == PLAYING_LOOP) {
            gif.frame_ptr = gif.first_frame;
            res = gif_decoder_read_next_frame(&gif, &frame);
        }
        else {
            state = STOPPED;
            mutex_exit(&gif_mutex);
            return;
        }
    }

    if (res != GIF_OK) {
        printf("Error in decoder %d\n", res);
        mutex_exit(&gif_mutex);
        return;
    }

    uint8_t *frame_ptr = frame.frame;
    for (int y=0; y<frame.height; y++) {
        for (int x=0; x<frame.width; x++) {
            uint8_t pixel = *frame_ptr++;
            if (frame.transparancy_enabled && pixel == frame.transparancy_index) {
                    continue;
            }

            uint8_t *color_idx = frame.color_table + pixel * 3;
            uint32_t color = *color_idx << 16 | *(color_idx+1) << 8 | *(color_idx+2);

            framebuffer_drawpixel(framebuffer, x+frame.offset_x, y+frame.offset_y, color);
        }
    }
    free(frame.frame);
    frame.frame = NULL;
    free(frame.color_table);
    frame.color_table = NULL;
    mutex_exit(&gif_mutex);
}