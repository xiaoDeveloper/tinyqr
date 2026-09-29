#ifndef TINYQR_SCAN_PREVIEW_MATH_H
#define TINYQR_SCAN_PREVIEW_MATH_H

#include <stdint.h>
typedef struct { int left; int top; int right; int bottom; } TinyQrScanArea;
typedef struct { int left; int top; int right; int bottom; } TinyQrPreviewRect;
TinyQrScanArea tinyqr_scan_area(int width, int height);
int tinyqr_scan_line_y(const TinyQrScanArea *area, uint32_t frame_index);
int tinyqr_center_crop_source_x(int x, int destination_width, int destination_height, int source_width, int source_height);
int tinyqr_center_crop_source_y(int y, int destination_width, int destination_height, int source_width, int source_height);
int tinyqr_preview_source_x(int x, int y, int destination_width, int destination_height, int source_width, int source_height, int sensor_orientation);
int tinyqr_preview_source_y(int x, int y, int destination_width, int destination_height, int source_width, int source_height, int sensor_orientation);
uint16_t tinyqr_rgb565_luma(uint8_t luma);
TinyQrPreviewRect tinyqr_scan_overlay_rect(const TinyQrScanArea *area, uint32_t frame_index, int index);
#endif
