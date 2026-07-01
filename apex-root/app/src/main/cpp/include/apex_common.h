#ifndef APEX_ROOT_COMMON_H
#define APEX_ROOT_COMMON_H

#include <cstdint>
#include <cstddef>

namespace apex {

// Version info
constexpr uint32_t VERSION_MAJOR = 3;
constexpr uint32_t VERSION_MINOR = 1;
constexpr uint32_t VERSION_PATCH = 0;
constexpr const char* VERSION_STRING = "1.0.2";
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

} // namespace apex

#endif // APEX_ROOT_COMMON_H
