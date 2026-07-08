// ═══════════════════════════════════════════════════════════
//  Stub JNI / Android headers for host-side fuzz testing.
// ----------------------------------------------------------------
//  Provides minimal declarations so the parsers under test compile
//  without the Android NDK. Real JNI calls are never invoked from
//  the fuzz harness — we exercise the C++ parsing logic only.
// ═══════════════════════════════════════════════════════════

#ifndef APEX_FUZZ_STUB_JNI_H
#define APEX_FUZZ_STUB_JNI_H

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>

// ─── Stub JNI types ───
typedef int32_t jint;
typedef int64_t jlong;
typedef int8_t jbyte;
typedef uint8_t jboolean;
typedef void* jobject;
typedef jobject jstring;
typedef jobject jbyteArray;
typedef jint jsize;

struct JNIEnv_ {
    jstring (*NewStringUTF)(const char*);
    jbyte* (*GetByteArrayElements)(jbyteArray, jboolean*);
    void (*ReleaseByteArrayElements)(jbyteArray, jbyte*, jint);
    jsize (*GetArrayLength)(jbyteArray);
    jbyteArray (*NewByteArray)(jsize);
    void (*SetByteArrayRegion)(jbyteArray, jsize, jsize, const jbyte*);
    const char* (*GetStringUTFChars)(jstring, jboolean*);
    void (*ReleaseStringUTFChars)(jstring, const char*);
};
typedef JNIEnv_ JNIEnv;
struct JavaVM {};

#define JNI_VERSION_1_6 6
#define JNIEXPORT
#define JNICALL
#define JNI_ABORT 2

// ─── Stub Android log ───
#define ANDROID_LOG_INFO 4
#define ANDROID_LOG_WARN 5
#define ANDROID_LOG_ERROR 6
static inline int __android_log_print(int, const char*, const char*, ...) { return 0; }

// ─── Stub system properties ───
static inline int __system_property_get(const char*, char*) { return 0; }

#endif // APEX_FUZZ_STUB_JNI_H
