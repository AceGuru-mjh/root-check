#pragma once

#define APEX_FW_MAX_WHITELIST 8

enum apex_fw_mode {
    APEX_FW_MODE_DETECT = 0,
    APEX_FW_MODE_HIDE = 1,
    APEX_FW_MODE_GAME = 2,
};

struct apex_fw_stats {
    __u64 openat_intercepted;
    __u64 statx_intercepted;
    __u64 getdents_filtered;
    __u64 read_spoofed;
    __u64 access_blocked;
};

struct apex_fw_config {
    __u32 apex_uid;
    __u32 mode;
    __u32 whitelist_count;
    __u32 whitelist[APEX_FW_MAX_WHITELIST];
};
