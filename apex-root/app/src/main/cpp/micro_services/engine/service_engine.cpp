#include "service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include "dirent_compat.h"
#include <cstring>
#include <mutex>
#include <random>
#include <dlfcn.h>
#include <android/log.h>

#define LOG_TAG "APEX-PluginEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace apex {
namespace engine {

static const int MAX_SERVICES = 256;
static ServicePlugin g_plugins[MAX_SERVICES];
static int g_plugin_count = 0;
static std::mutex g_engine_mutex;
static bool g_initialized = false;
static bool g_plugin_loaded[MAX_SERVICES] = {false};

// Inline shuffle
static void shuffle_services(int* indices, int count) {
    for (int i = count - 1; i > 0; i--) {
        uint64_t r = bs_get_random();
        int j = r % (i + 1);
        int tmp = indices[i];
        indices[i] = indices[j];
        indices[j] = tmp;
    }
}

namespace service_engine {

static const char* PLUGINS_DIR = "/data/app/com.apex.root/lib/arm64/plugins";

static bool load_native_plugin(const char* path) {
    void* handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        LOGE("Failed to load plugin %s: %s", path, dlerror());
        return false;
    }
    LOGI("Plugin loaded: %s", path);
    return true;
}

bool initialize() {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    g_initialized = true;
    g_plugin_count = 0;

    // Discover and load plugin .so files
    int fd = static_cast<int>(bs_openat(-100, PLUGINS_DIR, 0x10000, 0));
    if (fd >= 0) {
        char dentry_buf[8192];
        int64_t n = bs_getdents64(fd, dentry_buf, sizeof(dentry_buf));
        if (n > 0) {
            size_t pos = 0;
            while (pos + sizeof(struct apex_dirent64) <= static_cast<size_t>(n)) {
                auto* dirent = reinterpret_cast<struct apex_dirent64*>(dentry_buf + pos);
                // P0-1 修复: 校验 d_reclen 防止无限循环和越界读取
                if (dirent->d_reclen == 0 || dirent->d_reclen > static_cast<size_t>(n) - pos) {
                    break;
                }
                if (dirent->d_type == 8) {
                    size_t name_len = 0;
                    const char* name_end = (const char*)dirent + dirent->d_reclen;
                    while (dirent->d_name + name_len < name_end &&
                           dirent->d_name[name_len]) name_len++;
                    if (name_len > 3 &&
                        dirent->d_name[name_len-3] == '.' &&
                        dirent->d_name[name_len-2] == 's' &&
                        dirent->d_name[name_len-1] == 'o') {
                        char full_path[512];
                        int idx = 0;
                        for (int i = 0; PLUGINS_DIR[i]; i++) {
                            if (idx < (int)sizeof(full_path) - 1) full_path[idx++] = PLUGINS_DIR[i];
                        }
                        if (idx < (int)sizeof(full_path) - 1) full_path[idx++] = '/';
                        for (size_t i = 0; i < name_len; i++) {
                            if (idx < (int)sizeof(full_path) - 1) full_path[idx++] = dirent->d_name[i];
                        }
                        full_path[idx] = '\0';
                        load_native_plugin(full_path);
                    }
                }
                pos += dirent->d_reclen;
            }
        }
        bs_close(fd);
    }

    return true;
}

bool register_service(const ServicePlugin& plugin) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (g_plugin_count >= MAX_SERVICES) return false;
    g_plugins[g_plugin_count] = plugin;
    g_plugin_loaded[g_plugin_count] = true;
    g_plugin_count++;
    return true;
}

bool unregister_service(uint32_t service_id) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    for (int i = 0; i < g_plugin_count; i++) {
        if (g_plugins[i].id == service_id) {
            if (g_plugins[i].cleanup) g_plugins[i].cleanup();
            g_plugin_loaded[i] = false;
            return true;
        }
    }
    return false;
}

std::optional<ServicePlugin> get_service(uint32_t service_id) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    for (int i = 0; i < g_plugin_count; i++) {
        if (g_plugins[i].id == service_id && g_plugin_loaded[i]) {
            return g_plugins[i];
        }
    }
    return std::nullopt;
}

ScanReport execute_scan(const ScanConfig& config) {
    ScanReport report;
    report.total_duration_ns = 0;
    report.overall_risk = 0;
    report.services_executed = 0;
    report.services_failed = 0;

    uint64_t start_time = bs_clock_ns();

    // Build execution order
    int indices[MAX_SERVICES];
    int count = 0;
    {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        for (int i = 0; i < g_plugin_count; i++) {
            if (g_plugin_loaded[i]) {
                indices[count++] = i;
            }
        }
    }

    // Determine subset based on scan level
    int subset = count;
    switch (config.level) {
        case ScanLevel::QUICK:    subset = (count < 3) ? count : 3; break;
        case ScanLevel::STANDARD: subset = (count < 8) ? count : 8; break;
        case ScanLevel::DEEP:     subset = count; break;
        case ScanLevel::FORENSIC: subset = count; break;
    }

    // Shuffle if requested
    if (config.shuffle_services) {
        shuffle_services(indices, subset);
    }

    // Execute
    for (int i = 0; i < subset; i++) {
        int idx = indices[i];

        // CPU binding (optional)
        if (config.cpu_bind) {
            uint64_t cpu_mask = 1ULL << (bs_get_random() % 8);
            bs_sched_setaffinity(0, 8, &cpu_mask);
        }

        // Pre-execution noise
        if (config.noise_ns_max > 0) {
            uint64_t noise = config.noise_ns_min +
                (bs_get_random() % (config.noise_ns_max - config.noise_ns_min));
            bs_nanosleep(noise);
        }

        // Execute service
        ServiceResult result;
        ServicePlugin* plugin = &g_plugins[idx];
        if (plugin->init && !plugin->init()) {
            result.success = false;
            result.confidence = 0;
            result.description = "Init failed";
            report.services_failed++;
        } else {
            uint64_t svc_start = bs_clock_ns();
            result = plugin->execute(config);
            result.duration_ns = bs_clock_ns() - svc_start;
            if (result.success) {
                report.services_executed++;
            } else {
                report.services_failed++;
            }
        }
        result.service_id = plugin->id;
        result.service_name = plugin->name;

        report.results.push_back(result);

        // Update overall risk
        if (result.confidence > report.overall_risk && !result.success) {
            report.overall_risk = result.confidence;
        }
    }

    report.total_duration_ns = bs_clock_ns() - start_time;
    return report;
}

void shutdown() {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    for (int i = 0; i < g_plugin_count; i++) {
        if (g_plugin_loaded[i] && g_plugins[i].cleanup) {
            g_plugins[i].cleanup();
        }
    }
    g_plugin_count = 0;
    g_initialized = false;
}

} // namespace service_engine
} // namespace engine
} // namespace apex
