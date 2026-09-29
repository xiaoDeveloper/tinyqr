#include "scan_preview.h"
#include "scan_preview_math.h"

#define TINYQR_PREVIEW_FP_SHIFT 32

static void preview_crop(int destination_width, int destination_height, int source_width, int source_height,
                         int *crop_x, int *crop_y, int *visible_width, int *visible_height) {
    *crop_x = 0; *crop_y = 0;
    *visible_width = source_width; *visible_height = source_height;
    if ((int64_t)source_width * destination_height > (int64_t)source_height * destination_width) {
        *visible_width = (int)((int64_t)source_height * destination_width / destination_height);
        *crop_x = (source_width - *visible_width) / 2;
    } else if ((int64_t)source_width * destination_height < (int64_t)source_height * destination_width) {
        *visible_height = (int)((int64_t)source_width * destination_height / destination_width);
        *crop_y = (source_height - *visible_height) / 2;
    }
}

static void fill_rect(uint16_t *pixels, int width, int height, int stride, int left, int top, int right, int bottom, uint16_t color) {
    if (left < 0) left = 0; if (top < 0) top = 0; if (right > width) right = width; if (bottom > height) bottom = height;
    for (int y = top; y < bottom; ++y) for (int x = left; x < right; ++x) pixels[y * stride + x] = color;
}
static void draw_overlay(uint16_t *pixels, int width, int height, int stride, uint32_t frame_index) {
    TinyQrScanArea area = tinyqr_scan_area(width, height);
    const uint16_t color = 0xffff;
    for (int index = 0; index < 9; ++index) {
        TinyQrPreviewRect rect = tinyqr_scan_overlay_rect(&area, frame_index, index);
        fill_rect(pixels, width, height, stride, rect.left, rect.top, rect.right, rect.bottom, color);
    }
}
void tinyqr_render_boot_frame(ANativeWindow *window) {
    ANativeWindow_Buffer buffer;
    if (!window || ANativeWindow_lock(window, &buffer, 0) != 0) return;
    if (buffer.bits && buffer.width > 0 && buffer.height > 0) {
        uint16_t *pixels = buffer.bits;
        for (int y = 0; y < buffer.height; ++y)
            for (int x = 0; x < buffer.width; ++x) pixels[y * buffer.stride + x] = 0;
    }
    ANativeWindow_unlockAndPost(window);
}
void tinyqr_render_scan_frame(ANativeWindow *window, const uint8_t *y_plane, int source_width, int source_height, int row_stride, uint32_t frame_index, int sensor_orientation) {
    ANativeWindow_Buffer buffer;
    if (!window || !y_plane || source_width <= 0 || source_height <= 0 || row_stride < source_width || ANativeWindow_lock(window, &buffer, 0) != 0) return;
    if (!buffer.bits || buffer.width <= 0 || buffer.height <= 0) { ANativeWindow_unlockAndPost(window); return; }
    uint16_t *pixels = buffer.bits;
    const int rotated = sensor_orientation == 90 || sensor_orientation == 270;
    const int reversed = sensor_orientation == 180 || sensor_orientation == 270;
    const int oriented_width = rotated ? source_height : source_width;
    const int oriented_height = rotated ? source_width : source_height;
    int crop_x, crop_y, visible_width, visible_height;
    preview_crop(buffer.width, buffer.height, oriented_width, oriented_height,
                 &crop_x, &crop_y, &visible_width, &visible_height);
    const int64_t x_step = ((int64_t)visible_width << TINYQR_PREVIEW_FP_SHIFT) / buffer.width;
    const int64_t y_step = ((int64_t)visible_height << TINYQR_PREVIEW_FP_SHIFT) / buffer.height;
    int64_t v_fp = (int64_t)crop_y << TINYQR_PREVIEW_FP_SHIFT;

    if (!rotated) {
        const int source_y_base = reversed ? source_height - 1 : 0;
        const int source_y_direction = reversed ? -1 : 1;
        const int source_x_base = reversed ? source_width - 1 : 0;
        const int source_x_direction = reversed ? -1 : 1;
        for (int y = 0; y < buffer.height; ++y, v_fp += y_step) {
            uint16_t *destination = pixels + y * buffer.stride;
            const int source_y = source_y_base + source_y_direction * (int)(v_fp >> TINYQR_PREVIEW_FP_SHIFT);
            const uint8_t *source_row = y_plane + source_y * row_stride;
            int64_t u_fp = (int64_t)crop_x << TINYQR_PREVIEW_FP_SHIFT;
            for (int x = 0; x < buffer.width; ++x, u_fp += x_step) {
                const uint8_t luma = source_row[source_x_base + source_x_direction * (int)(u_fp >> TINYQR_PREVIEW_FP_SHIFT)];
                destination[x] = (uint16_t)(((luma >> 3) << 11) | ((luma >> 2) << 5) | (luma >> 3));
            }
        }
    } else {
        const int source_x_base = reversed ? source_width - 1 : 0;
        const int source_x_direction = reversed ? -1 : 1;
        const int source_y_base = reversed ? 0 : source_height - 1;
        const int source_y_direction = reversed ? 1 : -1;
        for (int y = 0; y < buffer.height; ++y, v_fp += y_step) {
            uint16_t *destination = pixels + y * buffer.stride;
            const int source_x = source_x_base + source_x_direction * (int)(v_fp >> TINYQR_PREVIEW_FP_SHIFT);
            int64_t u_fp = (int64_t)crop_x << TINYQR_PREVIEW_FP_SHIFT;
            int source_y = source_y_base + source_y_direction * (int)(u_fp >> TINYQR_PREVIEW_FP_SHIFT);
            const uint8_t *source = y_plane + source_y * row_stride + source_x;
            for (int x = 0; x < buffer.width - 1; ++x, u_fp += x_step) {
                const uint8_t luma = *source;
                destination[x] = (uint16_t)(((luma >> 3) << 11) | ((luma >> 2) << 5) | (luma >> 3));
                const int next_y = source_y_base + source_y_direction * (int)((u_fp + x_step) >> TINYQR_PREVIEW_FP_SHIFT);
                source += (next_y - source_y) * row_stride;
                source_y = next_y;
            }
            const uint8_t luma = *source;
            destination[buffer.width - 1] = (uint16_t)(((luma >> 3) << 11) | ((luma >> 2) << 5) | (luma >> 3));
        }
    }
    draw_overlay(pixels, buffer.width, buffer.height, buffer.stride, frame_index); ANativeWindow_unlockAndPost(window);
}
