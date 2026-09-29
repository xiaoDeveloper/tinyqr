plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.xiao.tinyqr"
    enableKotlin = false

    compileSdk {
        version = release(36)
    }

    defaultConfig {
        applicationId = "com.xiao.tinyqr"
        minSdk = 24
        targetSdk = 36

        versionCode = 8
        versionName = "1.0.0"

        ndk {
            abiFilters += listOf("arm64-v8a")
        }

        externalNativeBuild {
            cmake {
                arguments += listOf("-DTINYQR_CHALLENGE_BUILD=OFF")
            }
        }
    }

    bundle {
        abi {
            enableSplit = false
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/c/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}
