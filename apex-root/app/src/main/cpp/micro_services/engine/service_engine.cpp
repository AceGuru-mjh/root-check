#include "service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include "dirent_compat.h"
#include <cstring>
#include <memory>
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
// FIX-P1-CPP (v1.1.2): g_plugins 改为 shared_ptr 数组, 解决 execute_scan 持锁外
// 调用 plugin->execute() 的 UAF 风险 (P1-C4)。execute_scan 在持锁期间拷贝需要的
// shared_ptr 到局部 vector, 释放锁后遍历局部 vector — 即使另一线程调用
// unregister_service 重置槽位, 局部 shared_ptr 仍延长插件生命周期。
static std::shared_ptr<ServicePlugin> g_plugins[MAX_SERVICES];
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

// P1-1 修复: 不再硬编码路径,改为运行时可设置。默认 "plugins" 相对路径
// (会被 dlopen 解析为进程当前工作目录下的 plugins/,通常无效,但不崩溃)
// 实际路径由 Kotlin 通过 set_plugins_dir() 在 initialize() 前设置。
static char g_plugins_dir[512] = "plugins";
static bool g_plugins_dir_set = false;

void set_plugins_dir(const char* path) {
    if (!path) return;
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    strncpy(g_plugins_dir, path, sizeof(g_plugins_dir) - 1);
    g_plugins_dir[sizeof(g_plugins_dir) - 1] = '\0';
    g_plugins_dir_set = true;
    LOGI("Plugins dir set to: %s", g_plugins_dir);
}

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
    // P1-1 修复: 使用运行时设置的 g_plugins_dir 而非硬编码 PLUGINS_DIR
    int fd = static_cast<int>(bs_openat(-100, g_plugins_dir, 0x10000, 0));
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
                        // P1-1 修复: 使用 g_plugins_dir 构建完整路径
                        char full_path[512];
                        int idx = 0;
                        for (int i = 0; g_plugins_dir[i] && idx < (int)sizeof(full_path) - 1; i++) {
                            full_path[idx++] = g_plugins_dir[i];
                        }
                        if (idx < (int)sizeof(full_path) - 1) full_path[idx++] = '/';
                        for (size_t i = 0; i < name_len && idx < (int)sizeof(full_path) - 1; i++) {
                            full_path[idx++] = dirent->d_name[i];
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
    // FIX-P1-CPP (v1.1.2): 用 shared_ptr 托管插件生命周期, execute_scan 可安全
    // 在锁外拷贝 shared_ptr 延长插件存活。
    g_plugins[g_plugin_count] = std::make_shared<ServicePlugin>(plugin);
    g_plugin_loaded[g_plugin_count] = true;
    g_plugin_count++;
    return true;
}

bool unregister_service(uint32_t service_id) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    for (int i = 0; i < g_plugin_count; i++) {
        if (g_plugins[i] && g_plugins[i]->id == service_id) {
            // FIX-P1-CPP: cleanup 在持锁状态下调用 (与原行为一致), 但在 reset()
            // 之前调用。若有其他线程正在 execute_scan 中持有同一 shared_ptr 的
            // 局部拷贝, 该拷贝仍存活, UAF 不会发生 (cleanup 不释放插件内存, 只是
            // 调用插件自定义析构逻辑 — 由插件作者保证 cleanup 不破坏 *this)。
            if (g_plugins[i]->cleanup) g_plugins[i]->cleanup();
            g_plugins[i].reset();
            g_plugin_loaded[i] = false;
            return true;
        }
    }
    return false;
}

std::optional<ServicePlugin> get_service(uint32_t service_id) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    for (int i = 0; i < g_plugin_count; i++) {
        if (g_plugins[i] && g_plugins[i]->id == service_id && g_plugin_loaded[i]) {
            return *g_plugins[i];
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

    // FIX-P1-CPP (v1.1.2): 持锁期间拷贝需要的 shared_ptr 到局部 vector, 释放锁后
    // 遍历局部 vector — 即使另一线程调用 unregister_service, 局部 shared_ptr 仍
    // 延长插件生命周期, 避免 UAF (原代码取裸指针 &g_plugins[idx] 在锁外调用
    // plugin->execute() 是 UAF 风险)。
    std::vector<std::shared_ptr<ServicePlugin>> selected;
    selected.reserve(MAX_SERVICES);
    {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        for (int i = 0; i < g_plugin_count; i++) {
            if (g_plugin_loaded[i] && g_plugins[i]) {
                selected.push_back(g_plugins[i]);
            }
        }
    }
    int count = (int)selected.size();
    // indices[k] = k, 后续 shuffle_services 会打乱顺序
    int indices[MAX_SERVICES];
    for (int k = 0; k < count; k++) indices[k] = k;

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
        // FIX-P1-CPP: 通过 shared_ptr 访问插件, 局部拷贝保证插件存活到本次 execute 结束。
        auto& plugin = selected[idx];
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
        if (g_plugin_loaded[i] && g_plugins[i] && g_plugins[i]->cleanup) {
            g_plugins[i]->cleanup();
        }
        g_plugins[i].reset();
    }
    g_plugin_count = 0;
    g_initialized = false;
}

} // namespace service_engine
} // namespace engine
} // namespace apex
