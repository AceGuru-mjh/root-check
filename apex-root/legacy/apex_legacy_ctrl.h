#pragma once
#include <cstdint>
#include <string>

class ApexLegacyHide {
public:
    enum Mode {
        MODE_DETECT = 0,
        MODE_HIDE = 1,
        MODE_GAME = 2,
    };

    explicit ApexLegacyHide(uint32_t apex_uid);
    ~ApexLegacyHide();

    int load();
    void unload();
    bool is_running() const;

    int mount_namespace_hide();

private:
    struct Impl;
    Impl *impl_;
    uint32_t apex_uid_;
    bool loaded_;

    int fork_hide_daemon();
    int install_ld_preload_hook();
};
