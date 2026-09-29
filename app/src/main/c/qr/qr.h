#ifndef TINYQR_QR_H
#define TINYQR_QR_H
#include <stdbool.h>
#include <stdint.h>
typedef struct { const uint8_t *data; int width; int height; int row_stride; } TinyQrFrame;
typedef struct TinyQrDecoder TinyQrDecoder;
typedef struct {
    /* Decoder-owned. Valid until the next decode on this decoder or destruction. */
    const uint8_t *data;
    int length;
} TinyQrResult;
TinyQrDecoder *tinyqr_decoder_create(void);
void tinyqr_decoder_destroy(TinyQrDecoder *decoder);
bool tinyqr_decode(TinyQrDecoder *decoder, const TinyQrFrame *frame, TinyQrResult *result);
#endif
