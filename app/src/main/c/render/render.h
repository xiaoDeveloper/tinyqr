#ifndef TINYQR_RENDER_H
#define TINYQR_RENDER_H

#include <android/native_window.h>
#include "../scan/scan_result.h"

void tinyqr_render_result(ANativeWindow *window, const TinyQrStoredResult *result, bool copied, int density_dpi);
void tinyqr_render_permission_required(ANativeWindow *window, int density_dpi);

#endif
