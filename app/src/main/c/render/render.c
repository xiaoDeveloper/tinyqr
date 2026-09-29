#include "render.h"
#include "result_ui.h"
#include "../tinyqr.h"
#include <stdint.h>

typedef struct { char character; uint8_t rows[7]; } Glyph;
#define TINYQR_REFERENCE_DENSITY 440
static const Glyph glyphs[] = {
    {' ', {0,0,0,0,0,0,0}}, {'?', {14,17,2,4,0,4,0}},
    {'A',{14,17,17,31,17,17,17}},{'B',{30,17,17,30,17,17,30}},{'C',{14,17,16,16,16,17,14}},{'D',{30,17,17,17,17,17,30}},{'E',{31,16,16,30,16,16,31}},{'F',{31,16,16,30,16,16,16}},{'G',{14,17,16,23,17,17,14}},{'H',{17,17,17,31,17,17,17}},{'I',{14,4,4,4,4,4,14}},{'J',{7,2,2,2,2,18,12}},{'K',{17,18,20,24,20,18,17}},{'L',{16,16,16,16,16,16,31}},{'M',{17,27,21,21,17,17,17}},{'N',{17,25,21,19,17,17,17}},{'O',{14,17,17,17,17,17,14}},{'P',{30,17,17,30,16,16,16}},{'Q',{14,17,17,17,21,18,13}},{'R',{30,17,17,30,20,18,17}},{'S',{15,16,16,14,1,1,30}},{'T',{31,4,4,4,4,4,4}},{'U',{17,17,17,17,17,17,14}},{'V',{17,17,17,17,17,10,4}},{'W',{17,27,21,21,17,17,17}},{'X',{17,17,10,4,10,17,17}},{'Y',{17,17,10,4,4,4,4}},{'Z',{31,1,2,4,8,16,31}},
    {'a',{0,0,14,1,15,17,15}},{'b',{16,16,30,17,17,17,30}},{'c',{0,0,14,16,16,17,14}},{'d',{1,1,15,17,17,17,15}},{'e',{0,0,14,17,31,16,14}},{'f',{6,8,28,8,8,8,8}},{'g',{0,0,15,17,15,1,14}},{'h',{16,16,30,17,17,17,17}},{'i',{4,0,12,4,4,4,14}},{'j',{2,0,6,2,2,18,12}},{'k',{16,16,18,20,24,20,18}},{'l',{12,4,4,4,4,4,14}},{'m',{0,0,26,21,21,17,17}},{'n',{0,0,30,17,17,17,17}},{'o',{0,0,14,17,17,17,14}},{'p',{0,0,30,17,17,30,16}},{'q',{0,0,15,17,17,15,1}},{'r',{0,0,22,25,16,16,16}},{'s',{0,0,15,16,14,1,30}},{'t',{8,8,28,8,8,9,6}},{'u',{0,0,17,17,17,19,13}},{'v',{0,0,17,17,17,10,4}},{'w',{0,0,17,17,21,21,10}},{'x',{0,0,17,10,4,10,17}},{'y',{0,0,17,17,15,1,14}},{'z',{0,0,31,2,4,8,31}},
    {'0',{14,17,19,21,25,17,14}},{'1',{4,12,4,4,4,4,14}},{'2',{14,17,1,2,4,8,31}},{'3',{30,1,1,14,1,1,30}},{'4',{2,6,10,18,31,2,2}},{'5',{31,16,16,30,1,1,30}},{'6',{14,16,16,30,17,17,14}},{'7',{31,1,2,4,8,8,8}},{'8',{14,17,17,14,17,17,14}},{'9',{14,17,17,15,1,1,14}},
    {':',{0,4,0,0,0,4,0}},{'.',{0,0,0,0,0,6,6}},{'/',{1,2,2,4,8,8,16}},{'-',{0,0,0,31,0,0,0}},{'_',{0,0,0,0,0,0,31}},{'=',{0,31,0,31,0,0,0}},{'%',{17,2,4,8,17,0,0}},{'#',{10,31,10,10,31,10,0}},{'+',{0,4,4,31,4,4,0}},{'@',{14,17,23,21,23,16,14}},{'!',{4,4,4,4,4,0,4}},{'&',{12,18,20,8,21,18,13}},{'(',{2,4,8,8,8,4,2}},{')',{8,4,2,2,2,4,8}},{'[',{14,8,8,8,8,8,14}},{']',{14,2,2,2,2,2,14}},{';',{0,4,0,0,4,4,8}},{',',{0,0,0,0,6,6,12}},{'\'',{4,4,8,0,0,0,0}},{'\"',{10,10,0,0,0,0,0}}
};

static uint16_t rgb565(uint32_t color) { return (uint16_t)(((color >> 8) & 0xF800) | ((color >> 5) & 0x07E0) | ((color >> 3) & 0x001F)); }
static int ui_px(int base, int density) { int value; if (density <= 0) density = TINYQR_REFERENCE_DENSITY; value = (base * density + TINYQR_REFERENCE_DENSITY / 2) / TINYQR_REFERENCE_DENSITY; return value < 1 ? 1 : value; }
static int clamp_int(int value, int min_value, int max_value) { if (value < min_value) return min_value; return value > max_value ? max_value : value; }
static uint8_t glyph_row(unsigned char character, int row) { for (unsigned int i = 0; i < sizeof(glyphs) / sizeof(glyphs[0]); ++i) if ((unsigned char)glyphs[i].character == character) return glyphs[i].rows[row]; return glyphs[1].rows[row]; }
static void fill_rect(uint16_t *pixels, int width, int height, int stride, int left, int top, int right, int bottom, uint16_t color) { if (left < 0) left = 0; if (top < 0) top = 0; if (right > width) right = width; if (bottom > height) bottom = height; for (int y = top; y < bottom; ++y) for (int x = left; x < right; ++x) pixels[y * stride + x] = color; }
static void character(uint16_t *pixels, int width, int height, int stride, int x, int y, unsigned char value, int scale, uint16_t color) { for (int row = 0; row < 7; ++row) for (int column = 0; column < 5; ++column) if (glyph_row(value, row) & (1u << (4 - column))) fill_rect(pixels, width, height, stride, x + column * scale, y + row * scale, x + (column + 1) * scale, y + (row + 1) * scale, color); }
static void text(uint16_t *pixels, int width, int height, int stride, const uint8_t *data, int length, int x, int y, int scale, int max_rows, int max_width, uint16_t color) { int column = 0, row = 0, columns = max_width / (6 * scale); if (columns <= 0) return; for (int i = 0; i < length && row < max_rows; ++i) { if (column == columns) { column = 0; ++row; } character(pixels, width, height, stride, x + column * 6 * scale, y + row * 9 * scale, data[i], scale, color); ++column; } }
static void text_single_line(uint16_t *pixels, int width, int height, int stride, const uint8_t *data, int length, int x, int y, int scale, uint16_t color) { for (int i = 0; i < length; ++i) character(pixels, width, height, stride, x + i * 6 * scale, y, data[i], scale, color); }
static int decimal(int value, uint8_t *output) { uint8_t reversed[5]; int count = 0; do { reversed[count++] = (uint8_t)('0' + value % 10); value /= 10; } while (value); for (int i = 0; i < count; ++i) output[i] = reversed[count - 1 - i]; return count; }
static void button_box(uint16_t *pixels, int width, int height, int stride, int left, int right, int top, int bottom, int thickness, uint16_t background, uint16_t border) { fill_rect(pixels, width, height, stride, left, top, right, bottom, background); fill_rect(pixels, width, height, stride, left, top, right, top + thickness, border); fill_rect(pixels, width, height, stride, left, bottom - thickness, right, bottom, border); fill_rect(pixels, width, height, stride, left, top, left + thickness, bottom, border); fill_rect(pixels, width, height, stride, right - thickness, top, right, bottom, border); }

void tinyqr_render_permission_required(ANativeWindow *window, int density_dpi) {
    ANativeWindow_Buffer buffer;
    if (!window || ANativeWindow_lock(window, &buffer, 0) != 0) return;
    if (!buffer.bits || buffer.width <= 0 || buffer.height <= 0) { ANativeWindow_unlockAndPost(window); return; }
    uint16_t *pixels = buffer.bits; int width = buffer.width, height = buffer.height, panel_width = width - ui_px(48, density_dpi);
    uint16_t background = rgb565(0x0B1020), surface = rgb565(0x141A2B), panel_alt = rgb565(0x141A2B), text_primary = rgb565(0xEAF6FF), text_secondary = rgb565(0x8FA3B8), accent = rgb565(0xC77DFF), primary = rgb565(0xC77DFF), on_primary = rgb565(0x101020), border = rgb565(0x39445F), divider = rgb565(0x242D48);
    static const uint8_t camera_access[] = "CAMERA ACCESS", required[] = "REQUIRED", allow_camera_access[] = "ALLOW CAMERA ACCESS", to_scan_qr_codes[] = "TO SCAN QR CODES", open_settings[] = "OPEN SETTINGS", tap_anywhere[] = "TAP ANYWHERE";
    if (panel_width > ui_px(560, density_dpi)) panel_width = ui_px(560, density_dpi);
    int panel_left = (width - panel_width) / 2, panel_right = panel_left + panel_width, panel_top = height / 2 - ui_px(176, density_dpi), panel_bottom = panel_top + ui_px(352, density_dpi), divider_y = panel_top + ui_px(118, density_dpi);
    int button_left = panel_left + ui_px(24, density_dpi), button_right = panel_right - ui_px(24, density_dpi), button_top = panel_top + ui_px(232, density_dpi), button_bottom = button_top + ui_px(56, density_dpi);
    int content_width = panel_right - panel_left - ui_px(24, density_dpi), button_content_width = button_right - button_left - ui_px(4, density_dpi);
    int camera_access_scale = panel_width >= ((int)sizeof(camera_access) - 1) * 24 ? 4 : panel_width >= ((int)sizeof(camera_access) - 1) * 18 ? 3 : 2;
    int required_scale = content_width >= ((int)sizeof(required) - 1) * 24 ? 4 : content_width >= ((int)sizeof(required) - 1) * 18 ? 3 : 2;
    int body_scale = content_width >= ((int)sizeof(allow_camera_access) - 1) * 18 ? 3 : 2;
    int button_scale = button_content_width >= ((int)sizeof(open_settings) - 1) * 24 ? 4 : button_content_width >= ((int)sizeof(open_settings) - 1) * 18 ? 3 : 2;
    int hint_scale = content_width >= ((int)sizeof(tap_anywhere) - 1) * 18 ? 3 : 2;
    fill_rect(pixels, width, height, buffer.stride, 0, 0, width, height, background);
    fill_rect(pixels, width, height, buffer.stride, panel_left, panel_top, panel_right, panel_bottom, divider);
    camera_access_scale = clamp_int(ui_px(camera_access_scale, density_dpi), 2, 6); required_scale = clamp_int(ui_px(required_scale, density_dpi), 2, 6); body_scale = clamp_int(ui_px(body_scale, density_dpi), 2, 6); button_scale = clamp_int(ui_px(button_scale, density_dpi), 2, 6); hint_scale = clamp_int(ui_px(hint_scale, density_dpi), 2, 6);
    int panel_border = ui_px(1, density_dpi), panel_inset = ui_px(3, density_dpi), divider_thickness = ui_px(2, density_dpi), button_border = ui_px(2, density_dpi);
    fill_rect(pixels, width, height, buffer.stride, panel_left + panel_border, panel_top + panel_border, panel_right - panel_border, panel_bottom - panel_border, border);
    fill_rect(pixels, width, height, buffer.stride, panel_left + panel_inset, panel_top + panel_inset, panel_right - panel_inset, panel_bottom - panel_inset, surface);
    fill_rect(pixels, width, height, buffer.stride, panel_left + panel_inset, panel_top + panel_inset, panel_right - panel_inset, divider_y, panel_alt);
    text_single_line(pixels, width, height, buffer.stride, camera_access, sizeof(camera_access) - 1, (width - ((int)sizeof(camera_access) - 1) * 6 * camera_access_scale) / 2, panel_top + ui_px(28, density_dpi), camera_access_scale, text_primary);
    text_single_line(pixels, width, height, buffer.stride, required, sizeof(required) - 1, (width - ((int)sizeof(required) - 1) * 6 * required_scale) / 2, panel_top + ui_px(70, density_dpi), required_scale, accent);
    fill_rect(pixels, width, height, buffer.stride, panel_left + ui_px(16, density_dpi), divider_y, panel_right - ui_px(16, density_dpi), divider_y + divider_thickness, divider);
    text_single_line(pixels, width, height, buffer.stride, allow_camera_access, sizeof(allow_camera_access) - 1, (width - ((int)sizeof(allow_camera_access) - 1) * 6 * body_scale) / 2, panel_top + ui_px(145, density_dpi), body_scale, text_secondary);
    text_single_line(pixels, width, height, buffer.stride, to_scan_qr_codes, sizeof(to_scan_qr_codes) - 1, (width - ((int)sizeof(to_scan_qr_codes) - 1) * 6 * body_scale) / 2, panel_top + ui_px(181, density_dpi), body_scale, text_secondary);
    button_box(pixels, width, height, buffer.stride, button_left, button_right, button_top, button_bottom, button_border, primary, accent);
    text_single_line(pixels, width, height, buffer.stride, open_settings, sizeof(open_settings) - 1, (width - ((int)sizeof(open_settings) - 1) * 6 * button_scale) / 2, button_top + (button_bottom - button_top - 7 * button_scale) / 2, button_scale, on_primary);
    text_single_line(pixels, width, height, buffer.stride, tap_anywhere, sizeof(tap_anywhere) - 1, (width - ((int)sizeof(tap_anywhere) - 1) * 6 * hint_scale) / 2, panel_top + ui_px(314, density_dpi), hint_scale, text_secondary);
    ANativeWindow_unlockAndPost(window);
}

void tinyqr_render_result(ANativeWindow *window, const TinyQrStoredResult *result, bool copied, int density_dpi) {
    ANativeWindow_Buffer buffer;
    if (!window || !result || !result->ready || ANativeWindow_lock(window, &buffer, 0) != 0) return;
    if (!buffer.bits || buffer.width <= 0 || buffer.height <= 0) { ANativeWindow_unlockAndPost(window); return; }
    uint16_t *pixels = buffer.bits; int width = buffer.width, height = buffer.height; uint16_t background = rgb565(0x0B1020), card = rgb565(0x141A2B), text_primary = rgb565(0xEAF6FF), text_secondary = rgb565(0x8FA3B8), cyan = rgb565(0x4DEEEA), primary = rgb565(0xC77DFF), on_primary = rgb565(0x101020), border = rgb565(0x39445F), divider = rgb565(0x242D48);
    int content_padding = ui_px(20, density_dpi), title_top_padding = ui_px(32, density_dpi), title_content_gap = ui_px(54, density_dpi), content_bottom_padding = ui_px(16, density_dpi), action_padding = ui_px(12, density_dpi), max_button_height = ui_px(52, density_dpi), border_thickness = ui_px(2, density_dpi), divider_half = ui_px(1, density_dpi);
    int title_scale = clamp_int(ui_px(4, density_dpi), 3, 7);
    int base_content_scale = result->length <= 80 ? 4 : result->length <= 180 ? 3 : 2;
    TinyQrResultLayout layout = tinyqr_result_layout(width, height); bool printable_ascii = tinyqr_result_is_printable_ascii(result->data, result->length), utf8_text = tinyqr_result_is_utf8_text(result->data, result->length); int content_scale = clamp_int(ui_px(base_content_scale, density_dpi), 2, 6); int content_x = layout.card_left + content_padding, content_width = layout.card_right - content_padding - content_x, title_y = layout.card_top + title_top_padding, content_y = title_y + title_content_gap, max_rows = (layout.actions_top - content_y - content_bottom_padding) / (9 * content_scale), button_height = layout.actions_bottom - layout.actions_top - action_padding * 2, button_top, button_bottom;
    if (max_rows < 1) max_rows = 1;
    if (button_height > max_button_height) button_height = max_button_height;
    button_top = (layout.actions_top + layout.actions_bottom - button_height) / 2;
    button_bottom = button_top + button_height;
    fill_rect(pixels, width, height, buffer.stride, 0, 0, width, height, background);
    fill_rect(pixels, width, height, buffer.stride, layout.card_left, layout.card_top, layout.card_right, layout.card_bottom, border);
    fill_rect(pixels, width, height, buffer.stride, layout.card_left + border_thickness, layout.card_top + border_thickness, layout.card_right - border_thickness, layout.card_bottom - border_thickness, card);
    static const uint8_t title[] = "QR RESULT", copy[] = "COPY", copied_label[] = "COPIED", rescan[] = "RESCAN", binary[] = "BINARY QR", utf8[] = "UTF-8 TEXT";
    int action_visible_width = layout.copy_right - layout.copy_left - action_padding * 2;
    int rescan_visible_width = layout.rescan_right - layout.rescan_left - action_padding * 2;
    int action_label_length = (int)sizeof(rescan) - 1;
    int base_action_scale = action_visible_width >= (action_label_length * 6 - 1) * 4 && rescan_visible_width >= (action_label_length * 6 - 1) * 4 ? 4 : action_visible_width >= (action_label_length * 6 - 1) * 3 && rescan_visible_width >= (action_label_length * 6 - 1) * 3 ? 3 : 2;
    int action_scale = clamp_int(ui_px(base_action_scale, density_dpi), 3, 6);
    text_single_line(pixels, width, height, buffer.stride, title, sizeof(title) - 1, content_x, title_y, title_scale, text_primary);
    if (printable_ascii) text(pixels, width, height, buffer.stride, result->data, result->length, content_x, content_y, content_scale, max_rows, content_width, text_primary);
    else { uint8_t count[11]; int count_length = decimal(result->length, count); count[count_length++] = ' '; count[count_length++] = 'B'; count[count_length++] = 'Y'; count[count_length++] = 'T'; count[count_length++] = 'E'; count[count_length++] = 'S'; if (utf8_text) text_single_line(pixels, width, height, buffer.stride, utf8, sizeof(utf8) - 1, content_x, content_y, content_scale, text_primary); else text_single_line(pixels, width, height, buffer.stride, binary, sizeof(binary) - 1, content_x, content_y, content_scale, text_primary); text_single_line(pixels, width, height, buffer.stride, count, count_length, content_x, content_y + 9 * content_scale, content_scale, text_secondary); }
    fill_rect(pixels, width, height, buffer.stride, content_x, layout.actions_top - divider_half, layout.card_right - content_padding, layout.actions_top + divider_half, divider);
    if (utf8_text) { const uint8_t *label = copied ? copied_label : copy; int length = copied ? (int)sizeof(copied_label) - 1 : (int)sizeof(copy) - 1; int label_x = (layout.copy_left + action_padding + layout.copy_right - action_padding - (length * 6 - 1) * action_scale) / 2; button_box(pixels, width, height, buffer.stride, layout.copy_left + action_padding, layout.copy_right - action_padding, button_top, button_bottom, border_thickness, primary, primary); text_single_line(pixels, width, height, buffer.stride, label, length, label_x, button_top + (button_bottom - button_top - 7 * action_scale) / 2, action_scale, on_primary); }
    button_box(pixels, width, height, buffer.stride, layout.rescan_left + action_padding, layout.rescan_right - action_padding, button_top, button_bottom, border_thickness, card, cyan);
    text_single_line(pixels, width, height, buffer.stride, rescan, sizeof(rescan) - 1, (layout.rescan_left + action_padding + layout.rescan_right - action_padding - (((int)sizeof(rescan) - 1) * 6 - 1) * action_scale) / 2, button_top + (button_bottom - button_top - 7 * action_scale) / 2, action_scale, cyan);
    ANativeWindow_unlockAndPost(window); TQ_LOG("Result render posted: %dx%d", width, height);
}
