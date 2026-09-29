#ifndef TINYQR_PERMISSION_H
#define TINYQR_PERMISSION_H
#include <android/native_activity.h>
#include <stdbool.h>
bool tinyqr_camera_permission_granted(ANativeActivity *activity);
void tinyqr_request_camera_permission(ANativeActivity *activity);
void tinyqr_open_app_settings(ANativeActivity *activity);
#endif
