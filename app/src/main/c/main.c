#include "tinyqr.h"
#include "camera/camera.h"
#include "platform/permission.h"
#include "platform/clipboard.h"
#include "render/render.h"
#include "render/scan_preview.h"
#include "render/result_ui.h"
#include "scan/scan_result.h"
#include <android_native_app_glue.h>
#include <android/configuration.h>
#include <android/input.h>
#include <stdbool.h>
#include <string.h>

typedef enum { TINYQR_MODE_SCANNING, TINYQR_MODE_RESULT } TinyQrMode;
typedef struct {
    struct android_app *app; TinyQrCamera camera; TinyQrDecoder *decoder;
    TinyQrPendingResult pending_result; TinyQrStoredResult displayed_result;
    TinyQrMode mode; atomic_bool camera_surface_ready;
    bool result_render_pending; bool window_ready; bool permission_requested; bool camera_permission_known_granted; bool permission_blocked; bool copied;
} TinyQrApp;

static int tinyqr_density_dpi(struct android_app *app) {
    int density = 0;
    if (app && app->config) density = AConfiguration_getDensity(app->config);
    return density <= 0 || density > 1000 ? 440 : density;
}

static void render_result(TinyQrApp *state) {
    if (state->window_ready && state->app->window && state->mode == TINYQR_MODE_RESULT) {
        TQ_LOG("Result render requested: %d bytes", state->displayed_result.length);
        tinyqr_render_result(state->app->window, &state->displayed_result, state->copied,
                             tinyqr_density_dpi(state->app));
    } else {
        TQ_LOG("Result render deferred: window_ready=%d window=%p mode=%d",
               state->window_ready, state->app->window, state->mode);
    }
}

static void on_scan(void *context, const uint8_t *data, int length) {
    TinyQrApp *state = context;
    if (tinyqr_pending_result_publish(&state->pending_result, data, length)) {
        TQ_LOG("Result published: %d bytes", length);
        ALooper_wake(state->app->looper);
    } else {
        TQ_LOG("Result publish ignored: %d bytes", length);
    }
}
static void on_camera_stopped(void *context) {
    TinyQrApp *state = context;
    atomic_store_explicit(&state->camera_surface_ready, true, memory_order_release);
    TQ_LOG("Camera Surface released");
    ALooper_wake(state->app->looper);
}
static void try_start_camera(TinyQrApp *state) {
    if (state->mode != TINYQR_MODE_SCANNING) return;
    if (state->window_ready && state->app->window &&
        !tinyqr_camera_set_preview_window(&state->camera, state->app->window)) return;
    if (tinyqr_camera_permission_granted(state->app->activity)) {
        state->permission_blocked = false;
        state->camera_permission_known_granted = true;
        atomic_store_explicit(&state->camera_surface_ready, false, memory_order_release);
        if (!tinyqr_camera_start(&state->camera, state->decoder,
                                 on_scan, state, on_camera_stopped, state))
            atomic_store_explicit(&state->camera_surface_ready, true, memory_order_release);
    }
    else if (!state->permission_requested) { tinyqr_request_camera_permission(state->app->activity); state->permission_requested = true; }
    else {
        state->permission_blocked = true;
        if (state->window_ready && state->app->window)
            tinyqr_render_permission_required(state->app->window,
                                              tinyqr_density_dpi(state->app));
    }
}
static void render_result_if_ready(TinyQrApp *state) {
    if (!state->result_render_pending ||
        !atomic_load_explicit(&state->camera_surface_ready, memory_order_acquire)) return;
    state->result_render_pending = false;
    render_result(state);
}
static void consume_scan_result(TinyQrApp *state) {
    if (state->mode != TINYQR_MODE_SCANNING || !tinyqr_pending_result_consume(&state->pending_result, &state->displayed_result)) return;
    TQ_LOG("Result consumed: %d bytes", state->displayed_result.length);
    state->mode = TINYQR_MODE_RESULT;
    state->copied = false;
    state->result_render_pending = true;
    tinyqr_camera_stop(&state->camera);
}
static void begin_rescan(TinyQrApp *state) {
    tinyqr_stored_result_clear(&state->displayed_result);
    tinyqr_pending_result_reset(&state->pending_result);
    state->copied = false;
    state->result_render_pending = false;
    state->mode = TINYQR_MODE_SCANNING;
    TQ_LOG("Rescan requested");
    try_start_camera(state);
}

static int32_t on_input_event(struct android_app *app, AInputEvent *event) {
    TinyQrApp *state = app->userData;
    if (state && state->mode == TINYQR_MODE_SCANNING && state->permission_blocked && AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION &&
        (AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK) == AMOTION_EVENT_ACTION_UP) {
        tinyqr_open_app_settings(app->activity);
        return 1;
    }
    if (!state || state->mode != TINYQR_MODE_RESULT || AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION ||
        (AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK) != AMOTION_EVENT_ACTION_UP) return 0;
    float x = AMotionEvent_getX(event, 0);
    float y = AMotionEvent_getY(event, 0);
    TinyQrResultAction action = tinyqr_result_hit_test(ANativeWindow_getWidth(app->window), ANativeWindow_getHeight(app->window), x, y);
    if (action == TINYQR_RESULT_ACTION_RESCAN) { begin_rescan(state); return 1; }
    if (action == TINYQR_RESULT_ACTION_COPY &&
        tinyqr_copy_to_clipboard(app->activity, state->displayed_result.data, state->displayed_result.length)) {
        state->copied = true;
        render_result(state);
        return 1;
    }
    return action == TINYQR_RESULT_ACTION_COPY;
}
static void on_app_command(struct android_app *app, int32_t command) {
    TinyQrApp *state = app->userData;
    TQ_LOG("App command: %d mode=%d", command, state->mode);
    switch (command) {
        case APP_CMD_INIT_WINDOW:
            state->window_ready = true;
            if (state->mode == TINYQR_MODE_RESULT) {
                state->result_render_pending = true;
                render_result_if_ready(state);
            } else {
                if (!tinyqr_camera_set_preview_window(&state->camera, state->app->window)) break;
                tinyqr_render_boot_frame(state->app->window);
                try_start_camera(state);
            }
            break;
        case APP_CMD_RESUME: case APP_CMD_GAINED_FOCUS: try_start_camera(state); break;
        case APP_CMD_TERM_WINDOW:
            state->window_ready = false;
            tinyqr_camera_set_preview_window(&state->camera, 0);
            break;
        case APP_CMD_PAUSE: case APP_CMD_LOST_FOCUS: case APP_CMD_DESTROY:
            tinyqr_camera_stop(&state->camera); break;
    }
}
void android_main(struct android_app *app) {
    TinyQrApp state; memset(&state, 0, sizeof(state));
    state.pending_result = (TinyQrPendingResult)TINYQR_PENDING_RESULT_INIT;
    atomic_init(&state.camera_surface_ready, true);
    state.app = app; state.mode = TINYQR_MODE_SCANNING; state.decoder = tinyqr_decoder_create();
    app->userData = &state; app->onAppCmd = on_app_command; app->onInputEvent = on_input_event;
    if (!state.decoder) return;
    TQ_LOG("NativeActivity started");
    while (!app->destroyRequested) {
        int events; struct android_poll_source *source = 0;
        int poll_result = ALooper_pollOnce(-1, 0, &events, (void **)&source);
        if (poll_result >= 0 && source) source->process(app, source);
        if (poll_result == ALOOPER_POLL_WAKE) TQ_LOG("Looper woke for pending result");
        consume_scan_result(&state);
        render_result_if_ready(&state);
    }
    tinyqr_camera_stop(&state.camera); if (state.camera.manager) ACameraManager_delete(state.camera.manager); tinyqr_decoder_destroy(state.decoder);
}
