param(
    [Parameter(Mandatory)] [string] $Ndk,
    [Parameter(Mandatory)] [string] $Sdk,
    [Parameter(Mandatory)] [string] $Keystore,
    [Parameter(Mandatory)] [string] $KeyAlias
)
$ErrorActionPreference = 'Stop'
$root = Resolve-Path "$PSScriptRoot/../.."
$out = Join-Path $PSScriptRoot 'out'
New-Item -ItemType Directory -Force $out | Out-Null
$clang = Join-Path $Ndk 'toolchains/llvm/prebuilt/windows-x86_64/bin/aarch64-linux-android24-clang.exe'
$glue = Join-Path $Ndk 'sources/android/native_app_glue'
$so = Join-Path $out 'libtinyqr.so'
& $clang -Oz -ffunction-sections -fdata-sections -fvisibility=hidden -flto -fPIC -shared `
  "-I$glue" "-I$root/app/src/main/c" "$root/app/src/main/c/main.c" `
  "$root/app/src/main/c/platform/permission.c" "$root/app/src/main/c/camera/camera.c" `
  "$root/app/src/main/c/qr/qr.c" "$glue/android_native_app_glue.c" `
  -Wl,--gc-sections -Wl,--strip-all -u ANativeActivity_onCreate -landroid -llog -lcamera2ndk -lmediandk -o $so
Write-Host 'Native library built. Package this library under lib/arm64-v8a/ with aapt2, zipalign, and apksigner; keep that packaging invocation and the resulting size report with the experiment.'
