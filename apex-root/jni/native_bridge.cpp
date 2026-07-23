#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <mutex>
// 修复：原 #include "ctrl/apex_firewall_ctrl.h" 假设从仓库根 include，
// 但 CMakeLists.txt 已把 ctrl/ 加入 include path，改为直接引用。
#include "apex_firewall_ctrl.h"

#define LOG_TAG "APEX-JNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static std::mutex g_fw_mutex;
static ApexFirewall *g_fw = nullptr;

static void ensure_fw(uint32_t uid) {
    std::lock_guard<std::mutex> lock(g_fw_mutex);
    if (!g_fw) {
        g_fw = new ApexFirewall(uid);
    }
}

static void destroy_fw() {
    std::lock_guard<std::mutex> lock(g_fw_mutex);
    if (g_fw) {
        g_fw->switch_mode(ApexFirewall::MODE_DETECT);
        delete g_fw;
        g_fw = nullptr;
    }
}

extern "C" JNIEXPORT jint JNICALL
Java_com_apex_root_data_jni_NativeBridge_enableHideModeNative(
    JNIEnv *env, jobject thiz, jint apex_uid) {
    ensure_fw((uint32_t)apex_uid);
    std::lock_guard<std::mutex> lock(g_fw_mutex);
    if (!g_fw) return -1;
    int ret = g_fw->switch_mode(ApexFirewall::MODE_HIDE);
    LOGD("enableHideMode -> %d", ret);
    return ret;
}

extern "C" JNIEXPORT void JNICALL
Java_com_apex_root_data_jni_NativeBridge_disableHideModeNative(
    JNIEnv *env, jobject thiz) {
    (void)env;
    (void)thiz;
    // P3-2(3a): disable 时立即释放 g_fw (BPF fd / perf event fd 等内核资源),
    // 不等 JNI_OnUnload (后者仅在 ClassLoader GC 时触发, 对主进程等同进程退出)。
    // destroy_fw() 内部已加 g_fw_mutex 锁并执行 switch_mode(MODE_DETECT) + delete,
    // 故此处不再重复持锁 (避免重入死锁)。
    destroy_fw();
    LOGD("disableHideMode (g_fw released)");
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_isHideModeActiveNative(
    JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(g_fw_mutex);
    if (!g_fw) return false;
    return g_fw->is_running();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_apex_root_data_jni_NativeBridge_enableGameModeNative(
    JNIEnv *env, jobject thiz, jint apex_uid) {
    ensure_fw((uint32_t)apex_uid);
    std::lock_guard<std::mutex> lock(g_fw_mutex);
    if (!g_fw) return -1;
    int ret = g_fw->switch_mode(ApexFirewall::MODE_GAME);
    LOGD("enableGameMode -> %d", ret);
    return ret;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_apex_root_data_jni_NativeBridge_getCurrentModeNative(
    JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(g_fw_mutex);
    if (!g_fw) return 0;
    return (jint)g_fw->current_mode();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_getLastErrorNative(
    JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(g_fw_mutex);
    if (!g_fw) return env->NewStringUTF("not initialized");
    return env->NewStringUTF(g_fw->last_error().c_str());
}

// FIX-P2-CPP (v1.1.3): 新增 JNI_OnUnload 在库卸载时调用 destroy_fw() 释放 g_fw。
// 原实现: ensure_fw 用 `new ApexFirewall(uid)` 创建 g_fw, 但 destroy_fw 是 static
// 函数从未被任何 JNI 方法调用, g_fw 在进程退出前永不释放。若 ApexFirewall 持有
// 内核资源 (perf event fd / eBPF prog fd), 不显式 close 可能泄漏到内核。
//
// 注意: JNI_OnUnload 仅在加载该 .so 的 ClassLoader 被 GC 时调用, 对 Android
// 主进程而言通常等同于进程退出。更可靠的清理路径是在 Application.onTerminate
// 或 ViewModel.onCleared 中调一个 destroyFirewallNative JNI 方法 + Kotlin 侧
// 调用 — 本任务为最小改动只加 JNI_OnUnload, Kotlin 侧显式清理留作后续。
extern "C" JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM *vm, void *reserved) {
    (void)vm;
    (void)reserved;
    destroy_fw();
}

