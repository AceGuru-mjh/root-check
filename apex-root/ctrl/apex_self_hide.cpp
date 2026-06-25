#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#include <android/log.h>

#define LOG_TAG "APEX-SELF"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

class ApexSelfHide {
private:
    pid_t pid_;
    char proc_path_[64];
    bool hidden_ = false;

    static bool path_matches_pid(const char *name, pid_t pid) {
        char expected[16];
        snprintf(expected, sizeof(expected), "%d", pid);
        return strcmp(name, expected) == 0;
    }

public:
    ApexSelfHide() {
        pid_ = getpid();
        snprintf(proc_path_, sizeof(proc_path_), "/proc/%d", pid_);
        LOGD("Self-hide initialized: PID=%d, path=%s", pid_, proc_path_);
    }

    int hide_from_proc() {
        // Strategy 1: Use mount namespace to overlay /proc
        if (unshare(CLONE_NEWNS) == 0) {
            if (mount("tmpfs", "/proc", "tmpfs", MS_NOSUID | MS_NOEXEC, NULL) == 0) {
                LOGD("Self-hide: mounted tmpfs over /proc (aggressive)");
                hidden_ = true;
                return 0;
            }
        }

        // Strategy 2: bind-mount empty dir over our /proc entry
        char empty_dir[] = "/data/local/tmp/.apex_empty";
        mkdir(empty_dir, 0000);

        if (mount(empty_dir, proc_path_, NULL, MS_BIND, NULL) == 0) {
            LOGD("Self-hide: bind-mounted empty dir over %s", proc_path_);
            hidden_ = true;
            return 0;
        }

        LOGD("Self-hide: mount-based hiding not supported, using LD_PRELOAD");
        hidden_ = true;
        return 1;
    }

    int hide_bpf_programs() {
        const char *bpf_fs_paths[] = {
            "/sys/fs/bpf/apex_firewall",
            "/sys/fs/bpf/apex_root",
            "/sys/fs/bpf/tp/apex",
            NULL
        };

        for (int i = 0; bpf_fs_paths[i]; i++) {
            if (access(bpf_fs_paths[i], F_OK) == 0) {
                // Try to hide via mount overlay
                char hide_src[] = "/dev/null";
                if (mount(hide_src, bpf_fs_paths[i], NULL, MS_BIND, NULL) == 0) {
                    LOGD("Hidden BPF path: %s", bpf_fs_paths[i]);
                } else {
                    // Fallback: try to unlink (will fail for pinned progs but try)
                    unlink(bpf_fs_paths[i]);
                }
            }
        }

        return 0;
    }

    int hide_module_entry() {
        const char *module_paths[] = {
            "/data/adb/modules/apex-root",
            "/data/adb/modules_update/apex-root",
            NULL
        };

        for (int i = 0; module_paths[i]; i++) {
            if (access(module_paths[i], F_OK) == 0) {
                char empty_dir[] = "/data/local/tmp/.apex_empty";
                if (mount(empty_dir, module_paths[i], NULL, MS_BIND, NULL) == 0) {
                    LOGD("Hidden module entry: %s", module_paths[i]);
                }
            }
        }
        return 0;
    }

    void reveal_all() {
        if (hidden_) {
            umount(proc_path_);
            umount("/sys/fs/bpf/apex_firewall");
            const char *module_paths[] = {
                "/data/adb/modules/apex-root",
                "/data/adb/modules_update/apex-root",
                NULL
            };
            for (int i = 0; module_paths[i]; i++) {
                umount(module_paths[i]);
            }
            hidden_ = false;
            LOGD("All self-hide mounts reverted");
        }
    }

    bool is_hidden() const { return hidden_; }
};
