#include "../app/src/main/c/scan/scan_result.h"
#include "../app/src/main/c/render/result_ui.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void copied_payload_survives_source_mutation(void) {
    TinyQrPendingResult pending = TINYQR_PENDING_RESULT_INIT;
    TinyQrStoredResult displayed = {0};
    uint8_t source[] = "hello";

    assert(tinyqr_pending_result_publish(&pending, source, 5));
    memset(source, 'x', 5);
    assert(tinyqr_pending_result_consume(&pending, &displayed));
    assert(displayed.ready && displayed.length == 5);
    assert(memcmp(displayed.data, "hello", 5) == 0);
}

static void exact_length_and_duplicate_are_preserved(void) {
    TinyQrPendingResult pending = TINYQR_PENDING_RESULT_INIT;
    TinyQrStoredResult displayed = {0};
    const uint8_t first[] = {'a', 0, 'b'};
    const uint8_t second[] = "second";

    assert(tinyqr_pending_result_publish(&pending, first, 3));
    assert(!tinyqr_pending_result_publish(&pending, second, 6));
    assert(tinyqr_pending_result_consume(&pending, &displayed));
    assert(displayed.length == 3);
    assert(memcmp(displayed.data, first, 3) == 0);
}

static void explicit_reset_allows_a_new_result(void) {
    TinyQrPendingResult pending = TINYQR_PENDING_RESULT_INIT;
    TinyQrStoredResult displayed = {0};
    const uint8_t first[] = "first";
    const uint8_t second[] = "second";

    assert(tinyqr_pending_result_publish(&pending, first, 5));
    assert(tinyqr_pending_result_consume(&pending, &displayed));
    assert(!tinyqr_pending_result_publish(&pending, second, 6));
    tinyqr_stored_result_clear(&displayed);
    assert(!displayed.ready && displayed.length == 0);
    tinyqr_pending_result_reset(&pending);
    assert(tinyqr_pending_result_publish(&pending, second, 6));
    assert(tinyqr_pending_result_consume(&pending, &displayed));
    assert(displayed.length == 6 && memcmp(displayed.data, second, 6) == 0);
}

static void bounds_are_explicit(void) {
    TinyQrPendingResult pending = TINYQR_PENDING_RESULT_INIT;
    TinyQrPendingResult maximum_pending = TINYQR_PENDING_RESULT_INIT;
    uint8_t byte = 1;
    uint8_t maximum[TINYQR_MAX_RESULT_BYTES] = {0};

    assert(!tinyqr_pending_result_publish(&pending, NULL, 1));
    assert(!tinyqr_pending_result_publish(&pending, &byte, -1));
    assert(!tinyqr_pending_result_publish(&pending, &byte, TINYQR_MAX_RESULT_BYTES + 1));
    assert(tinyqr_pending_result_publish(&pending, &byte, 0));
    assert(tinyqr_pending_result_publish(&maximum_pending, maximum, TINYQR_MAX_RESULT_BYTES));
}

static void printable_classification_is_binary_safe(void) {
    const uint8_t text[] = "https://example.com";
    const uint8_t binary[] = {0, 1, 0xff};
    assert(tinyqr_result_is_printable_ascii((const uint8_t *)"hello", 5));
    assert(tinyqr_result_is_printable_ascii(text, (int)sizeof(text) - 1));
    assert(!tinyqr_result_is_printable_ascii(binary, 3));
}

static void result_actions_have_stable_hit_regions(void) {
    assert(tinyqr_result_hit_test(1080, 2400, 300.0f, 1860.0f) == TINYQR_RESULT_ACTION_COPY);
    assert(tinyqr_result_hit_test(1080, 2400, 760.0f, 1860.0f) == TINYQR_RESULT_ACTION_RESCAN);
    assert(tinyqr_result_hit_test(1080, 2400, 540.0f, 1000.0f) == TINYQR_RESULT_ACTION_NONE);
    assert(tinyqr_result_hit_test(1080, 2400, 0.0f, 0.0f) == TINYQR_RESULT_ACTION_NONE);
    assert(tinyqr_result_hit_test(720, 1280, 205.0f, 990.0f) == TINYQR_RESULT_ACTION_COPY);
    assert(tinyqr_result_hit_test(720, 1280, 520.0f, 990.0f) == TINYQR_RESULT_ACTION_RESCAN);
}

static void action_labels_fit_their_regions(int width, int height) {
    const int scale = 3;
    const int glyph_advance = 6 * scale;
    const int copy_width = 4 * glyph_advance;
    const int rescan_width = 6 * glyph_advance;
    TinyQrResultLayout layout = tinyqr_result_layout(width, height);
    int copy_x = (layout.copy_left + layout.copy_right - copy_width) / 2;
    int rescan_x = (layout.rescan_left + layout.rescan_right - rescan_width) / 2;

    assert(copy_x >= layout.copy_left);
    assert(copy_x + copy_width <= layout.copy_right);
    assert(rescan_x >= layout.rescan_left);
    assert(rescan_x + rescan_width <= layout.rescan_right);
}

static void action_label_geometry_is_valid_on_portrait_screens(void) {
    action_labels_fit_their_regions(415, 922);
    action_labels_fit_their_regions(360, 800);
    action_labels_fit_their_regions(1080, 2400);
}

int main(void) {
    copied_payload_survives_source_mutation();
    exact_length_and_duplicate_are_preserved();
    explicit_reset_allows_a_new_result();
    bounds_are_explicit();
    printable_classification_is_binary_safe();
    result_actions_have_stable_hit_regions();
    action_label_geometry_is_valid_on_portrait_screens();
    puts("scan result tests passed");
    return 0;
}
