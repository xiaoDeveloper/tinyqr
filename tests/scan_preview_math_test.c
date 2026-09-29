#include "../app/src/main/c/render/scan_preview_math.h"
#include <assert.h>

int main(void) {
    TinyQrScanArea area = tinyqr_scan_area(1080, 2400);
    assert(area.left >= 0 && area.top >= 0);
    assert(area.right <= 1080 && area.bottom <= 2400);
    assert(area.right - area.left == area.bottom - area.top);
    assert(tinyqr_scan_line_y(&area, 0) == area.top);
    assert(tinyqr_scan_line_y(&area, 60) == area.bottom - 1);
    assert(tinyqr_scan_line_y(&area, 120) == area.top);
    assert(tinyqr_center_crop_source_x(0, 1080, 2400, 640, 480) == 212);
    assert(tinyqr_center_crop_source_x(1079, 1080, 2400, 640, 480) == 427);
    assert(tinyqr_center_crop_source_y(0, 1080, 2400, 640, 480) == 0);
    assert(tinyqr_center_crop_source_y(2399, 1080, 2400, 640, 480) == 479);
    assert(tinyqr_center_crop_source_x(0, 2400, 1080, 640, 480) == 0);
    assert(tinyqr_center_crop_source_x(2399, 2400, 1080, 640, 480) == 639);
    assert(tinyqr_center_crop_source_y(0, 2400, 1080, 640, 480) == 96);
    assert(tinyqr_center_crop_source_y(1079, 2400, 1080, 640, 480) == 383);
    assert(tinyqr_preview_source_x(0, 0, 1080, 2400, 640, 480, 90) == 0);
    assert(tinyqr_preview_source_y(0, 0, 1080, 2400, 640, 480, 90) == 383);
    assert(tinyqr_preview_source_x(1079, 2399, 1080, 2400, 640, 480, 90) == 639);
    assert(tinyqr_preview_source_y(1079, 2399, 1080, 2400, 640, 480, 90) == 96);
    assert(tinyqr_preview_source_x(0, 0, 1080, 2400, 640, 480, 270) == 639);
    assert(tinyqr_preview_source_y(0, 0, 1080, 2400, 640, 480, 270) == 96);
    assert(tinyqr_preview_source_x(2399, 0, 2400, 1080, 640, 480, 0) == 639);
    assert(tinyqr_preview_source_y(2399, 0, 2400, 1080, 640, 480, 0) == 96);
    assert(tinyqr_rgb565_luma(0) == 0x0000);
    assert(tinyqr_rgb565_luma(255) == 0xffff);
    assert(tinyqr_rgb565_luma(128) == 0x8410);
    for (int index = 0; index < 9; ++index) {
        TinyQrPreviewRect rect = tinyqr_scan_overlay_rect(&area, 30, index);
        assert(rect.left >= 0 && rect.top >= 0);
        assert(rect.right <= 1080 && rect.bottom <= 2400);
        assert(rect.left < rect.right && rect.top < rect.bottom);
    }
    return 0;
}
