# TinyQR

TinyQR is a small, fully offline QR scanner for Android. The application is written in C and runs through Android `NativeActivity`; it uses no Java or Kotlin application source, AndroidX, CameraX, Play Services, analytics, or network permission. The current build supports `arm64-v8a` devices running Android 7.0 (API 24) or later.

The size challenge is to keep a practical scanner below 200 KB, with 100 KB as a stretch goal. See [BENCHMARK.md](BENCHMARK.md) for the initial release measurement and how to reproduce it.

## How it works

`Camera2 NDK` and `AImageReader` supply YUV frames. Native code displays a grayscale preview through `ANativeWindow` and sends the Y plane to the vendored [quirc](app/src/main/c/third_party/quirc/UPSTREAM.md) decoder. A successful scan opens a native result screen with copy and rescan actions. Camera permission and clipboard access use small JNI bridges to Android framework APIs.

The normal decoder path includes a mirror retry. If normal candidates fail, a bounded fallback can reconstruct finder geometry for some rounded or stylized QR codes; ambiguous geometry is rejected. QR payloads are not logged.

The supported build uses Gradle and CMake. Its native release flags include `-Oz`, LTO, hidden visibility, section garbage collection, and symbol stripping; the optional `TINYQR_CHALLENGE_BUILD` profile is off. The separate [`experimental/raw-build/`](experimental/raw-build/README.md) script explores direct NDK packaging and is not the supported build.

## Build and test

Open this directory in Android Studio, install the SDK/NDK components requested by the project, and run the `app` configuration on an `arm64-v8a` device. The project uses Gradle 9.5.0, Android Gradle Plugin 9.3.0-rc01, compile/target SDK 36, and CMake 3.22.1. A local SDK location can be supplied through Android Studio or an ignored `local.properties` file; do not commit machine-specific paths.

From the project root, build with:

```powershell
.\gradlew.bat :app:assembleDebug
.\gradlew.bat :app:assembleRelease
```

The release task produces an unsigned APK. Host-side decoder tests require `gcc` on `PATH`:

```powershell
.\tests\run_qr_decoder_test.ps1
```

The decoder suite covers standard QR cases, rotation, mirroring, row stride, damaged data, reuse, and negative cases. An optional development photo for stylized QR verification is not distributed; its regression case reports a skip when absent. The explicitly requested stylized-photo benchmark requires that fixture. Other native and static checks are in [`tests/`](tests/).

## Repository layout

- [`app/src/main/c/`](app/src/main/c/) — native application, camera, QR decoding, rendering, and vendored quirc.
- [`app/src/main/AndroidManifest.xml`](app/src/main/AndroidManifest.xml) and [`app/src/main/res/`](app/src/main/res/) — Android entry point and resources.
- [`tests/`](tests/) — host-side decoder, math, rendering, and static checks.
- [`tools/`](tools/) — APK size reporting and release measurement.
- [`CHANGELOG.md`](CHANGELOG.md) — release history.

quirc's ISC license and upstream details are preserved in [`app/src/main/c/third_party/quirc/`](app/src/main/c/third_party/quirc/), and its notice is packaged in [`THIRD_PARTY_NOTICES.txt`](app/src/main/assets/THIRD_PARTY_NOTICES.txt). TinyQR does not yet have a project-level open-source license; one must be selected before publishing it as licensed open source.
