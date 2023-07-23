//
// Created by Hugo Trippaers on 23/07/2023.
//

#ifndef LEDPANEL_GIF_LZW_DECOMPRESS_H
#define LEDPANEL_GIF_LZW_DECOMPRESS_H

#define GIF_LZW_OK 0
#define GIF_LZW_ERROR 1

typedef unsigned char gif_lzw_error_t;

gif_lzw_error_t gif_decoder_read_image_data(uint8_t *data, uint8_t *buffer);
#endif //LEDPANEL_GIF_LZW_DECOMPRESS_H
