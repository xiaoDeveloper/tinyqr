$permission = Get-Content -Raw "$PSScriptRoot/../app/src/main/c/platform/permission.c"
$camera = Get-Content -Raw "$PSScriptRoot/../app/src/main/c/camera/camera.c"
if ($permission -match 'JNIEnv \*env = activity->env') { throw 'android_main must attach its own JNI environment.' }
if ($camera -match 'AImageReader_ImageListener\)\{' -or $camera -match 'ACameraDevice_StateCallbacks callbacks =') { throw 'Camera callbacks must live in TinyQrCamera, not a stack frame.' }
