#include <jni.h>
#include <android/log.h>
#include <cstring>
#include "ctrl/apex_firewall_ctrl.h"

#define LOG_TAG "APEX-JNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

static ApexFirewall *g_fw = nullptr;
static JavaVM *g_jvm = nullptr;

static jclass g_callback_cls = nullptr;
static jobject g_callback_obj = nullptr;

static void ensure_fw(uint32_t uid) {
    if (!g_fw) {
        g_fw = new ApexFirewall(uid);
    }
}

extern "C" JNIEXPORT jint JNICALL
Java_com_apex_root_data_jni_NativeBridge_enableHideMode(
    JNIEnv *env, jobject thiz, jint apex_uid) {
    ensure_fw((uint32_t)apex_uid);
    int ret = g_fw->switch_mode(ApexFirewall::MODE_HIDE);
    LOGD("enableHideMode -> %d", ret);
    return ret;
}

extern "C" JNIEXPORT void JNICALL
Java_com_apex_root_data_jni_NativeBridge_disableHideMode(
    JNIEnv *env, jobject thiz) {
    if (g_fw) {
        g_fw->switch_mode(ApexFirewall::MODE_DETECT);
    }
    LOGD("disableHideMode");
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_isHideModeActive(
    JNIEnv *env, jobject thiz) {
    if (!g_fw) return false;
    return g_fw->is_running();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_apex_root_data_jni_NativeBridge_enableGameMode(
    JNIEnv *env, jobject thiz, jint apex_uid) {
    ensure_fw((uint32_t)apex_uid);
    int ret = g_fw->switch_mode(ApexFirewall::MODE_GAME);
    LOGD("enableGameMode -> %d", ret);
    return ret;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_apex_root_data_jni_NativeBridge_getCurrentMode(
    JNIEnv *env, jobject thiz) {
    if (!g_fw) return 0;
    return (jint)g_fw->current_mode();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_getLastError(
    JNIEnv *env, jobject thiz) {
    if (!g_fw) return env->NewStringUTF("not initialized");
    return env->NewStringUTF(g_fw->last_error().c_str());
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    g_jvm = vm;
    LOGD("APEX JNI loaded");
    return JNI_VERSION_1_6;
}
