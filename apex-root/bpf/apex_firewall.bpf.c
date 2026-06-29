#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

struct clean_baseline {
    char kernel_ver[256];
    char cmdline[512];
    char build_prop[1024];
};

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, u32);
    __type(value, struct clean_baseline);
} baseline_map SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 8);
    __type(key, u32);
    __type(value, bool);
} uid_whitelist SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 64);
    __type(key, u64);
    __type(value, u64);
} open_result_map SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 64);
    __type(key, u64);
    __type(value, u64);
} statx_result_map SEC(".maps");

static __always_inline u32 get_uid(void) {
    u64 uid_gid = bpf_get_current_uid_gid();
    return uid_gid >> 32;
}

static __always_inline bool is_whitelisted(u32 uid) {
    bool *val = bpf_map_lookup_elem(&uid_whitelist, &uid);
    return val != NULL && *val;
}

/*
 * Fixed: Rewrite is_sensitive_raw() with correct pattern iteration.
 * Original code had a bug where `pi` was not reset at the start of each
 * outer loop iteration, and bpf_probe_read_kernel was used unnecessarily
 * on static const data (adding overhead and verifier complexity).
 *
 * New approach: Use a BPF-aware loop that directly compares characters
 * from the static patterns array (no bpf_probe_read_kernel needed for
 * rodata) against the user-space path.
 */
static __always_inline bool match_pattern(const char *path, u32 path_len,
                                           const char *pattern, u32 pat_len) {
    if (pat_len == 0 || pat_len > path_len) return false;

    #pragma unroll
    for (u32 j = 0; j <= path_len - pat_len; j++) {
        bool match = true;
        #pragma unroll
        for (u32 k = 0; k < pat_len; k++) {
            if (path[j + k] != pattern[k]) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

static __always_inline bool is_sensitive_raw(const char *path, u32 max_len) {
    /* Each pattern is null-terminated and packed sequentially */
    const char patterns[] =
        "/su\0"
        "/magisk\0"
        "/ksu\0"
        "/apatch\0"
        "/data/adb\0"
        "/dev/__properties\0"
        "/proc/kallsyms\0"
        "/sbin/.magisk\0";

    u32 i = 0;
    while (i < sizeof(patterns) - 1) {
        /* Compute current pattern length */
        u32 pat_len = 0;
        #pragma unroll
        while (i + pat_len < sizeof(patterns) && patterns[i + pat_len] != '\0') {
            pat_len++;
            if (pat_len >= 64) break; /* safety bound */
        }
        if (pat_len == 0) { i++; continue; }

        if (match_pattern(path, max_len, patterns + i, pat_len)) {
            return true;
        }
        i += pat_len + 1; /* skip past pattern + null terminator */
    }
    return false;
}

SEC("tp/syscalls/sys_enter_openat")
int tp_enter_openat(struct trace_event_raw_sys_enter *ctx) {
    u32 uid = get_uid();
    if (is_whitelisted(uid)) return 0;

    const char *path_ptr = (const char *)BPF_CORE_READ(ctx, args[1]);
    char path_buf[256] = {0};
    long ret = bpf_probe_read_user_str(path_buf, sizeof(path_buf), path_ptr);
    if (ret < 0) return 0;

    if (is_sensitive_raw(path_buf, ret)) {
        u64 id = bpf_get_current_pid_tgid();
        u64 enoent = (u64)-2;
        bpf_map_update_elem(&open_result_map, &id, &enoent, BPF_ANY);
    }
    return 0;
}

SEC("tp/syscalls/sys_exit_openat")
int tp_exit_openat(struct trace_event_raw_sys_exit *ctx) {
    u32 uid = get_uid();
    if (is_whitelisted(uid)) return 0;

    u64 id = bpf_get_current_pid_tgid();
    u64 *val = bpf_map_lookup_elem(&open_result_map, &id);
    if (!val) return 0;

    bpf_map_delete_elem(&open_result_map, &id);
    ctx->ret = (long)*val;
    return 0;
}

SEC("tp/syscalls/sys_enter_statx")
int tp_enter_statx(struct trace_event_raw_sys_enter *ctx) {
    u32 uid = get_uid();
    if (is_whitelisted(uid)) return 0;

    const char *path_ptr = (const char *)BPF_CORE_READ(ctx, args[1]);
    char path_buf[256] = {0};
    long ret = bpf_probe_read_user_str(path_buf, sizeof(path_buf), path_ptr);
    if (ret < 0) return 0;

    if (is_sensitive_raw(path_buf, ret)) {
        u64 id = bpf_get_current_pid_tgid();
        u64 enoent = (u64)-2;
        bpf_map_update_elem(&statx_result_map, &id, &enoent, BPF_ANY);
    }
    return 0;
}

SEC("tp/syscalls/sys_exit_statx")
int tp_exit_statx(struct trace_event_raw_sys_exit *ctx) {
    u32 uid = get_uid();
    if (is_whitelisted(uid)) return 0;

    u64 id = bpf_get_current_pid_tgid();
    u64 *val = bpf_map_lookup_elem(&statx_result_map, &id);
    if (!val) return 0;

    bpf_map_delete_elem(&statx_result_map, &id);
    ctx->ret = (long)*val;
    return 0;
}

struct linux_dirent64 {
    u64 d_ino;
    s64 d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[256];
};

SEC("tp/syscalls/sys_exit_getdents64")
int tp_exit_getdents64(struct trace_event_raw_sys_exit *ctx) {
    u32 uid = get_uid();
    if (is_whitelisted(uid)) return 0;

    long ret = ctx->ret;
    if (ret <= 0) return 0;

    struct linux_dirent64 *dirp = (struct linux_dirent64 *)BPF_CORE_READ(ctx, args[1]);
    long total = 0;
    while (total < ret) {
        struct linux_dirent64 *entry = (struct linux_dirent64 *)((char *)dirp + total);
        unsigned short reclen;
        bpf_probe_read_kernel(&reclen, sizeof(reclen), &entry->d_reclen);
        if (reclen == 0) break;

        /* Safety: skip entries that would cross page boundaries */
        long entry_end = total + reclen;
        if (entry_end > ret || reclen > 4096) break;

        char name[64] = {0};
        bpf_probe_read_kernel_str(name, sizeof(name), entry->d_name);
        bool hidden = false;
        const char *hide_names[] = {"magiskd\0", "ksud\0", "apd\0", "lspd\0", "zygiskd\0"};
        for (u32 i = 0; i < 5; i++) {
            u32 off = 0;
            for (; off < 63; off++) {
                char hc = hide_names[i][off];
                char nc;
                bpf_probe_read_kernel(&nc, 1, name + off);
                if (hc == '\0' || nc != hc) break;
                if (hc == '\0' && nc == '\0') { hidden = true; break; }
            }
            if (hidden) break;
        }

        if (hidden) {
            long remain = ret - total - reclen;
            if (remain > 0) {
                __builtin_memmove((char *)dirp + total, (char *)dirp + total + reclen, remain);
                ret -= reclen;
            } else {
                ret = total;
            }
        } else {
            total += reclen;
        }
    }
    ctx->ret = ret;
    return 0;
}

SEC("tp/syscalls/sys_exit_access")
int tp_exit_access(struct trace_event_raw_sys_exit *ctx) {
    u32 uid = get_uid();
    if (is_whitelisted(uid)) return 0;

    const char *path_ptr = (const char *)BPF_CORE_READ(ctx, args[0]);
    char path_buf[256] = {0};
    long ret = bpf_probe_read_user_str(path_buf, sizeof(path_buf), path_ptr);
    if (ret < 0) return 0;

    if (is_sensitive_raw(path_buf, ret)) {
        ctx->ret = -2;
    }
    return 0;
}

SEC("tp/syscalls/sys_exit_faccessat")
int tp_exit_faccessat(struct trace_event_raw_sys_exit *ctx) {
    u32 uid = get_uid();
    if (is_whitelisted(uid)) return 0;

    const char *path_ptr = (const char *)BPF_CORE_READ(ctx, args[1]);
    char path_buf[256] = {0};
    long ret = bpf_probe_read_user_str(path_buf, sizeof(path_buf), path_ptr);
    if (ret < 0) return 0;

    if (is_sensitive_raw(path_buf, ret)) {
        ctx->ret = -2;
    }
    return 0;
}

char _license[] SEC("license") = "GPL";