$cameraSource = Get-Content -Raw "$PSScriptRoot/../app/src/main/c/camera/camera.c"
if ($cameraSource -match 'ACameraDevice_StateCallbacks callbacks = \{[^}]*on_device_opened') {
    throw 'ACameraManager_openCamera has no onOpened callback; session setup must follow its successful return.'
}
