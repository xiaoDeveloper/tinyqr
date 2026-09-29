#include "camera.h"
#include "../qr/qr.h"
#include "../render/scan_preview.h"
#include "../tinyqr.h"
#include <camera/NdkCameraMetadata.h>
#include <camera/NdkCameraMetadataTags.h>
#include <media/NdkImage.h>
#include <stdint.h>
#include <string.h>

static void on_image(void *context, AImageReader *reader) {
    TinyQrCamera *camera = context; AImage *image = 0; uint8_t *data = 0;
    TinyQrScanCallback callback = 0; void *callback_context = 0; TinyQrResult result = {0};
    int32_t width = 0, height = 0, stride = 0, length = 0;
    if (AImageReader_acquireLatestImage(reader, &image) != AMEDIA_OK) return;
    ++camera->frame_count;
    if (AImage_getWidth(image, &width) == AMEDIA_OK && AImage_getHeight(image, &height) == AMEDIA_OK &&
        AImage_getPlaneRowStride(image, 0, &stride) == AMEDIA_OK &&
        AImage_getPlaneData(image, 0, &data, &length) == AMEDIA_OK) {
        bool valid = width > 0 && height > 0 && stride >= width && length >= 0;
        size_t required = 0;
        if (valid && (size_t)(height - 1) <= SIZE_MAX / (size_t)stride) {
            required = (size_t)(height - 1) * (size_t)stride;
            valid = required <= SIZE_MAX - (size_t)width;
            required += (size_t)width;
            valid = valid && (size_t)length >= required;
        } else valid = false;
        if (valid) {
            if (camera->preview_window)
                tinyqr_render_scan_frame(camera->preview_window, data, width, height, stride, camera->frame_count, camera->sensor_orientation);
            if (camera->frame_count % 3 == 0) {
                if (camera->frame_count == 3) TQ_LOG("Camera analysis active: %dx%d stride=%d", width, height, stride);
                TinyQrFrame frame = { data, width, height, stride };
                if (camera->scan_callback && tinyqr_decode(camera->decoder, &frame, &result)) {
                    callback = camera->scan_callback;
                    callback_context = camera->scan_context;
                }
            }
        }
    }
    AImage_delete(image);
    if (callback) callback(callback_context, result.data, result.length);
}

static bool select_back_camera(TinyQrCamera *camera) {
    ACameraIdList *ids = 0;
    if (ACameraManager_getCameraIdList(camera->manager, &ids) != ACAMERA_OK) return false;
    for (int i = 0; i < ids->numCameras; ++i) {
        ACameraMetadata *metadata = 0; ACameraMetadata_const_entry facing, orientation;
        if (ACameraManager_getCameraCharacteristics(camera->manager, ids->cameraIds[i], &metadata) == ACAMERA_OK &&
            ACameraMetadata_getConstEntry(metadata, ACAMERA_LENS_FACING, &facing) == ACAMERA_OK &&
            facing.count && facing.data.u8[0] == ACAMERA_LENS_FACING_BACK) {
            strncpy(camera->camera_id, ids->cameraIds[i], sizeof(camera->camera_id) - 1);
            if (ACameraMetadata_getConstEntry(metadata, ACAMERA_SENSOR_ORIENTATION, &orientation) == ACAMERA_OK && orientation.count) camera->sensor_orientation = orientation.data.i32[0];
            ACameraMetadata_free(metadata); ACameraManager_deleteCameraIdList(ids); return true;
        }
        if (metadata) ACameraMetadata_free(metadata);
    }
    ACameraManager_deleteCameraIdList(ids); return false;
}

static bool start_repeating(TinyQrCamera *camera, ACameraCaptureSession *session) {
    int sequence_id = 0;
    if (!camera->opening || !camera->request || !session) return false;
    if (camera->repeating) return true;
    if (ACameraCaptureSession_setRepeatingRequest(session, 0, 1, &camera->request, &sequence_id) != ACAMERA_OK) return false;
    camera->repeating = true;
    return true;
}

static void on_session_ready(void *context, ACameraCaptureSession *session) {
    (void)start_repeating(context, session);
}

static void notify_stopped(TinyQrCamera *camera) {
    if (camera->stopped_callback) camera->stopped_callback(camera->stopped_context);
}

static void on_session_closed(void *context, ACameraCaptureSession *session) {
    (void)session;
    TinyQrCamera *camera = context;
    TQ_LOG("Camera session onClosed");
    notify_stopped(camera);
}

static bool configure_device(TinyQrCamera *camera) {
    ACameraDevice *device = camera->device;
    camera->image_listener.context = camera; camera->image_listener.onImageAvailable = on_image;
    camera->session_callbacks.context = camera; camera->session_callbacks.onClosed = on_session_closed;
    camera->session_callbacks.onReady = on_session_ready; camera->session_callbacks.onActive = 0;
    if (AImageReader_new(640, 480, AIMAGE_FORMAT_YUV_420_888, 2, &camera->reader) != AMEDIA_OK) goto fail;
    if (AImageReader_setImageListener(camera->reader, &camera->image_listener) != AMEDIA_OK) goto fail;
    if (AImageReader_getWindow(camera->reader, &camera->reader_window) != AMEDIA_OK) goto fail;
    if (ACameraOutputTarget_create(camera->reader_window, &camera->reader_target) != ACAMERA_OK) goto fail;
    if (ACameraDevice_createCaptureRequest(device, TEMPLATE_PREVIEW, &camera->request) != ACAMERA_OK) goto fail;
    if (ACaptureRequest_addTarget(camera->request, camera->reader_target) != ACAMERA_OK) goto fail;
    if (ACaptureSessionOutput_create(camera->reader_window, &camera->reader_output) != ACAMERA_OK) goto fail;
    if (ACaptureSessionOutputContainer_create(&camera->outputs) != ACAMERA_OK) goto fail;
    if (ACaptureSessionOutputContainer_add(camera->outputs, camera->reader_output) != ACAMERA_OK) goto fail;
    if (ACameraDevice_createCaptureSession(device, camera->outputs, &camera->session_callbacks, &camera->session) != ACAMERA_OK) goto fail;
    if (!start_repeating(camera, camera->session)) goto fail;
    return true;
fail:
    tinyqr_camera_stop(camera);
    return false;
}
static void on_device_disconnected(void *context, ACameraDevice *device) { (void)device; tinyqr_camera_stop(context); }
static void on_device_error(void *context, ACameraDevice *device, int error) {
    (void)device;
    TQ_LOG("Camera device error: %d", error);
    tinyqr_camera_stop(context);
}

bool tinyqr_camera_start(TinyQrCamera *camera, TinyQrDecoder *decoder,
                         TinyQrScanCallback scan_callback, void *scan_context,
                         TinyQrCameraStoppedCallback stopped_callback, void *stopped_context) {
    camera->device_callbacks.context = camera;
    camera->device_callbacks.onDisconnected = on_device_disconnected;
    camera->device_callbacks.onError = on_device_error;
    if (camera->opening) return true;
    if (!decoder || !scan_callback || !stopped_callback) return false;
    camera->decoder = decoder;
    camera->scan_callback = scan_callback;
    camera->scan_context = scan_context;
    camera->stopped_callback = stopped_callback;
    camera->stopped_context = stopped_context;
    if (!camera->manager) camera->manager = ACameraManager_create();
    if (!camera->manager || !select_back_camera(camera)) return false;
    camera->opening = true;
    if (ACameraManager_openCamera(camera->manager, camera->camera_id, &camera->device_callbacks, &camera->device) != ACAMERA_OK) {
        tinyqr_camera_stop(camera); return false;
    }
    bool configured = configure_device(camera);
    TQ_LOG(configured ? "Camera started" : "Camera start failed during configuration");
    return configured;
}

bool tinyqr_camera_set_preview_window(TinyQrCamera *camera, ANativeWindow *window) {
    if (camera->preview_window == window) return true;
    if (camera->preview_window) {
        ANativeWindow_release(camera->preview_window);
        camera->preview_window = 0;
    }
    if (!window) return true;
    if (ANativeWindow_setBuffersGeometry(window, 0, 0, WINDOW_FORMAT_RGB_565) != 0) return false;
    ANativeWindow_acquire(window);
    camera->preview_window = window;
    return true;
}

void tinyqr_camera_stop(TinyQrCamera *camera) {
    bool was_active = camera->opening || camera->session || camera->device;
    bool had_session = camera->session != 0;
    camera->opening = false; camera->repeating = false;
    if (camera->session) {
        TQ_LOG("Camera stop: session stop begin");
        ACameraCaptureSession_stopRepeating(camera->session);
        TQ_LOG("Camera stop: session close begin");
        ACameraCaptureSession_close(camera->session);
        camera->session = 0;
        TQ_LOG("Camera stop: session done");
    }
    if (camera->device) {
        TQ_LOG("Camera stop: device begin");
        ACameraDevice_close(camera->device); camera->device = 0;
        TQ_LOG("Camera stop: device done");
    }
    if (camera->request) { ACaptureRequest_free(camera->request); camera->request = 0; }
    if (camera->reader_target) { ACameraOutputTarget_free(camera->reader_target); camera->reader_target = 0; }
    if (camera->reader_output) { ACaptureSessionOutput_free(camera->reader_output); camera->reader_output = 0; }
    if (camera->outputs) { ACaptureSessionOutputContainer_free(camera->outputs); camera->outputs = 0; }
    if (camera->reader) {
        TQ_LOG("Camera stop: reader begin");
        AImageReader_delete(camera->reader); camera->reader = 0; camera->reader_window = 0;
        TQ_LOG("Camera stop: reader done");
    }
    tinyqr_camera_set_preview_window(camera, 0);
    camera->frame_count = 0;
    camera->scan_callback = 0;
    camera->scan_context = 0;
    if (was_active && !had_session) notify_stopped(camera);
    if (was_active) TQ_LOG("Camera stopped");
}
