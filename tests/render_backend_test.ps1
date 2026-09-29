$ErrorActionPreference = 'Stop'
$render = Get-Content -Raw "$PSScriptRoot/../app/src/main/c/render/render.c"
$cmake = Get-Content -Raw "$PSScriptRoot/../app/src/main/c/CMakeLists.txt"

foreach ($required in @('ANativeWindow_lock', 'ANativeWindow_unlockAndPost', 'TinyQrResultLayout', 'tinyqr_result_layout')) {
    if ($render -notmatch "\b$required\b") {
        throw "Result renderer must use the same CPU NativeWindow producer as scan preview: missing $required."
    }
}
if ([regex]::IsMatch($render, '\b(egl[A-Z]|gl[A-Z]|EGL_|GL_)', [System.Text.RegularExpressions.RegexOptions]::CultureInvariant)) {
    throw 'Result renderer must not reconnect the NativeActivity Surface through EGL/GLES after CPU preview.'
}
if ($cmake -match '\bEGL\b|\bGLESv2\b') {
    throw 'TinyQR must not link EGL/GLESv2 after moving result rendering to the CPU producer.'
}
if ($render -notmatch 'if \(!buffer\.bits \|\| buffer\.width <= 0 \|\| buffer\.height <= 0\) \{ ANativeWindow_unlockAndPost\(window\); return; \}') {
    throw 'A rejected CPU result buffer must be unlocked before returning.'
}

Write-Output 'render backend tests passed'
