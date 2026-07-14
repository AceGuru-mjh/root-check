#include <jni.h>
#include <string>
#include <vector>
#include <android/log.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <sstream>

#define LOG_TAG "ApexRootNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

// 检查文件是否存在
bool fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

// 读取文件内容
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 检查常见 Root 管理应用
bool checkRootApps() {
    const std::vector<std::string> rootApps = {
        "/system/app/Superuser.apk",
        "/system/app/SuperSU.apk",
        "/system/app/magisk.apk",
        "/data/app/com.noshufou.android.su/",
        "/data/app/com.topjohnwu.magisk/",
        "/system/bin/su",
        "/system/xbin/su",
        "/sbin/su",
        "/data/local/xbin/su",
        "/data/local/bin/su"
    };
    
    for (const auto& app : rootApps) {
        if (fileExists(app)) {
            LOGI("Found root app: %s", app.c_str());
            return true;
        }
    }
    return false;
}

// 检查 SELinux 状态
bool checkSELinux() {
    std::string status = readFile("/sys/fs/selinux/enforce");
    if (status.find("0") != std::string::npos) {
        LOGI("SELinux is permissive");
        return true;
    }
    return false;
}

// 检查危险应用包名
std::vector<std::string> getDangerousApps() {
    const std::vector<std::string> dangerousApps = {
        "com.noshufou.android.su",
        "com.topjohnwu.magisk",
        "eu.chainfire.supersu",
        "me.phh.superuser",
        "com.kingroot.kinguser",
        "com.baidubox.app"
    };
    return dangerousApps;
}

} // anonymous namespace

// JNI: 检测设备是否 Root
extern "C"
JNIEXPORT jpair JNICALL
Java_com_apex_root_data_native_NativeBridge_nativeIsDeviceRooted(JNIEnv* env, jobject thiz) {
    LOGI("Starting native root detection...");
    
    bool isRooted = false;
    float confidence = 0.0f;
    int detectionCount = 0;
    
    // 检测 1: 检查 Root 管理应用
    if (checkRootApps()) {
        isRooted = true;
        detectionCount++;
        LOGI("Detection 1: Root apps found");
    }
    
    // 检测 2: 检查 SELinux
    if (checkSELinux()) {
        isRooted = true;
        detectionCount++;
        LOGI("Detection 2: SELinux permissive");
    }
    
    // 检测 3: 检查 su 二进制
    const char* paths[] = {"/system/bin/su", "/system/xbin/su", "/sbin/su"};
    for (const auto& path : paths) {
        if (fileExists(path)) {
            isRooted = true;
            detectionCount++;
            LOGI("Detection 3: su binary found at %s", path);
            break;
        }
    }
    
    // 计算置信度
    if (detectionCount > 0) {
        confidence = std::min(1.0f, detectionCount * 0.35f);
    }
    
    LOGI("Root detection complete: rooted=%s, confidence=%.2f, detections=%d",
         isRooted ? "true" : "false", confidence, detectionCount);
    
    // 创建 Pair<Boolean, Float>
    jclass pairClass = env->FindClass("kotlin/Pair");
    jmethodID pairConstructor = env->GetMethodID(pairClass, "<init>", "(Ljava/lang/Object;Ljava/lang/Object;)V");
    
    jboolean rooted = isRooted ? JNI_TRUE : JNI_FALSE;
    jfloat conf = confidence;
    
    jobject boxedBoolean = env->CallStaticObjectMethod(
        env->FindClass("java/lang/Boolean"),
        env->GetStaticMethodID(env->FindClass("java/lang/Boolean"), "valueOf", "(Z)Ljava/lang/Boolean;"),
        rooted
    );
    
    jobject boxedFloat = env->CallStaticObjectMethod(
        env->FindClass("java/lang/Float"),
        env->GetStaticMethodID(env->FindClass("java/lang/Float"), "valueOf", "(F)Ljava/lang/Float;"),
        conf
    );
    
    return (jpair)env->NewObject(pairClass, pairConstructor, boxedBoolean, boxedFloat);
}

// JNI: 检测 Busybox
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_native_NativeBridge_nativeHasBusybox(JNIEnv* env, jobject thiz) {
    LOGI("Checking for Busybox...");
    
    const char* paths[] = {
        "/system/xbin/busybox",
        "/system/bin/busybox",
        "/data/local/xbin/busybox",
        "/sbin/busybox"
    };
    
    for (const auto& path : paths) {
        if (fileExists(path)) {
            LOGI("Busybox found at %s", path);
            return JNI_TRUE;
        }
    }
    
    LOGI("Busybox not found");
    return JNI_FALSE;
}

// JNI: 检测危险应用
extern "C"
JNIEXPORT jobjectArray JNICALL
Java_com_apex_root_data_native_NativeBridge_nativeDetectDangerousApps(JNIEnv* env, jobject thiz) {
    LOGI("Detecting dangerous apps...");
    
    std::vector<std::string> apps = getDangerousApps();
    
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray result = env->NewObjectArray(apps.size(), stringClass, nullptr);
    
    for (size_t i = 0; i < apps.size(); ++i) {
        jstring str = env->NewStringUTF(apps[i].c_str());
        env->SetObjectArrayElement(result, i, str);
        env->DeleteLocalRef(str);
    }
    
    LOGI("Found %zu dangerous apps", apps.size());
    return result;
}

// JNI: 获取系统属性
extern "C"
JNIEXPORT jstring JNICALL
Java_com_apex_root_data_native_NativeBridge_nativeGetSystemProperty(JNIEnv* env, jobject thiz, jstring propName) {
    const char* propNameStr = env->GetStringUTFChars(propName, nullptr);
    LOGI("Getting system property: %s", propNameStr);
    
    std::string path = "/system/build.prop";
    std::string content = readFile(path);
    std::string value = "";
    
    std::istringstream stream(content);
    std::string line;
    std::string propPrefix = std::string(propNameStr) + "=";
    
    while (std::getline(stream, line)) {
        if (line.find(propPrefix) == 0) {
            value = line.substr(propPrefix.length());
            break;
        }
    }
    
    env->ReleaseStringUTFChars(propName, propNameStr);
    
    if (value.empty()) {
        LOGI("Property %s not found", propNameStr);
        return nullptr;
    }
    
    LOGI("Property %s = %s", propNameStr, value.c_str());
    return env->NewStringUTF(value.c_str());
}
