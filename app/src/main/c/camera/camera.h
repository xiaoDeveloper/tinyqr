#ifndef TINYQR_CAMERA_H
#define TINYQR_CAMERA_H
#include <stdbool.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraCaptureSession.h>
#include <media/NdkImageReader.h>
#include "../qr/qr.h"
typedef void (*TinyQrScanCallback)(void *context, const uint8_t *data, int length);
typedef void (*TinyQrCameraStoppedCallback)(void *context);
typedef struct {
    ACameraManager *manager; ACameraDevice *device; AImageReader *reader;
    ANativeWindow *reader_window; ANativeWindow *preview_window;
    ACameraOutputTarget *reader_target;
    ACaptureRequest *request;
    ACaptureSessionOutput *reader_output;
    ACaptureSessionOutputContainer *outputs;
    ACameraCaptureSession *session; AImageReader_ImageListener image_listener;
    ACameraDevice_StateCallbacks device_callbacks; ACameraCaptureSession_stateCallbacks session_callbacks;
    TinyQrDecoder *decoder; TinyQrScanCallback scan_callback; void *scan_context;
    TinyQrCameraStoppedCallback stopped_callback; void *stopped_context;
    char camera_id[64]; int sensor_orientation; unsigned int frame_count; bool opening; bool repeating;
} TinyQrCamera;
bool tinyqr_camera_start(TinyQrCamera *camera, TinyQrDecoder *decoder,
                         TinyQrScanCallback scan_callback, void *scan_context,
                         TinyQrCameraStoppedCallback stopped_callback, void *stopped_context);
bool tinyqr_camera_set_preview_window(TinyQrCamera *camera, ANativeWindow *window);
void tinyqr_camera_stop(TinyQrCamera *camera);
#endif
