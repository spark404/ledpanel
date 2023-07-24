//
// Created by Hugo Trippaers on 23/07/2023.
//

#include <stdint.h>
#include <stdio.h>
#include "gif_lzw_decompress.h"

#define DEBUG 0

#if DEBUG
#define LOG_MSG printf
#else
#define LOG_MSG(...)
#endif

typedef struct {
    uint16_t length;
    uint16_t prefix;
    uint8_t  string_part;
} entry_t;

typedef struct {
    uint8_t code_size;
    uint8_t bits_remaining;
    uint16_t current;
    uint8_t *data_ptr;
    uint16_t table_size;
    entry_t table[4096];
    uint8_t *frame;
} reader_state_t;

static void init_table(reader_state_t *reader_state, uint16_t key_size);
static void add_table_entry(reader_state_t *reader_state, uint16_t length, uint16_t prefix, uint8_t suffix);
static uint16_t read_bits(reader_state_t *reader_state);


gif_lzw_error_t gif_decoder_read_image_data(uint8_t *data, uint8_t *buffer) {
    uint8_t *ptr = data;
    uint8_t root_size = *ptr;

    if (root_size < 2 || root_size > 8) {
        LOG_MSG("Invalid LZW minimum code size %d\n", root_size);
        return GIF_LZW_ERROR;
    }
    ptr++;

    // <CC> or the clear code, is (2**N),
    // <EOI>, or end-of-information, is (2**N + 1)
    uint8_t clear_code = 1 << root_size;
    uint8_t stop_code = clear_code + 1;

    uint8_t block_size = *ptr;

    LOG_MSG("LZW variable code root size %d\n", root_size);
    LOG_MSG("First block %d\n", block_size);

    // Read bit by bit, from right to left
    // the first code to read is root_size + 1
    // if the code read is the clear_code we clear the compression table
    // if the code read is the stop_code we are done

    reader_state_t reader_state = {
            .code_size = root_size + 1,
            .bits_remaining = 0,
            .data_ptr = ptr + 1
    };
    init_table(&reader_state, root_size);

    LOG_MSG("Setup bit_size %d, clear_code %02x, stop_code %02x\n", reader_state.code_size, clear_code, stop_code);
    LOG_MSG("Table initialized with %d entries, keysize increase at %d\n", reader_state.table_size, (1<<reader_state.code_size) -1);

    uint16_t pattern_length = 0;
    entry_t *code_table_entry;
    uint8_t *buffer_ptr = buffer;
    uint16_t code, old_code;
    uint8_t first_code = 1; // special marker that we are expecting the first code
    while (1) {
        code = read_bits(&reader_state);
        LOG_MSG("Code %04x (%d)\n", code, code);

        if (code == clear_code) {
            LOG_MSG("  Cleaning table and resetting key size after clear code\n");
            init_table(&reader_state, root_size);
            reader_state.code_size = root_size + 1;
            first_code = 1;
            continue;
        }

        if (code == stop_code) {
            LOG_MSG("  Stop code\n");
            break;
        }

        if (first_code) {
            LOG_MSG("  Output first code %02d\n", reader_state.table[code].string_part);
            *buffer_ptr++ = reader_state.table[code].string_part;
            old_code = code;
            first_code = 0;
            continue;
        }

        if (code < reader_state.table_size) {
            // Find the first pixel of the current sequence
            entry_t *tmp_ptr = &reader_state.table[code];
            while(1) {
                if (tmp_ptr->prefix == 0xFFF) {
                    break;
                }
                tmp_ptr = &reader_state.table[tmp_ptr->prefix];
            }

            // Entry is in the table, output the code
            uint8_t str_len = 0;
            code_table_entry = &reader_state.table[code];
            LOG_MSG("  Output existing sequence for key %04x (%d)\n", code, code);
            while(1) {
                LOG_MSG("    Output string %02d at %08d (prefix %04x)\n",
                        code_table_entry->string_part, (unsigned int)(buffer_ptr + code_table_entry->length - 1), code_table_entry->prefix);
                *(buffer_ptr + code_table_entry->length - 1) = code_table_entry->string_part;
                str_len++;

                if (code_table_entry->prefix == 0xFFF) {
                    break;
                }
                code_table_entry = &reader_state.table[code_table_entry->prefix];
            }
            buffer_ptr += str_len;

            // A new entry is added to the table consisting of a pointer to the last sequence,
            // followed by the first pixel in the current sequence.
            add_table_entry(&reader_state, reader_state.table[old_code].length + 1, old_code, tmp_ptr->string_part);

            old_code = code;
            if (reader_state.table_size == 1<<reader_state.code_size) {
                reader_state.code_size++;
            }
            continue;
        }

        // Code isn't in the table
        // A new entry is added to the table consisting of a pointer to the last sequence,
        // followed by the first pixel in the last sequence.

        // Find the first pixel of the previous sequence
        entry_t *tmp_ptr = &reader_state.table[old_code];
        while(1) {
            if (tmp_ptr->prefix == 0xFFF) {
                break;
            }
            tmp_ptr = &reader_state.table[tmp_ptr->prefix];
        }

        add_table_entry(&reader_state, reader_state.table[old_code].length + 1, old_code, tmp_ptr->string_part);
        uint8_t str_len = 0;
        code_table_entry = &reader_state.table[code];
        LOG_MSG("  Output new sequence for key %04x (%d)\n", code, code);
        while(1) {
            LOG_MSG("    Output string %02d at %08d (prefix %04x)\n",
                    code_table_entry->string_part, (unsigned int)(buffer_ptr + code_table_entry->length - 1), code_table_entry->prefix);
            *(buffer_ptr + code_table_entry->length - 1) = code_table_entry->string_part;
            str_len++;

            if (code_table_entry->prefix == 0xFFF) {
                break;
            }
            code_table_entry = &reader_state.table[code_table_entry->prefix];
        }
        buffer_ptr += str_len;
        old_code = code;
        if (reader_state.table_size == 1<<reader_state.code_size) {
            reader_state.code_size++;
        }
    }

    return GIF_LZW_OK;
}

static void init_table(reader_state_t *reader_state, uint16_t key_size) {
    // roots take up slots #0 through #(2**N-1), and the special codes are (2**N) and (2**N + 1)
    reader_state->table_size = (1 << key_size) + 2;
    for (int i=0; i < reader_state->table_size; i++) {
        reader_state->table[i].length = 1;
        reader_state->table[i].prefix = 0xFFF;
        reader_state->table[i].string_part = i;
    }
}

static void add_table_entry(reader_state_t *reader_state, uint16_t length, uint16_t prefix, uint8_t suffix) {
    LOG_MSG("      Adding entry %d, length=%d, prefix=%04x, string_part=%02x\n",
            reader_state->table_size, length, prefix, suffix);
    reader_state->table[reader_state->table_size].length = length;
    reader_state->table[reader_state->table_size].prefix = prefix;
    reader_state->table[reader_state->table_size].string_part = suffix;
    reader_state->table_size++;
}

static uint16_t read_bits(reader_state_t *reader_state) {
    uint8_t *ptr = reader_state->data_ptr;
    // TODO check for block end

    if (reader_state->bits_remaining == 0) {
        reader_state->current = *ptr | *(ptr + 1) << 8;
        reader_state->bits_remaining += 16;
        reader_state->data_ptr = ptr+2;
    }

    if (reader_state->bits_remaining <= 8) {
        // Add 8 new bits of data to the reader
        // by moving the data pointer forward by one position
        // and filling up with a new byte
        reader_state->current >>= 8;
        reader_state->current |= *ptr << 8;
        reader_state->bits_remaining += 8;
        reader_state->data_ptr = ptr+1;
    }

    if (reader_state->bits_remaining < reader_state->code_size) {
        LOG_MSG("Not enough data remaining\n");
        return 0;
    }

    LOG_MSG("Current 0x%04x, keysize %d, bitremaining %d\n", reader_state->current, reader_state->code_size, reader_state->bits_remaining);
    uint16_t value = reader_state->current;
    value >>= (16 - reader_state->bits_remaining);
    value &= (1 << reader_state->code_size) - 1;

    reader_state->bits_remaining -= reader_state->code_size;

    return value;
}
