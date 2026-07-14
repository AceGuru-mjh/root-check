#ifndef APEX_ROOT_GUARD_ENGINE_H
#define APEX_ROOT_GUARD_ENGINE_H

#include <cstdint>

namespace apex {
namespace guard {

struct IntegrityEntry {
    const char* path;
    uint8_t hash[32]; // SHA-256
    bool is_directory;
};

enum class AlertLevel {
    INFO,
    WARNING,
    CRITICAL
};

struct SecurityAlert {
    AlertLevel level;
    char message[192];   // P0-2 修复: 固定大小数组,避免悬空指针
    char source[64];     // P0-2 修复: 固定大小数组,避免悬空指针
    uint64_t timestamp;
};

// Start monitoring in background
bool start_guardian();

// Stop monitoring
bool stop_guardian();

// Check system file integrity
bool check_system_integrity();

// Check for unauthorized kernel module loading
bool check_kernel_module_integrity();

// Check for unauthorized process starts
bool check_process_integrity(const char* whitelist[], int count);

// Active protection: prevent harmful operations
bool prevent_system_tamper();

// Get latest alerts
int get_alerts(SecurityAlert* buffer, int max_count);

// Initialize integrity database from stock hashes
bool init_integrity_database();

} // namespace guard
} // namespace apex

#endif
