#ifndef TINYQR_CLIPBOARD_H
#define TINYQR_CLIPBOARD_H

#include <android/native_activity.h>
#include <stdbool.h>
#include <stdint.h>

bool tinyqr_copy_to_clipboard(ANativeActivity *activity, const uint8_t *data, int length);

#endif
