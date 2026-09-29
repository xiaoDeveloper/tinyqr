$ErrorActionPreference = 'Stop'
$camera = Get-Content -Raw "$PSScriptRoot/../app/src/main/c/camera/camera.c"
$header = Get-Content -Raw "$PSScriptRoot/../app/src/main/c/camera/camera.h"
$main = Get-Content -Raw "$PSScriptRoot/../app/src/main/c/main.c"
$scan = Get-Content -Raw "$PSScriptRoot/../app/src/main/c/scan/scan_result.c"

foreach ($required in @('TinyQrScanCallback', 'scan_callback', 'scan_context')) {
    if ($header -notmatch "\b$required\b") { throw "Camera result API is missing $required." }
}
if ($header -notmatch 'TinyQrCameraStoppedCallback') {
    throw 'Camera API must report when Camera2 has actually released its output Surface.'
}
if ($camera -notmatch 'onClosed\s*=\s*on_session_closed') {
    throw 'Result rendering must wait for the Camera2 session onClosed callback.'
}
if ($camera -notmatch 'callback\(callback_context, result.data, result.length\)') {
    throw 'Camera must report decoded bytes through its callback.'
}
$imageDeleteIndex = $camera.IndexOf('AImage_delete(image);')
$resultCallbackIndex = $camera.IndexOf('callback(callback_context, result.data, result.length);')
if ($imageDeleteIndex -lt 0 -or $resultCallbackIndex -lt 0 -or $imageDeleteIndex -gt $resultCallbackIndex) {
    throw 'The acquired AImage must be returned before the result callback can wake camera teardown.'
}
if ($camera -match 'tinyqr_camera_stop\(camera\).*on_image') {
    throw 'Camera callback must not stop the camera directly.'
}
if ($camera -notmatch 'SIZE_MAX' -or $camera -notmatch '\(size_t\)length >= required') {
    throw 'Y plane access must validate the final readable byte against the supplied plane length.'
}
if ($main -notmatch 'TINYQR_MODE_SCANNING' -or $main -notmatch 'TINYQR_MODE_RESULT') {
    throw 'App must explicitly distinguish scanning and result modes.'
}
if ($main -notmatch 'ALooper_wake\(state->app->looper\)' -or $main -notmatch 'tinyqr_camera_stop\(&state->camera\)') {
    throw 'Result publication must wake the app loop and the app loop must stop the camera.'
}
if ($main -match 'while\s*\(\s*ALooper_pollOnce\([^\r\n]+\)\s*>=\s*0\s*\)') {
    throw 'ALOOPER_POLL_WAKE is -1; result consumption must not be gated by a non-negative poll result.'
}
if (-not [regex]::IsMatch($main, 'ALooper_pollOnce\([^;]+;[\s\S]{0,300}consume_scan_result\(&state\)', 'Singleline')) {
    throw 'Every looper return, including ALOOPER_POLL_WAKE, must be followed by pending-result consumption.'
}
if ($main -notmatch 'app->onInputEvent = on_input_event' -or $main -notmatch 'AMOTION_EVENT_ACTION_UP') {
    throw 'Result mode must provide a native tap-to-rescan path.'
}
if ($main -notmatch 'tinyqr_pending_result_reset\(&state->pending_result\)') {
    throw 'Rescan must explicitly reopen result publication after in-flight camera callbacks are suppressed.'
}
if ($main -notmatch 'state->mode != TINYQR_MODE_SCANNING') {
    throw 'Lifecycle camera start must be gated by scanning mode.'
}
if ($main -notmatch 'on_camera_stopped' -or $main -notmatch 'camera_surface_ready') {
    throw 'The app thread must gate result rendering on asynchronous Camera2 Surface release.'
}
if ($main -match 'tinyqr_camera_stop\(&state->camera\);\s*render_result\(state\)') {
    throw 'ANativeWindow rendering must not begin synchronously after closing Camera2.'
}
if ($scan -match 'strlen\s*\(') { throw 'Result state must use explicit lengths, not strlen.' }
if ($camera -match 'startActivity|Intent|clipboard|browser') { throw 'Camera must not execute QR contents.' }

$logging = Get-Content -Raw "$PSScriptRoot/../app/src/main/c/tinyqr.h"
if ($logging -notmatch '!defined\(NDEBUG\)') { throw 'Debug builds must compile TinyQR diagnostic logs in.' }
foreach ($message in @('Result published', 'Result consumed', 'Result render')) {
    if ($main -notmatch $message) { throw "Missing diagnostic boundary log: $message" }
}
foreach ($message in @('Camera started', 'Camera analysis active', 'Camera device error')) {
    if ($camera -notmatch $message) { throw "Missing camera diagnostic boundary log: $message" }
}
foreach ($message in @('Camera stop: session', 'Camera stop: reader', 'Camera stop: device', 'Camera stopped')) {
    if ($camera -notmatch $message) { throw "Missing camera teardown diagnostic: $message" }
}
$stopSource = $camera.Substring($camera.IndexOf('void tinyqr_camera_stop('))
$sessionCloseIndex = $stopSource.IndexOf('ACameraCaptureSession_close(camera->session);')
$deviceCloseIndex = $stopSource.IndexOf('ACameraDevice_close(camera->device);')
$readerDeleteIndex = $stopSource.IndexOf('AImageReader_delete(camera->reader);')
$previewReleaseIndex = $stopSource.IndexOf('tinyqr_camera_set_preview_window(camera, 0);')
if ($sessionCloseIndex -lt 0 -or $deviceCloseIndex -lt 0 -or $readerDeleteIndex -lt 0 -or $previewReleaseIndex -lt 0 -or
    $sessionCloseIndex -gt $deviceCloseIndex -or $deviceCloseIndex -gt $readerDeleteIndex -or
    $readerDeleteIndex -gt $previewReleaseIndex) {
    throw 'Teardown must close the session and blocking camera device before deleting ImageReader and releasing preview.'
}
