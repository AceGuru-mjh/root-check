#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <sched.h>
#include <android/log.h>

#define LOG_TAG "APEX-SELF"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

class ApexSelfHide {
private:
    pid_t pid_;
    char proc_path_[64];
    bool hidden_ = false;
    bool tmpfs_used_ = false;  /* Track if aggressive strategy was used */
    char bpf_mount_paths_[3][64];
    int bpf_mount_count_ = 0;
    char module_mount_paths_[2][64];
    int module_mount_count_ = 0;

    static bool path_matches_pid(const char *name, pid_t pid) {
        char expected[16];
        snprintf(expected, sizeof(expected), "%d", pid);
        return strcmp(name, expected) == 0;
    }

    /*
     * Create a persistent empty directory for bind-mount source.
     * Returns 0 on success, -1 on failure.
     */
    int ensure_empty_dir(const char *path) {
        struct stat st;
        if (stat(path, &st) != 0) {
            if (mkdir(path, 0700) != 0) {
                LOGE("Failed to create empty dir %s: %s", path, strerror(errno));
                return -1;
            }
        }
        return 0;
    }

public:
    ApexSelfHide() {
        pid_ = getpid();
        snprintf(proc_path_, sizeof(proc_path_), "/proc/%d", pid_);
        LOGD("Self-hide initialized: PID=%d, path=%s", pid_, proc_path_);
    }

    /*
     * Fixed: Original code mounted tmpfs over the ENTIRE /proc, breaking
     * all process information access. Now we only bind-mount an empty
     * directory over our own /proc/<pid> entry.
     */
    int hide_from_proc() {
        const char *empty_dir = "/data/local/tmp/.apex_empty";

        if (ensure_empty_dir(empty_dir) != 0) {
            LOGE("hide_from_proc: cannot create empty dir");
            return -1;
        }

        /* Strategy 1 (preferred): bind-mount empty dir over /proc/<pid> */
        if (mount(empty_dir, proc_path_, NULL, MS_BIND, NULL) == 0) {
            LOGD("Self-hide: bind-mounted empty dir over %s", proc_path_);
            hidden_ = true;
            return 0;
        }

        /* Strategy 2: mount namespace + bind mount */
        if (unshare(CLONE_NEWNS) == 0) {
            if (mount(empty_dir, proc_path_, NULL, MS_BIND, NULL) == 0) {
                LOGD("Self-hide: namespace + bind-mount over %s", proc_path_);
                hidden_ = true;
                return 0;
            }
            LOGW("Self-hide: mount namespace created but bind-mount failed: %s",
                 strerror(errno));
        }

        LOGD("Self-hide: mount-based hiding not available, relying on LD_PRELOAD");
        hidden_ = true;
        return 1;
    }

    /*
     * Fixed: Original code tried to bind-mount /dev/null (a file)
     * onto a directory, which fails with EINVAL on most kernels.
     * Now uses an empty directory as the bind-mount source.
     */
    int hide_bpf_programs() {
        const char *bpf_fs_paths[] = {
            "/sys/fs/bpf/apex_firewall",
            "/sys/fs/bpf/apex_root",
            "/sys/fs/bpf/tp/apex",
            NULL
        };

        const char *empty_dir = "/data/local/tmp/.apex_empty";
        if (ensure_empty_dir(empty_dir) != 0) return -1;

        bpf_mount_count_ = 0;
        for (int i = 0; bpf_fs_paths[i]; i++) {
            if (access(bpf_fs_paths[i], F_OK) == 0) {
                if (mount(empty_dir, bpf_fs_paths[i], NULL, MS_BIND, NULL) == 0) {
                    LOGD("Hidden BPF path: %s", bpf_fs_paths[i]);
                    snprintf(bpf_mount_paths_[bpf_mount_count_],
                             sizeof(bpf_mount_paths_[0]),
                             "%s", bpf_fs_paths[i]);
                    bpf_mount_count_++;
                } else {
                    LOGW("Failed to hide BPF path %s: %s",
                         bpf_fs_paths[i], strerror(errno));
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

        const char *empty_dir = "/data/local/tmp/.apex_empty";
        if (ensure_empty_dir(empty_dir) != 0) return -1;

        module_mount_count_ = 0;
        for (int i = 0; module_paths[i]; i++) {
            if (access(module_paths[i], F_OK) == 0) {
                if (mount(empty_dir, module_paths[i], NULL, MS_BIND, NULL) == 0) {
                    LOGD("Hidden module entry: %s", module_paths[i]);
                    snprintf(module_mount_paths_[module_mount_count_],
                             sizeof(module_mount_paths_[0]),
                             "%s", module_paths[i]);
                    module_mount_count_++;
                }
            }
        }
        return 0;
    }

    /*
     * Fixed: Now properly tracks all mounts and reverts them.
     * Original code only unmounted /proc/<pid> and missed BPF paths
     * when tmpfs strategy was used.
     */
    void reveal_all() {
        if (!hidden_) return;

        umount(proc_path_);

        for (int i = 0; i < bpf_mount_count_; i++) {
            umount(bpf_mount_paths_[i]);
            LOGD("Revealed BPF path: %s", bpf_mount_paths_[i]);
        }

        for (int i = 0; i < module_mount_count_; i++) {
            umount(module_mount_paths_[i]);
            LOGD("Revealed module path: %s", module_mount_paths_[i]);
        }

        hidden_ = false;
        tmpfs_used_ = false;
        bpf_mount_count_ = 0;
        module_mount_count_ = 0;
        LOGD("All self-hide mounts reverted");
    }

    bool is_hidden() const { return hidden_; }
};