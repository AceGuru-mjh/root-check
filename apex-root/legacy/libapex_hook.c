#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>

static int apex_uid = -1;
static int initialized = 0;

static void ensure_init(void) {
    if (initialized) return;
    initialized = 1;
    char *env = getenv("APEX_UID");
    if (env) apex_uid = atoi(env);
}

static int should_hide(void) {
    ensure_init();
    uid_t uid = getuid();
    return uid != (uid_t)apex_uid;
}

typedef int (*orig_open_t)(const char *, int, ...);
typedef int (*orig_stat_t)(const char *, struct stat *);
typedef int (*orig_access_t)(const char *, int);
typedef DIR *(*orig_opendir_t)(const char *);
typedef struct dirent *(*orig_readdir_t)(DIR *);

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
