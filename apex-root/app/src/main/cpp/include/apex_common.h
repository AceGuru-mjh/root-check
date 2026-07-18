#ifndef APEX_ROOT_COMMON_H
#define APEX_ROOT_COMMON_H

#include <cstdint>
#include <cstddef>

namespace apex {

// Version info — P0-B1 修复: 统一到 1.1.1 (与 build.gradle.kts / module.prop 同步)
// 注意: C++ 层无法直接读 BuildConfig,这里手动同步。
// 未来可通过 CMake -DAPEX_VERSION 注入。
constexpr uint32_t VERSION_MAJOR = 1;
constexpr uint32_t VERSION_MINOR = 1;
constexpr uint32_t VERSION_PATCH = 1;
constexpr const char* VERSION_STRING = "1.1.1";
constexpr const char* BUILD_NAME = "APEX-Root Omnipotent";

// Protocol constants
constexpr size_t MAX_MSG_SIZE = 65536;
constexpr size_t HASH_SIZE = 64;
constexpr size_t SIG_SIZE = 2700; // Dilithium 3 signature

// IPC message types
enum class IpcMessage : uint8_t {
    HEARTBEAT = 0x01,
    SCAN_TASK = 0x02,
    LAYER_RESULT = 0x03,
    REPORT = 0x04,
    ALERT = 0x05,
    PROGRESS = 0x06,
    VOTE = 0x07,
    CONSENSUS = 0x08,
    UPDATE_RULES = 0x09,
    SHUTDOWN = 0xFF
};

// Error codes
enum class ErrorCode : int32_t {
    SUCCESS = 0,
    INIT_FAILED = -1,
    PERMISSION_DENIED = -2,
    IPC_ERROR = -3,
    CRYPTO_ERROR = -4,
    SERVICE_NOT_FOUND = -5,
    SANDBOX_ERROR = -6,
    TIMEOUT = -7
};

// v1.1.1 修复 P0-C5: 架构支持检测
// ─────────────────────────────────────────────────────────────
// detect/ 层的 inline asm (svc #0) 仅在 __aarch64__ 下实现,
// armeabi-v7a / x86_64 APK 上所有 syscall 返回 -1, 检测函数返回 false,
// 导致用户得到"设备安全"的错误结论。
//
// 本宏供 native-lib.cpp 在 runQuickScan / isDeviceRooted 入口处检测,
// 非 aarch64 时在结果中明确标注"架构不支持",而非静默返回 false。
// ─────────────────────────────────────────────────────────────
#if defined(__aarch64__)
    constexpr bool ARCH_SUPPORTED = true;
    constexpr const char* ARCH_NAME = "arm64-v8a (aarch64)";
#else
    constexpr bool ARCH_SUPPORTED = false;
    constexpr const char* ARCH_NAME =
        #if defined(__arm__)
            "armeabi-v7a (arm32) — syscall 检测不支持";
        #elif defined(__x86_64__)
            "x86_64 — syscall 检测不支持";
        #else
            "unknown — syscall 检测不支持";
        #endif
#endif

} // namespace apex

#endif // APEX_ROOT_COMMON_H
