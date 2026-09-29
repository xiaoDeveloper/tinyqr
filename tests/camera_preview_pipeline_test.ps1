$cameraHeader = Get-Content -Raw "$PSScriptRoot/../app/src/main/c/camera/camera.h"
$cameraSource = Get-Content -Raw "$PSScriptRoot/../app/src/main/c/camera/camera.c"
$mainSource = Get-Content -Raw "$PSScriptRoot/../app/src/main/c/main.c"

if ($cameraHeader -notmatch 'tinyqr_camera_start\s*\(\s*TinyQrCamera \*camera\s*,\s*TinyQrDecoder \*decoder\s*,\s*TinyQrScanCallback scan_callback\s*,\s*void \*scan_context\s*,\s*TinyQrCameraStoppedCallback stopped_callback\s*,\s*void \*stopped_context\s*\)' -or
    $cameraHeader -notmatch 'tinyqr_camera_set_preview_window\s*\(\s*TinyQrCamera \*camera\s*,\s*ANativeWindow \*window\s*\)') {
    throw 'Camera must accept a borrowed decoder and callbacks, with the preview window managed separately.'
}

foreach ($member in @('reader_target', 'reader_output', 'preview_window')) {
    if ($cameraHeader -notmatch "\b$member\b") { throw "TinyQrCamera must track $member explicitly." }
}
foreach ($removed in @('preview_target', 'preview_output')) {
    if ($cameraHeader -match "\b$removed\b") { throw "TinyQrCamera must not retain obsolete Camera2 $removed state." }
}

foreach ($required in @(
    'ACameraOutputTarget_create\(camera->reader_window, &camera->reader_target\)',
    'ACaptureRequest_addTarget\(camera->request, camera->reader_target\)',
    'ACaptureSessionOutput_create\(camera->reader_window, &camera->reader_output\)',
    'ACaptureSessionOutputContainer_add\(camera->outputs, camera->reader_output\)',
    'ANativeWindow_acquire\(window\)',
    'ANativeWindow_release\(camera->preview_window\)',
    'ANativeWindow_setBuffersGeometry\(window, 0, 0, WINDOW_FORMAT_RGB_565\)',
    'tinyqr_render_scan_frame\(camera->preview_window, data, width, height, stride, camera->frame_count, camera->sensor_orientation\)'
)) {
    if ($cameraSource -notmatch $required) { throw "Missing required preview-pipeline operation: $required" }
}

foreach ($forbidden in @(
    'ACameraOutputTarget_create\(camera->preview_window',
    'ACaptureRequest_addTarget\(camera->request, camera->preview_target\)',
    'ACaptureSessionOutput_create\(camera->preview_window',
    'ACaptureSessionOutputContainer_add\(camera->outputs, camera->preview_output\)'
)) {
    if ($cameraSource -match $forbidden) { throw "Camera2 must not capture directly into the NativeActivity window: $forbidden" }
}

if ($cameraSource -match 'ANativeWindow_release\(camera->reader_window\)') {
    throw 'AImageReader owns reader_window; it must not be released independently.'
}

if ($cameraSource -notmatch 'if \(valid\)\s*\{\s*if \(camera->preview_window\)\s*tinyqr_render_scan_frame\(camera->preview_window, data, width, height, stride, camera->frame_count, camera->sensor_orientation\);\s*if \(camera->frame_count % 3 == 0\)') {
    throw 'Preview must render valid frames while QR analysis keeps its every-third-frame cadence.'
}

$previewAcquireCount = [regex]::Matches($cameraSource, 'ANativeWindow_acquire\(window\)').Count
$previewReleaseCount = [regex]::Matches($cameraSource, 'ANativeWindow_release\(camera->preview_window\)').Count
if ($previewAcquireCount -ne 1 -or $previewReleaseCount -ne 1) {
    throw 'The camera module must acquire and release exactly one preview-window reference.'
}

if ($cameraSource -match 'camera->session\s*==\s*session') {
    throw 'onReady must not require the asynchronously stored session pointer before starting its first request.'
}

if ($cameraSource -notmatch 'if \(!camera->opening \|\| !camera->request \|\| !session\) return false;' -or
    $cameraSource -notmatch 'if \(camera->repeating\) return true;') {
    throw 'Repeating-request submission must be active-only and idempotent.'
}

if ($cameraSource -notmatch 'ACameraDevice_createCaptureSession\([^;]+&camera->session\)\s*!=\s*ACAMERA_OK\)\s*goto fail;\s*if \(!start_repeating\(camera, camera->session\)\) goto fail;') {
    throw 'The first repeating request must be submitted immediately after session creation; onReady only reports an already idle session.'
}

if ($mainSource -notmatch 'state->window_ready\s*&&\s*state->app->window\s*&&\s*!tinyqr_camera_set_preview_window\(&state->camera, state->app->window\)') {
    throw 'A ready NativeActivity window must be attached before starting the camera.'
}

if (-not [regex]::IsMatch($mainSource, 'tinyqr_camera_set_preview_window\(&state->camera, state->app->window\)') -or
    -not [regex]::IsMatch($mainSource, 'tinyqr_camera_start\(&state->camera, state->decoder,\s*on_scan, state, on_camera_stopped, state\)')) {
    throw 'The app must set its preview window and pass the decoder and callback state to the camera module.'
}

$renderIndex = $cameraSource.IndexOf('tinyqr_render_scan_frame(')
$imageDeleteIndex = $cameraSource.IndexOf('AImage_delete(image);')
if ($renderIndex -lt 0 -or $imageDeleteIndex -lt 0 -or $renderIndex -ge $imageDeleteIndex) {
    throw 'Preview rendering must consume the borrowed Y plane before AImage_delete(image).'
}

if ($cameraHeader -notmatch '\bsensor_orientation\b' -or
    $cameraSource -notmatch 'ACAMERA_SENSOR_ORIENTATION' -or
    $cameraSource -notmatch 'tinyqr_render_scan_frame\(camera->preview_window, data, width, height, stride, camera->frame_count, camera->sensor_orientation\)') {
    throw 'Portrait preview must pass selected-camera sensor orientation to the native renderer.'
}
