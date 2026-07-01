#include <jni.h>
#include <android/log.h>
#include "bare_syscall/syscall_bridge.h"
#include "micro_services/engine/service_engine.h"
#include "trusted_root/signing/signing_center.h"

#define LOG_TAG "ApexRoot-JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    (void)vm;
    LOGI("APEX Root v1.0.3 native library loaded");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL
Java_com_apex_root_ui_MainActivity_nativeStartSandbox(JNIEnv*, jobject) {
    LOGI("Starting sandbox process...");
    int pid = static_cast<int>(bs_fork());
    if (pid == 0) {
        bs_unshare(0x00020000);
        apex::engine::service_engine::initialize();
        while (true) {
            bs_nanosleep(500000000ULL);
        }
    } else if (pid > 0) {
        LOGI("Sandbox started with PID %d", pid);
    } else {
        LOGE("Failed to fork sandbox");
    }
}

JNIEXPORT jint JNICALL
Java_com_apex_root_ui_MainActivity_nativeGetOverallRisk(JNIEnv*, jobject) {
    auto report = apex::signing::signing_center::get_last_report();
    if (!report) return -1;
    return static_cast<jint>(report->overall_risk * 100);
}

} // extern "C"
