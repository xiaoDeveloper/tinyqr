#include "result_ui.h"

TinyQrResultLayout tinyqr_result_layout(int width, int height) {
    TinyQrResultLayout layout = {0};
    if (width <= 0 || height <= 0) return layout;
    layout.card_left = width / 10;
    layout.card_right = width - layout.card_left;
    layout.card_top = height * 15 / 100;
    layout.card_bottom = height * 86 / 100;
    layout.copy_left = layout.card_left;
    layout.copy_right = width / 2 - width / 4 / 10;
    layout.rescan_left = width / 2 + width / 4 / 10;
    layout.rescan_right = layout.card_right;
    layout.actions_top = height * 70 / 100;
    layout.actions_bottom = height * 84 / 100;
    return layout;
}

TinyQrResultAction tinyqr_result_hit_test(int width, int height, float x, float y) {
    TinyQrResultLayout layout = tinyqr_result_layout(width, height);
    if (y < layout.actions_top || y > layout.actions_bottom) return TINYQR_RESULT_ACTION_NONE;
    if (x >= layout.copy_left && x <= layout.copy_right) return TINYQR_RESULT_ACTION_COPY;
    if (x >= layout.rescan_left && x <= layout.rescan_right) return TINYQR_RESULT_ACTION_RESCAN;
    return TINYQR_RESULT_ACTION_NONE;
}
