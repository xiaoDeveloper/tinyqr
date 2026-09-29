#include "qr.h"
#include "../third_party/quirc/quirc.h"
#include "../tinyqr.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct TinyQrDecoder {
    struct quirc *quirc;
    struct quirc_data data;
    int width;
    int height;
};

static bool decode_candidates(TinyQrDecoder *decoder, const TinyQrFrame *frame,
                              TinyQrResult *result, bool stylized_fallback) {
    int count = quirc_count(decoder->quirc);
    const char *stage = stylized_fallback ? "stylized fallback " : "";
    TQ_LOG("%sframe=%dx%d candidates=%d", stage, frame->width, frame->height, count);
    for (int index = 0; index < count; ++index) {
        struct quirc_code code;
        quirc_extract(decoder->quirc, index, &code);
        quirc_decode_error_t error = quirc_decode(&code, &decoder->data);
        if (error != QUIRC_SUCCESS) {
            TQ_LOG("%scandidate=%d/%d frame=%dx%d decode failed: %s (grid=%d)",
                   stage, index, count, frame->width, frame->height, quirc_strerror(error), code.size);
            quirc_flip(&code);
            error = quirc_decode(&code, &decoder->data);
            if (error != QUIRC_SUCCESS) {
                TQ_LOG("%scandidate=%d/%d frame=%dx%d flipped decode failed: %s (grid=%d)",
                       stage, index, count, frame->width, frame->height,
                       quirc_strerror(error), code.size);
                continue;
            }
        }
        result->data = decoder->data.payload;
        result->length = decoder->data.payload_len;
        return true;
    }
    return false;
}

TinyQrDecoder *tinyqr_decoder_create(void) {
    TinyQrDecoder *decoder = calloc(1, sizeof(*decoder));
    if (!decoder) return NULL;
    decoder->quirc = quirc_new();
    if (!decoder->quirc) { free(decoder); return NULL; }
    return decoder;
}

void tinyqr_decoder_destroy(TinyQrDecoder *decoder) {
    if (!decoder) return;
    quirc_destroy(decoder->quirc);
    free(decoder);
}

bool tinyqr_decode(TinyQrDecoder *decoder, const TinyQrFrame *frame, TinyQrResult *result) {
    if (result) { result->data = NULL; result->length = 0; }
    if (!decoder || !decoder->quirc || !frame || !frame->data || !result ||
        frame->width <= 0 || frame->height <= 0 || frame->row_stride < frame->width ||
        frame->width > INT_MAX / frame->height) return false;
    if (decoder->width != frame->width || decoder->height != frame->height) {
        if (quirc_resize(decoder->quirc, frame->width, frame->height) < 0) return false;
        decoder->width = frame->width;
        decoder->height = frame->height;
    }
    uint8_t *image = quirc_begin(decoder->quirc, NULL, NULL);
    if (!image) return false;
    for (int y = 0; y < frame->height; ++y) {
        memcpy(image + (size_t)y * (size_t)frame->width,
               frame->data + (size_t)y * (size_t)frame->row_stride,
               (size_t)frame->width);
    }
    quirc_end(decoder->quirc);
    /* Zero grids can still contain capstones with rounded finder geometry.
       Rebuild only after every normal candidate has failed. */
    if (decode_candidates(decoder, frame, result, false)) return true;
    int stylized_count = quirc_rebuild_stylized(decoder->quirc);
    if (!stylized_count) return false;
    return decode_candidates(decoder, frame, result, true);
}
