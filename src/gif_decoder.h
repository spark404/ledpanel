//
// Created by Hugo Trippaers on 22/07/2023.
//

#ifndef LEDPANEL_GIF_DECODER_H
#define LEDPANEL_GIF_DECODER_H

#define GIF_OK    0
#define GIF_ERROR 1

#define BLOCK_EXTENSION_INTRODUCER 0x21
#define BLOCK_IMAGE_DESCRIPTOR 0x2C

#define BLOCK_TRAILER 0x3B

typedef unsigned char gif_error_t;

typedef struct {
    uint8_t *image_start;
    size_t image_size;
    uint8_t ct_size;
    uint8_t *global_ct;
    uint8_t *first_frame;
    uint8_t *frame_ptr;
} gif_t;

typedef struct {
    uint16_t offset_x, offset_y;
    uint16_t width, height;
    uint8_t *frame;
} frame_t;

gif_error_t gif_decoder_init(uint8_t *source, size_t size, gif_t *gif);
gif_error_t gif_decoder_read_image(gif_t *gif, frame_t *frame);

#endif //LEDPANEL_GIF_DECODER_H