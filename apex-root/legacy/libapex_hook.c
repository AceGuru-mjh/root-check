#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>

static int apex_uid = -1;
static int initialized = 0;

/*
 * Fixed: Use __atomic built-in for thread-safe one-time initialization.
 * Original code used a plain int flag which is not thread-safe.
 */
static void ensure_init(void) {
    int expected = 0;
    if (__atomic_compare_exchange_n(&initialized, &expected, 1, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
        char *env = getenv("APEX_UID");
        if (env) {
            int val = atoi(env);
            if (val > 0) {
                __atomic_store_n(&apex_uid, val, __ATOMIC_SEQ_CST);
            }
        }
    }
}

/*
 * Fixed: When APEX_UID is not set (default -1), should_hide() returned true
 * for ALL processes including system-critical ones, potentially causing
 * device instability. Now returns false (no hiding) when APEX_UID is invalid.
 */
static int should_hide(void) {
    ensure_init();
    int uid = __atomic_load_n(&apex_uid, __ATOMIC_SEQ_CST);
    if (uid <= 0) return 0;  /* Safety: no hiding if UID not configured */
    return getuid() != (uid_t)uid;
}

typedef int (*orig_open_t)(const char *, int, ...);
typedef int (*orig_stat_t)(const char *, struct stat *);
typedef int (*orig_access_t)(const char *, int);

static const char *HIDE_STRINGS[] = {
    "/su", "/magisk", "/ksu", "/apatch",
    "/data/adb", "magiskd", "ksud", "lspd",
    "/sbin/.magisk", NULL,
};

static int is_hidden_path(const char *path) {
    if (!path) return 0;
    for (int i = 0; HIDE_STRINGS[i]; i++) {
        if (strstr(path, HIDE_STRINGS[i])) return 1;
    }
    return 0;
}

int open(const char *path, int flags, ...) {
    if (should_hide() && is_hidden_path(path)) {
        errno = ENOENT;
        return -1;
    }
    static orig_open_t orig = NULL;
    if (!orig) orig = (orig_open_t)dlsym(RTLD_NEXT, "open");
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode_t mode = va_arg(ap, mode_t);
        va_end(ap);
        return orig(path, flags, mode);
    }
    return orig(path, flags);
}

int __openat(int dirfd, const char *path, int flags, ...) {
    if (should_hide() && is_hidden_path(path)) {
        errno = ENOENT;
        return -1;
    }
    static orig_open_t orig = NULL;
    if (!orig) orig = (orig_open_t)dlsym(RTLD_NEXT, "__openat");
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode_t mode = va_arg(ap, mode_t);
        va_end(ap);
        return orig(dirfd, path, flags, mode);
    }
    return orig(dirfd, path, flags);
}

int stat(const char *path, struct stat *buf) {
    if (should_hide() && is_hidden_path(path)) {
        errno = ENOENT;
        return -1;
    }
    static orig_stat_t orig = NULL;
    if (!orig) orig = (orig_stat_t)dlsym(RTLD_NEXT, "stat");
    return orig(path, buf);
}

int lstat(const char *path, struct stat *buf) {
    if (should_hide() && is_hidden_path(path)) {
        errno = ENOENT;
        return -1;
    }
    static orig_stat_t orig = NULL;
    if (!orig) orig = (orig_stat_t)dlsym(RTLD_NEXT, "lstat");
    return orig(path, buf);
}

int access(const char *path, int mode) {
    if (should_hide() && is_hidden_path(path)) {
        errno = ENOENT;
        return -1;
    }
    static orig_access_t orig = NULL;
    if (!orig) orig = (orig_access_t)dlsym(RTLD_NEXT, "access");
    return orig(path, mode);
}

/*
 * Added: faccessat hook — Android 10+ frequently uses faccessat
 * instead of the legacy access() syscall.
 */
int faccessat(int dirfd, const char *path, int mode, int flags) {
    if (should_hide() && is_hidden_path(path)) {
        errno = ENOENT;
        return -1;
    }
    typedef int (*orig_faccessat_t)(int, const char *, int, int);
    static orig_faccessat_t orig = NULL;
    if (!orig) orig = (orig_faccessat_t)dlsym(RTLD_NEXT, "faccessat");
    return orig(dirfd, path, mode, flags);
}