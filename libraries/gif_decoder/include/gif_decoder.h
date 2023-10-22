//
// Created by Hugo Trippaers on 22/07/2023.
//

#ifndef _GIF_DECODER_H
#define _GIF_DECODER_H

#define GIF_OK    0
#define GIF_ERROR 1
#define GIF_EOF 2

#define BLOCK_EXTENSION_INTRODUCER 0x21
#define BLOCK_IMAGE_DESCRIPTOR 0x2C
#define BLOCK_TRAILER 0x3B

#define EXTBLOCK_GCE 0xF9

typedef unsigned char gif_error_t;

typedef struct {
    uint8_t *image_start;
    size_t image_size;
    uint16_t ct_size;
    uint8_t *global_ct;
    uint8_t *first_frame;
    uint8_t *frame_ptr;
    uint8_t transparancy_enabled;
    uint8_t transparancy_index;
} gif_t;

typedef struct {
    uint16_t offset_x, offset_y;
    uint16_t width, height;
    uint8_t *frame;
    uint8_t *color_table;
    uint8_t transparancy_enabled;
    uint8_t transparancy_index;
} frame_t;

gif_error_t gif_decoder_init(uint8_t *source, size_t size, gif_t *gif);
gif_error_t gif_decoder_read_next_frame(gif_t *gif, frame_t *frame);

#endif //_GIF_DECODER_H
