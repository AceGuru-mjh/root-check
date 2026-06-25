#include <jni.h>
#include <android/log.h>
#include "detector/property_detector.h"
#include "detector/file_detector.h"
#include "detector/memory_detector.h"
#include "detector/mount_detector.h"
#include "detector/apatch_detector.h"
#include "detector/selinux_detector.h"
#include "detector/selinux_jump_detector.h"
#include "detector/kernel_detector.h"
#include "detector/kernel_module_detector.h"
#include "detector/self_protection.h"
#include "detector/process_detector.h"
#include "detector/syscall_timing_detector.h"
#include "detector/interrupt_timing_detector.h"
#include "detector/cache_timing_detector.h"
#include "detector/binder_latency_detector.h"
#include "detector/pagefault_monitor.h"
#include "detector/firmware_detector.h"
#include "detector/shamiko_detector.h"
#include "detector/zygisknext_detector.h"
#include "detector/apatch_kpm_detector.h"
#include "detector/kernelsu_overlay_detector.h"
#include "detector/lsposed_detector.h"

#define LOG_TAG "RootDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

extern "C" {

// ==================== 第 1 层：应用层属性检测 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detector_PropertyDetector_nativeDetectProperties(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");
    
    auto result = PropertyDetector::detect();
    
    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 第 4 层：文件系统检测 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detector_FileDetector_nativeDetectFiles(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");
    
    auto result = FileDetector::detect();
    
    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 第 3 层：内存扫描检测 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detector_MemoryDetector_nativeDetectMemory(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");
    
    auto result = MemoryDetector::detect();
    
    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 第 4 层增强：挂载命名空间对比 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detector_MountDetector_nativeDetectMountNamespace(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");
    
    auto result = MountDetector::detectNamespaceManipulation();
    
    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 第 5 层：APatch 侧信道检测 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detector_APatchDetector_nativeDetectAPatch(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");
    
    auto result = APatchDetector::detect();
    
    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 第 5 层：SELinux 上下文检测 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detector_SelinuxDetector_nativeDetectSelinux(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");
    
    auto result = SelinuxDetector::detect();
    
    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 第 6 层：内核模块检测 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detector_KernelDetector_nativeDetectKernel(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");
    
    auto result = KernelDetector::detect();
    
    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 第 6 层增强：内核模块深度检测 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detector_KernelModuleDetector_nativeDetectKernelModules(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");
    
    auto result = KernelModuleDetector::detect();
    
    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 第 5 层：系统调用时间分析检测 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detector_SyscallTimingDetector_nativeDetectSyscallTiming(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");
    
    auto result = SyscallTimingDetector::detect();
    
    jobject jResult = env->NewObject(resultClass, constructor,
        result.overall_result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.overall_result.layer.c_str()),
        env->NewStringUTF(result.overall_result.detail.c_str()));
    return jResult;
}

// ==================== 自保护检测 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detector_SelfProtection_nativeDetectSelfProtection(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");
    
    auto result = SelfProtection::detect();
    
    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 第 5 层增强：SELinux 上下文跳变检测 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detector_SelinuxJumpDetector_nativeDetectSelinuxJump(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");

    auto result = SelinuxJumpDetector::detect();

    jobject jResult = env->NewObject(resultClass, constructor,
        result.overall_result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.overall_result.layer.c_str()),
        env->NewStringUTF(result.overall_result.detail.c_str()));
    return jResult;
}

// 在指定阶段记录 SELinux 上下文
JNIEXPORT jstring JNICALL
Java_com_rootdetector_detector_SelinuxJumpDetector_nativeRecordContext(JNIEnv *env, jobject thiz, jint phase) {
    DetectionPhase detectionPhase = static_cast<DetectionPhase>(phase);
    auto snapshot = SelinuxJumpDetector::recordContext(detectionPhase);

    // 构建返回的 JSON 字符串
    std::string json = "{";
    json += "\"phase\":\"" + SelinuxJumpDetector::phaseToString(detectionPhase) + "\",";
    json += "\"context\":\"" + snapshot.context + "\",";
    json += "\"timestamp\":" + std::to_string(snapshot.timestamp_ms) + ",";
    json += "\"is_abnormal\":" + std::string(snapshot.is_abnormal ? "true" : "false") + ",";
    json += "\"reason\":\"" + snapshot.abnormal_reason + "\"";
    json += "}";

    return env->NewStringUTF(json.c_str());
}

// 重置检测状态
JNIEXPORT void JNICALL
Java_com_rootdetector_detector_SelinuxJumpDetector_nativeReset(JNIEnv *env, jobject thiz) {
    SelinuxJumpDetector::reset();
}

// 获取上下文历史
JNIEXPORT jstring JNICALL
Java_com_rootdetector_detector_SelinuxJumpDetector_nativeGetContextHistory(JNIEnv *env, jobject thiz) {
    auto history = SelinuxJumpDetector::getContextHistory();

    std::string json = "[";
    for (size_t i = 0; i < history.size(); ++i) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"phase\":\"" + SelinuxJumpDetector::phaseToString(history[i].phase) + "\",";
        json += "\"context\":\"" + history[i].context + "\",";
        json += "\"timestamp\":" + std::to_string(history[i].timestamp_ms) + ",";
        json += "\"is_abnormal\":" + std::string(history[i].is_abnormal ? "true" : "false");
        json += "}";
    }
    json += "]";

    return env->NewStringUTF(json.c_str());
}

// ==================== 第 5 层增强：进程与线程深度检测 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detect_DetectionEngine_nativeDetectProcesses(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");
    
    auto result = ProcessDetector::detect();
    
    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 第 7 层(TEE)：固件/分区完整性校验 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detect_DetectionEngine_nativeDetectFirmware(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");

    auto result = FirmwareDetector::detect();

    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 旁路侧信道：中断调度侧信道分析 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detector_InterruptTimingDetector_nativeDetectInterruptTiming(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");

    auto result = InterruptTimingDetector::detect();

    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 旁路侧信道：Cache 时序侧信道分析 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detector_CacheTimingDetector_nativeDetectCacheTiming(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");

    auto result = CacheTimingDetector::detect();

    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 旁路侧信道：Binder 调用时延分析 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detector_BinderLatencyDetector_nativeDetectBinderLatency(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");

    auto result = BinderLatencyDetector::detect();

    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 旁路侧信道：PageFault 缺页异常监控 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detector_PagefaultMonitor_nativeDetectPagefault(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");

    auto result = PagefaultMonitor::detect();

    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 对抗隐藏：Shamiko 行为特征专项探针 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detect_DetectionEngine_nativeDetectShamiko(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");

    auto result = ShamikoDetector::detect();

    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 对抗隐藏：ZygiskNext 隔离层检测探针 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detect_DetectionEngine_nativeDetectZygiskNext(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");

    auto result = ZygiskNextDetector::detect();

    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 对抗隐藏：APatch KPM 内核补丁探针 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detect_DetectionEngine_nativeDetectApatchKpm(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");

    auto result = ApatchKpmDetector::detect();

    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 对抗隐藏：KernelSU overlayfs 伪装探针 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detect_DetectionEngine_nativeDetectKernelSUOverlay(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");

    auto result = KernelSUOverlayDetector::detect();

    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

// ==================== 对抗隐藏：LSPosed Riru 双模式区分探针 ====================
JNIEXPORT jobject JNICALL
Java_com_rootdetector_detect_DetectionEngine_nativeDetectLSPosed(JNIEnv *env, jobject thiz) {
    jclass resultClass = env->FindClass("com/rootdetector/detect/DetectionResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZLjava/lang/String;Ljava/lang/String;)V");

    auto result = LSPosedDetector::detect();

    jobject jResult = env->NewObject(resultClass, constructor,
        result.detected ? JNI_TRUE : JNI_FALSE,
        env->NewStringUTF(result.layer.c_str()),
        env->NewStringUTF(result.detail.c_str()));
    return jResult;
}

} // extern "C"
