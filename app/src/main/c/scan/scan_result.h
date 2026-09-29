#ifndef TINYQR_SCAN_RESULT_H
#define TINYQR_SCAN_RESULT_H

#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>

/* Matches quirc's maximum decoded payload without exposing quirc outside qr/. */
#define TINYQR_MAX_RESULT_BYTES 8896

typedef struct {
    uint8_t data[TINYQR_MAX_RESULT_BYTES];
    int length;
    bool ready;
} TinyQrStoredResult;

typedef struct {
    atomic_flag lock;
    bool accepting;
    TinyQrStoredResult value;
} TinyQrPendingResult;

#define TINYQR_PENDING_RESULT_INIT { ATOMIC_FLAG_INIT, true, { {0}, 0, false } }

bool tinyqr_pending_result_publish(TinyQrPendingResult *pending, const uint8_t *data, int length);
bool tinyqr_pending_result_consume(TinyQrPendingResult *pending, TinyQrStoredResult *destination);
void tinyqr_pending_result_reset(TinyQrPendingResult *pending);
void tinyqr_stored_result_clear(TinyQrStoredResult *result);
bool tinyqr_result_is_printable_ascii(const uint8_t *data, int length);
bool tinyqr_result_is_utf8_text(const uint8_t *data, int length);

#endif
