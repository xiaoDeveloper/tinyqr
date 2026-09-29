#include "scan_preview_math.h"
TinyQrScanArea tinyqr_scan_area(int width, int height) {
    int size = (width < height ? width : height) * 3 / 4;
    TinyQrScanArea area = { (width - size) / 2, (height - size) / 2, 0, 0 };
    area.right = area.left + size; area.bottom = area.top + size; return area;
}
int tinyqr_scan_line_y(const TinyQrScanArea *area, uint32_t frame_index) {
    const int frames = 60; int height = area->bottom - area->top;
    int phase = (int)(frame_index % (frames * 2));
    if (phase > frames) phase = frames * 2 - phase;
    return area->top + phase * (height - 1) / frames;
}
int tinyqr_center_crop_source_x(int x, int destination_width, int destination_height, int source_width, int source_height) {
    if (source_width * destination_height > source_height * destination_width) {
        int visible = source_height * destination_width / destination_height;
        return (source_width - visible) / 2 + x * visible / destination_width;
    }
    return x * source_width / destination_width;
}
int tinyqr_center_crop_source_y(int y, int destination_width, int destination_height, int source_width, int source_height) {
    if (source_width * destination_height < source_height * destination_width) {
        int visible = source_width * destination_height / destination_width;
        return (source_height - visible) / 2 + y * visible / destination_height;
    }
    return y * source_height / destination_height;
}
int tinyqr_preview_source_x(int x, int y, int destination_width, int destination_height, int source_width, int source_height, int sensor_orientation) {
    if (sensor_orientation == 90 || sensor_orientation == 270) {
        int source_y = tinyqr_center_crop_source_y(y, destination_width, destination_height, source_height, source_width);
        return sensor_orientation == 90 ? source_y : source_width - 1 - source_y;
    }
    int source_x = tinyqr_center_crop_source_x(x, destination_width, destination_height, source_width, source_height);
    return sensor_orientation == 180 ? source_width - 1 - source_x : source_x;
}
int tinyqr_preview_source_y(int x, int y, int destination_width, int destination_height, int source_width, int source_height, int sensor_orientation) {
    if (sensor_orientation == 90 || sensor_orientation == 270) {
        int source_x = tinyqr_center_crop_source_x(x, destination_width, destination_height, source_height, source_width);
        return sensor_orientation == 90 ? source_height - 1 - source_x : source_x;
    }
    int source_y = tinyqr_center_crop_source_y(y, destination_width, destination_height, source_width, source_height);
    return sensor_orientation == 180 ? source_height - 1 - source_y : source_y;
}
uint16_t tinyqr_rgb565_luma(uint8_t luma) { return (uint16_t)(((luma >> 3) << 11) | ((luma >> 2) << 5) | (luma >> 3)); }
TinyQrPreviewRect tinyqr_scan_overlay_rect(const TinyQrScanArea *area, uint32_t frame_index, int index) {
    int size = area->right - area->left; int length = size / 7; int thickness = size * 4 / (3 * 270);
    if (thickness < 3) thickness = 3;
    int scan_y = tinyqr_scan_line_y(area, frame_index);
    switch (index) {
        case 0: return (TinyQrPreviewRect){area->left, area->top, area->left + length, area->top + thickness};
        case 1: return (TinyQrPreviewRect){area->left, area->top, area->left + thickness, area->top + length};
        case 2: return (TinyQrPreviewRect){area->right - length, area->top, area->right, area->top + thickness};
        case 3: return (TinyQrPreviewRect){area->right - thickness, area->top, area->right, area->top + length};
        case 4: return (TinyQrPreviewRect){area->left, area->bottom - thickness, area->left + length, area->bottom};
        case 5: return (TinyQrPreviewRect){area->left, area->bottom - length, area->left + thickness, area->bottom};
        case 6: return (TinyQrPreviewRect){area->right - length, area->bottom - thickness, area->right, area->bottom};
        case 7: return (TinyQrPreviewRect){area->right - thickness, area->bottom - length, area->right, area->bottom};
        default: return (TinyQrPreviewRect){area->left + thickness, scan_y, area->right - thickness, scan_y + thickness};
    }
}
