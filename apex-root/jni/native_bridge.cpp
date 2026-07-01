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
Java_com_apex_root_data_jni_NativeBridge_enableHideMode(
    JNIEnv *env, jobject thiz, jint apex_uid) {
    ensure_fw((uint32_t)apex_uid);
    std::lock_guard<std::mutex> lock(g_fw_mutex);
    if (!g_fw) return -1;
    int ret = g_fw->switch_mode(ApexFirewall::MODE_HIDE);
    LOGD("enableHideMode -> %d", ret);
    return ret;
}

extern "C" JNIEXPORT void JNICALL
Java_com_apex_root_data_jni_NativeBridge_disableHideMode(
    JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(g_fw_mutex);
    if (g_fw) {
        g_fw->switch_mode(ApexFirewall::MODE_DETECT);
    }
    LOGD("disableHideMode");
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_isHideModeActive(
    JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(g_fw_mutex);
    if (!g_fw) return false;
    return g_fw->is_running();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_apex_root_data_jni_NativeBridge_enableGameMode(
    JNIEnv *env, jobject thiz, jint apex_uid) {
    ensure_fw((uint32_t)apex_uid);
    std::lock_guard<std::mutex> lock(g_fw_mutex);
    if (!g_fw) return -1;
    int ret = g_fw->switch_mode(ApexFirewall::MODE_GAME);
    LOGD("enableGameMode -> %d", ret);
    return ret;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_apex_root_data_jni_NativeBridge_getCurrentMode(
    JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(g_fw_mutex);
    if (!g_fw) return 0;
    return (jint)g_fw->current_mode();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_getLastError(
    JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(g_fw_mutex);
    if (!g_fw) return env->NewStringUTF("not initialized");
    return env->NewStringUTF(g_fw->last_error().c_str());
}

}

