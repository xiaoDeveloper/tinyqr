#include "scan_result.h"

#include <string.h>

static void lock(TinyQrPendingResult *pending) {
    while (atomic_flag_test_and_set_explicit(&pending->lock, memory_order_acquire)) { }
}

static void unlock(TinyQrPendingResult *pending) {
    atomic_flag_clear_explicit(&pending->lock, memory_order_release);
}

bool tinyqr_pending_result_publish(TinyQrPendingResult *pending, const uint8_t *data, int length) {
    if (!pending || length < 0 || length > TINYQR_MAX_RESULT_BYTES || (length > 0 && !data)) return false;
    lock(pending);
    if (!pending->accepting || pending->value.ready) { unlock(pending); return false; }
    if (length > 0) memcpy(pending->value.data, data, (size_t)length);
    pending->value.length = length;
    pending->value.ready = true;
    unlock(pending);
    return true;
}

bool tinyqr_pending_result_consume(TinyQrPendingResult *pending, TinyQrStoredResult *destination) {
    if (!pending || !destination) return false;
    lock(pending);
    if (!pending->value.ready) { unlock(pending); return false; }
    *destination = pending->value;
    pending->value.length = 0;
    pending->value.ready = false;
    pending->accepting = false;
    unlock(pending);
    return true;
}

void tinyqr_pending_result_reset(TinyQrPendingResult *pending) {
    if (!pending) return;
    lock(pending);
    pending->value.length = 0;
    pending->value.ready = false;
    pending->accepting = true;
    unlock(pending);
}

void tinyqr_stored_result_clear(TinyQrStoredResult *result) {
    if (!result) return;
    result->length = 0;
    result->ready = false;
}

bool tinyqr_result_is_printable_ascii(const uint8_t *data, int length) {
    if (length < 0 || (length > 0 && !data)) return false;
    for (int i = 0; i < length; ++i)
        if (data[i] < 0x20 || data[i] > 0x7e) return false;
    return true;
}

bool tinyqr_result_is_utf8_text(const uint8_t *data, int length) {
    if (length < 0 || (length > 0 && !data)) return false;
    for (int i = 0; i < length;) {
        uint8_t first = data[i++];
        if (first < 0x20 || first == 0x7f) return false;
        if (first < 0x80) continue;
        int extra = first >= 0xf0 && first <= 0xf4 ? 3 : first >= 0xe0 && first <= 0xef ? 2 : first >= 0xc2 && first <= 0xdf ? 1 : -1;
        if (extra < 0 || i + extra > length) return false;
        uint32_t codepoint = first & ((1u << (7 - extra)) - 1u);
        for (int j = 0; j < extra; ++j) {
            uint8_t next = data[i++];
            if ((next & 0xc0) != 0x80) return false;
            codepoint = (codepoint << 6) | (next & 0x3f);
        }
        if (codepoint < (extra == 1 ? 0x80 : extra == 2 ? 0x800 : 0x10000) ||
            (codepoint >= 0xd800 && codepoint <= 0xdfff) || codepoint > 0x10ffff) return false;
    }
    return true;
}
