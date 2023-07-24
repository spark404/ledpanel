//
// Created by Hugo Trippaers on 23/07/2023.
//

#include <malloc.h>
#include "stdint.h"
#include "stdio.h"
#include "stdbool.h"
#include "gif_decoder.h"

#include "framebuffer.h"

extern uint8_t seasons_gif_start[] asm( "_binary_seasons_gif_start" );
extern uint8_t seasons_gif_end[]   asm( "_binary_seasons_gif_end" );
extern size_t seasons_gif_size  asm( "_binary_seasons_gif_size" );

static gif_t gif;

void gif_animation_init(framebuffer_t *framebuffer) {
    gif_decoder_init(seasons_gif_start, seasons_gif_end - seasons_gif_start, &gif);
}

void gif_animation_update(framebuffer_t *framebuffer) {
    frame_t frame;
    if (gif_decoder_read_image(&gif, &frame) != GIF_OK) {
        printf("Error in decoder");
        return;
    }

    printf("--- Frame %dx%d at %d,%d---\n", frame.width, frame.height, frame.offset_x, frame.offset_y);
    uint8_t *frame_ptr = frame.frame;
    for (int y=0; y<frame.height; y++) {
        for (int x=0; x<frame.width; x++) {
            uint32_t c = frame_ptr[0]<<16 | frame_ptr[1]<<8 | frame_ptr[2];
            framebuffer_drawpixel(framebuffer, x+frame.offset_x, y+frame.offset_y, c);
            //printf("Drawing %d,%d,%d at %d,%d", frame_ptr[0], frame_ptr[1], frame_ptr[2], x+frame.offset_x, y+frame.offset_y);
            frame_ptr+=3;
        }
    }
    free(frame.frame);
    frame.frame = NULL;
}