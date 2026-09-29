#ifndef TINYQR_RESULT_UI_H
#define TINYQR_RESULT_UI_H

typedef enum {
    TINYQR_RESULT_ACTION_NONE,
    TINYQR_RESULT_ACTION_COPY,
    TINYQR_RESULT_ACTION_RESCAN
} TinyQrResultAction;

typedef struct {
    int card_left, card_right, card_top, card_bottom;
    int copy_left, copy_right, rescan_left, rescan_right;
    int actions_top, actions_bottom;
} TinyQrResultLayout;

TinyQrResultLayout tinyqr_result_layout(int width, int height);
TinyQrResultAction tinyqr_result_hit_test(int width, int height, float x, float y);

#endif
