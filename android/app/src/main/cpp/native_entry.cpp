#include <jni.h>

extern "C" JNIEXPORT jstring JNICALL
Java_com_cleiltonbr_moheuropeanassault_MainActivity_nativeStatus(
    JNIEnv* env,
    jobject /* thiz */) {
    return env->NewStringUTF(
        "Native ARM64 library loaded. Game runtime is not connected yet.");
}
