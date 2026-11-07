plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

val recompGenDir = System.getenv("RECOMP_GEN_DIR")
val recompRootDir = System.getenv("RECOMP_ROOT_DIR")

android {
    namespace = "com.cleiltonbr.moheuropeanassault"
    compileSdk = 35
    ndkVersion = "27.2.12479018"
    defaultConfig {
        applicationId = "com.cleiltonbr.moheuropeanassault"
        minSdk = 26
        targetSdk = 35
        versionCode = 2
        versionName = "0.2.0-android-game-code-linked"
        ndk { abiFilters += "arm64-v8a" }
        externalNativeBuild {
            cmake {
                cppFlags += listOf("-std=c++17", "-Wall", "-Wextra")
                if (!recompGenDir.isNullOrBlank() && !recompRootDir.isNullOrBlank()) {
                    arguments += listOf(
                        "-DMOH_BUILD_GENERATED_RECOMP=ON",
                        "-DRECOMP_GEN_DIR=$recompGenDir",
                        "-DRECOMP_ROOT_DIR=$recompRootDir"
                    )
                } else {
                    arguments += "-DMOH_BUILD_GENERATED_RECOMP=OFF"
                }
            }
        }
    }
    externalNativeBuild {
        cmake { path = file("src/main/cpp/CMakeLists.txt"); version = "3.22.1" }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions { jvmTarget = "17" }
    buildTypes {
        debug { applicationIdSuffix = ".debug"; versionNameSuffix = "-debug" }
        release { isMinifyEnabled = false }
    }
}
