cmake_minimum_required(VERSION 3.22.1)
project(tinyqr C)
option(TINYQR_CHALLENGE_BUILD "Enable separately measured experimental native flags" OFF)

set(TINYQR_GLUE_DIR "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")
add_library(tinyqr SHARED main.c platform/permission.c platform/clipboard.c camera/camera.c qr/qr.c scan/scan_result.c render/result_ui.c render/render.c render/scan_preview.c render/scan_preview_math.c
    third_party/quirc/quirc.c third_party/quirc/identify.c third_party/quirc/decode.c third_party/quirc/version_db.c
    ${TINYQR_GLUE_DIR}/android_native_app_glue.c)
target_include_directories(tinyqr PRIVATE . ${TINYQR_GLUE_DIR} third_party/quirc)
target_compile_options(tinyqr PRIVATE -Oz -ffunction-sections -fdata-sections -fvisibility=hidden -fomit-frame-pointer -flto)
target_link_options(tinyqr PRIVATE -Wl,--gc-sections -Wl,--strip-all -flto)
target_link_libraries(tinyqr android log camera2ndk mediandk)
if(TINYQR_CHALLENGE_BUILD)
    target_compile_definitions(tinyqr PRIVATE TINYQR_CHALLENGE_BUILD=1)
endif()
