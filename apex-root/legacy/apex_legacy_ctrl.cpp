#include "apex_legacy_ctrl.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <android/log.h>

#define LOG_TAG "APEX-LEGACY"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static const char *HIDE_PATHS[] = {
    "/sbin/.magisk",
    "/data/adb/magisk",
    "/data/adb/ksu",
    "/data/adb/ap",
    "/data/adb/lspd",
    "/data/adb/modules",
    nullptr,
};

struct ApexLegacyHide::Impl {
    pid_t daemon_pid = -1;
    bool ns_active = false;
};

ApexLegacyHide::ApexLegacyHide(uint32_t apex_uid)
    : impl_(new Impl()), apex_uid_(apex_uid), loaded_(false) {}

ApexLegacyHide::~ApexLegacyHide() {
    unload();
    delete impl_;
}

static bool is_safe_path(const char *path) {
    for (int i = 0; HIDE_PATHS[i]; i++) {
        if (strstr(path, HIDE_PATHS[i])) return false;
    }
    return true;
}

int ApexLegacyHide::mount_namespace_hide() {
    if (unshare(CLONE_NEWNS) != 0) {
        LOGE("unshare(CLONE_NEWNS) failed: %s", strerror(errno));
        return -1;
    }

    if (mount(NULL, "/", NULL, MS_PRIVATE | MS_REC, NULL) != 0) {
        LOGE("mount --make-private failed: %s", strerror(errno));
        return -1;
    }

    for (int i = 0; HIDE_PATHS[i]; i++) {
        struct stat st;
        if (stat(HIDE_PATHS[i], &st) == 0) {
            char tmpdir[256];
            snprintf(tmpdir, sizeof(tmpdir), "/apex_hidden_%d", i);

            mkdir(tmpdir, 0000);
            if (mount(tmpdir, HIDE_PATHS[i], NULL, MS_BIND, NULL) == 0) {
                LOGD("Bind-mounted empty dir over %s", HIDE_PATHS[i]);
            } else {
                LOGE("mount bind over %s failed: %s", HIDE_PATHS[i], strerror(errno));
            }
        }
    }

    impl_->ns_active = true;
    return 0;
}

int ApexLegacyHide::fork_hide_daemon() {
    pid_t pid = fork();
    if (pid < 0) {
        LOGE("fork failed: %s", strerror(errno));
        return -1;
    }

    if (pid == 0) {
        setsid();

        if (mount_namespace_hide() != 0) {
            _exit(1);
        }

        impl_->daemon_pid = getpid();
        LOGD("Hide daemon started PID=%d", getpid());

        while (true) {
            pause();
        }
        _exit(0);
    }

    impl_->daemon_pid = pid;
    LOGD("Hide daemon forked PID=%d", pid);
    return 0;
}

int ApexLegacyHide::install_ld_preload_hook() {
    const char *hook_lib = "/data/local/tmp/libapex_hook.so";
    if (access(hook_lib, F_OK) != 0) {
        LOGE("Hook lib %s not found", hook_lib);
        return -1;
    }

    FILE *fp = fopen("/data/local/tmp/.apex_env", "w");
    if (!fp) {
        LOGE("Cannot write env file");
        return -1;
    }

    fprintf(fp, "LD_PRELOAD=%s\n", hook_lib);
    fprintf(fp, "APEX_UID=%u\n", apex_uid_);
    fclose(fp);

    LOGD("LD_PRELOAD hook configured via env file");
    return 0;
}

int ApexLegacyHide::load() {
    LOGD("Legacy hide loading (Android 10-11 fallback)");

    int ret = mount_namespace_hide();
    if (ret != 0) {
        LOGE("Mount namespace hide failed, trying fork daemon");
        ret = fork_hide_daemon();
        if (ret != 0) {
            LOGE("Fork daemon also failed, trying LD_PRELOAD");
            ret = install_ld_preload_hook();
            if (ret != 0) {
                LOGE("All legacy strategies failed");
                return -1;
            }
        }
    }

    loaded_ = true;
    LOGD("Legacy hide loaded successfully");
    return 0;
}

void ApexLegacyHide::unload() {
    if (impl_->daemon_pid > 0) {
        kill(impl_->daemon_pid, SIGTERM);
        waitpid(impl_->daemon_pid, nullptr, WNOHANG);
        impl_->daemon_pid = -1;
    }
    loaded_ = false;
    impl_->ns_active = false;
    LOGD("Legacy hide unloaded");
}

bool ApexLegacyHide::is_running() const {
    return loaded_;
}
