# Direct NDK build experiment

This directory is intentionally separate from Gradle/CMake. Its purpose is to measure the APK-size floor of direct NDK compilation and Android packaging, initially for `arm64-v8a` only.

`build.ps1` records the exact tool chain: `aarch64-linux-android24-clang`, `aapt2`, `zipalign`, and `apksigner`. It is an experiment, not the supported development build; the Gradle/CMake release is the reproducible primary build.

The script has not been run as part of the initial foundation because it needs a configured signing key and an `aapt2` packaging pass on the local SDK. Its output must be measured with `tools/apk-size-report.py`; do not copy Gradle measurements into this experiment.
