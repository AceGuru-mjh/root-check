#include "apex_firewall_ctrl.h"
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cinttypes>
#include <sys/types.h>
#include <unistd.h>
#include <android/log.h>

#define LOG_TAG "APEX-FW"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#ifdef __ANDROID__
#include <dlfcn.h>
#include <linux/bpf.h>
#include <sys/syscall.h>
#endif

bool CleanBaseline::load_from_file(const std::string &path) {
    FILE *fp = fopen(path.c_str(), "rb");
    if (!fp) return false;
    bool ok = fread(this, sizeof(*this), 1, fp) == 1;
    fclose(fp);
    return ok;
}

bool CleanBaseline::save_to_file(const std::string &path) {
    FILE *fp = fopen(path.c_str(), "wb");
    if (!fp) return false;
    bool ok = fwrite(this, sizeof(*this), 1, fp) == 1;
    fclose(fp);
    return ok;
}

static int bpf(enum bpf_cmd cmd, union bpf_attr *attr, unsigned int size) {
#ifdef __NR_bpf
    return syscall(__NR_bpf, cmd, attr, size);
#else
    errno = ENOSYS;
    return -1;
#endif
}

struct ApexFirewall::ApexFirewallImpl {
    int bpf_fd = -1;
    int link_fds[32];
    int link_count = 0;
    CleanBaseline baseline;
    bool loaded = false;

    ~ApexFirewallImpl() {
        for (int i = 0; i < link_count; i++) {
            if (link_fds[i] >= 0) close(link_fds[i]);
        }
        if (bpf_fd >= 0) close(bpf_fd);
    }
};

ApexFirewall::ApexFirewall(uint32_t apex_uid)
    : impl_(new ApexFirewallImpl()), apex_uid_(apex_uid), current_mode_(MODE_DETECT) {}

ApexFirewall::~ApexFirewall() {
    unload();
    delete impl_;
}

int ApexFirewall::collect_baseline(CleanBaseline &baseline) {
    memset(&baseline, 0, sizeof(baseline));
    FILE *fp;

    fp = fopen("/proc/version", "r");
    if (fp) {
        fgets(baseline.kernel_ver, sizeof(baseline.kernel_ver), fp);
        fclose(fp);
    }
    fp = fopen("/proc/cmdline", "r");
    if (fp) {
        fgets(baseline.cmdline, sizeof(baseline.cmdline), fp);
        fclose(fp);
    }
    fp = fopen("/system/build.prop", "r");
    if (fp) {
        size_t n = fread(baseline.build_prop, 1, sizeof(baseline.build_prop) - 1, fp);
        baseline.build_prop[n] = '\0';
        fclose(fp);
    }
    return 0;
}

int ApexFirewall::load() {
    CleanBaseline bl;
    collect_baseline(bl);
    return load_with_baseline(bl);
}

int ApexFirewall::load_with_baseline(const CleanBaseline &baseline) {
    impl_->baseline = baseline;

#ifdef __ANDROID__
    if (!access("/sys/fs/bpf", F_OK)) {
        LOGD("eBPF sysfs detected, using eBPF path");

        union bpf_attr prog_attr = {};
        prog_attr.prog_type = BPF_PROG_TYPE_TRACEPOINT;
        prog_attr.insn_cnt = 0;
        prog_attr.license = (__u64)"GPL";

        impl_->bpf_fd = bpf(BPF_PROG_LOAD, &prog_attr, sizeof(prog_attr));
        if (impl_->bpf_fd < 0) {
            LOGE("BPF prog load failed, fallback to legacy: %s", strerror(errno));
            return -1;
        }
        impl_->loaded = true;
        LOGD("eBPF firewall loaded successfully for UID %u", apex_uid_);
        return 0;
    }
#endif

    LOGE("No eBPF support detected, need legacy fallback");
    return -1;
}

void ApexFirewall::unload() {
    impl_->loaded = false;
    LOGD("Firewall unloaded");
}

bool ApexFirewall::is_running() const {
    return impl_->loaded;
}

int ApexFirewall::switch_mode(Mode mode) {
    bool was_running = is_running();
    if (was_running) unload();

    current_mode_ = mode;

    switch (mode) {
    case MODE_DETECT:
        LOGD("Switched to DETECT mode (no hiding)");
        return 0;
    case MODE_HIDE:
        LOGD("Switched to HIDE mode");
        return load();
    case MODE_GAME:
        LOGD("Switched to GAME mode (aggressive)");
        return load();
    }
    return 0;
}

ApexFirewall::Mode ApexFirewall::current_mode() const {
    return current_mode_;
}
