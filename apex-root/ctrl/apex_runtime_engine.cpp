#include "apex_firewall_ctrl.h"
#include "../legacy/apex_legacy_ctrl.h"
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/utsname.h>
#include <android/log.h>

#define LOG_TAG "APEX-ENGINE"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class ApexRuntimeEngine {
public:
    enum KernelLevel {
        KERNEL_UNKNOWN = 0,
        KERNEL_LEGACY = 1,    // < 5.10 (Android 10-11)
        KERNEL_EBPF = 2,      // >= 5.10 (Android 12+)
    };

private:
    ApexFirewall *fw_ = nullptr;
    ApexLegacyHide *legacy_ = nullptr;
    uint32_t apex_uid_;
    KernelLevel kernel_level_ = KERNEL_UNKNOWN;
    ApexFirewall::Mode current_mode_ = ApexFirewall::MODE_DETECT;

    KernelLevel detect_kernel_level() {
        struct utsname buf;
        if (uname(&buf) != 0) return KERNEL_LEGACY;

        int major = 0, minor = 0;
        sscanf(buf.release, "%d.%d", &major, &minor);

        LOGD("Kernel detected: %s (major=%d, minor=%d)", buf.release, major, minor);

        if (major > 5 || (major == 5 && minor >= 10)) {
            if (access("/sys/fs/bpf", F_OK) == 0) {
                LOGD("eBPF sysfs available, using eBPF path");
                return KERNEL_EBPF;
            }
        }

        if (access("/proc/self/ns/mnt", F_OK) == 0) {
            LOGD("Mount namespaces available, using legacy path");
            return KERNEL_LEGACY;
        }

        LOGD("Minimal support, using LD_PRELOAD hook path");
        return KERNEL_LEGACY;
    }

public:
    ApexRuntimeEngine(uint32_t apex_uid) : apex_uid_(apex_uid) {
        kernel_level_ = detect_kernel_level();
        LOGD("Engine initialized: UID=%u, kernel_level=%d", apex_uid, kernel_level_);
    }

    ~ApexRuntimeEngine() {
        disable();
    }

    int enable(ApexFirewall::Mode mode = ApexFirewall::MODE_HIDE) {
        disable();

        current_mode_ = mode;
        int ret = -1;

        if (kernel_level_ == KERNEL_EBPF) {
            LOGD("Enabling via eBPF firewall path");
            if (!fw_) fw_ = new ApexFirewall(apex_uid_);
            ret = fw_->switch_mode(mode);
            if (ret != 0) {
                LOGE("eBPF path failed, trying legacy fallback");
                delete fw_;
                fw_ = nullptr;
                kernel_level_ = KERNEL_LEGACY;
            }
        }

        if (kernel_level_ == KERNEL_LEGACY) {
            LOGD("Enabling via legacy mount-ns/LD_PRELOAD path");
            if (!legacy_) legacy_ = new ApexLegacyHide(apex_uid_);
            ret = legacy_->load();
        }

        if (ret == 0) {
            LOGD("Hide mode enabled successfully");
        } else {
            LOGE("All hide strategies failed");
        }

        return ret;
    }

    void disable() {
        if (fw_) {
            fw_->switch_mode(ApexFirewall::MODE_DETECT);
            delete fw_;
            fw_ = nullptr;
        }
        if (legacy_) {
            legacy_->unload();
            delete legacy_;
            legacy_ = nullptr;
        }
        current_mode_ = ApexFirewall::MODE_DETECT;
        LOGD("Hide mode disabled");
    }

    bool is_active() const {
        if (fw_ && fw_->is_running()) return true;
        if (legacy_ && legacy_->is_running()) return true;
        return false;
    }

    ApexFirewall::Mode current_mode() const { return current_mode_; }
    KernelLevel kernel_level() const { return kernel_level_; }
    const char *kernel_level_str() const {
        switch (kernel_level_) {
        case KERNEL_LEGACY: return "Legacy (mount-ns/LD_PRELOAD)";
        case KERNEL_EBPF: return "eBPF (kernel firewall)";
        default: return "Unknown";
        }
    }
};
