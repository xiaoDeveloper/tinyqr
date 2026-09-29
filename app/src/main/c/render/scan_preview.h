#ifndef TINYQR_SCAN_PREVIEW_H
#define TINYQR_SCAN_PREVIEW_H
#include <android/native_window.h>
#include <stdint.h>
void tinyqr_render_boot_frame(ANativeWindow *window);
void tinyqr_render_scan_frame(ANativeWindow *window, const uint8_t *y_plane, int source_width, int source_height,
                              int row_stride, uint32_t frame_index, int sensor_orientation);
#endif
