#include <jni.h>

#if defined(MOH_ANDROID_PLATFORM_LINKED)
#define MOH_PLATFORM_STATUS \
    "Android POSIX host layer linked (threads, timing and Win32 host primitives)."
#else
#define MOH_PLATFORM_STATUS "Android native bootstrap loaded."
#endif

extern "C" JNIEXPORT jstring JNICALL
Java_com_cleiltonbr_moheuropeanassault_MainActivity_nativeStatus(
    JNIEnv* env,
    jobject /* thiz */) {
    return env->NewStringUTF(
        MOH_PLATFORM_STATUS
        " Xbox game kernel, graphics, audio, input and generated game code "
        "are not connected yet.");
}
