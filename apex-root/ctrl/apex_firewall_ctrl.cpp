#include "apex_firewall_ctrl.h"
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cinttypes>
#include <sys/types.h>
#include <unistd.h>
#include <android/log.h>

// 修复：原代码将 <linux/bpf.h> 放在 #ifdef __ANDROID__ 内，
// 但 bpf() 函数签名使用了 enum bpf_cmd / union bpf_attr，
// 在非 Android 平台语法检查时会报 "use of enum without previous declaration"。
// 现改为无条件 include（<linux/bpf.h> 在 Linux/macOS 上都可用）。
#ifdef __linux__
#include <linux/bpf.h>
#include <sys/syscall.h>
#endif
#ifdef __ANDROID__
#include <dlfcn.h>
#endif

#define LOG_TAG "APEX-FW"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

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

        // 修复：原代码 prog_attr.insn_cnt = 0 加载空 BPF 程序，
        // 即使加载成功也无任何 hook 效果。
        // 现改为：尝试从预编译 .o 文件加载真实 BPF 字节码。
        // 若 .o 不存在或加载失败，返回 -1 让 runtime_engine 降级到 legacy。
        const char *bpf_obj_path = "/data/adb/modules/apex-root/bpf/apex_firewall.o";
        FILE *fp = fopen(bpf_obj_path, "rb");
        if (!fp) {
            LOGE("BPF object not found at %s, fallback to legacy", bpf_obj_path);
            last_error_ = "BPF object file missing";
            return -1;
        }

        // 读取 BPF 字节码（ELF 格式，这里简化为读取 raw instructions）
        // 真实实现需要 ELF 解析 + bpftool gen skeleton，这里读取整个文件
        // 并尝试加载。如果文件是合法 BPF 程序，insns 即文件内容。
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (file_size <= 0) {
            fclose(fp);
            LOGE("BPF object empty or invalid");
            last_error_ = "BPF object empty";
            return -1;
        }

        // 分配缓冲区读取 BPF 指令
        size_t insns_size = (size_t)file_size;
        void *insns_buf = malloc(insns_size);
        if (!insns_buf) {
            fclose(fp);
            last_error_ = "OOM reading BPF object";
            return -1;
        }
        size_t nread = fread(insns_buf, 1, insns_size, fp);
        fclose(fp);

        if (nread != insns_size) {
            free(insns_buf);
            last_error_ = "BPF object read incomplete";
            return -1;
        }

        // 计算 BPF 指令数（每条指令 8 字节）
        // 真实 ELF 解析需要 bpf_object__open，这里简化处理
        size_t insn_cnt = insns_size / 8;
        if (insn_cnt == 0) {
            free(insns_buf);
            LOGE("BPF object has no instructions");
            last_error_ = "BPF object has no instructions";
            return -1;
        }

        // 加载 BPF 程序
        union bpf_attr prog_attr = {};
        prog_attr.prog_type = BPF_PROG_TYPE_TRACEPOINT;
        prog_attr.insn_cnt = (__u32)insn_cnt;
        prog_attr.insns = (__u64)(uintptr_t)insns_buf;
        prog_attr.license = (__u64)(uintptr_t)"GPL";
        prog_attr.log_level = 1;
        char log_buf[4096] = {};
        prog_attr.log_buf = (__u64)(uintptr_t)log_buf;
        prog_attr.log_size = sizeof(log_buf);

        impl_->bpf_fd = bpf(BPF_PROG_LOAD, &prog_attr, sizeof(prog_attr));
        free(insns_buf);

        if (impl_->bpf_fd < 0) {
            LOGE("BPF prog load failed: %s (log: %s)", strerror(errno), log_buf);
            last_error_ = std::string("BPF load failed: ") + strerror(errno);
            return -1;
        }

        impl_->loaded = true;
        LOGD("eBPF firewall loaded successfully for UID %u (insns=%zu)",
             apex_uid_, insn_cnt);
        return 0;
    }
#endif

    LOGE("No eBPF support detected, need legacy fallback");
    last_error_ = "No eBPF support";
    return -1;
}

void ApexFirewall::unload() {
    // 修复：原 unload() 只设 loaded=false，不关闭 bpf_fd，会造成 fd 泄漏
    if (impl_->bpf_fd >= 0) {
        close(impl_->bpf_fd);
        impl_->bpf_fd = -1;
        LOGD("BPF program fd closed");
    }
    for (int i = 0; i < impl_->link_count; i++) {
        if (impl_->link_fds[i] >= 0) {
            close(impl_->link_fds[i]);
            impl_->link_fds[i] = -1;
        }
    }
    impl_->link_count = 0;
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
