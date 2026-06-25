#pragma once
#include <cstdint>
#include <string>

struct CleanBaseline {
    char kernel_ver[256];
    char cmdline[512];
    char build_prop[1024];

    bool load_from_file(const std::string &path);
    bool save_to_file(const std::string &path);
};

class ApexFirewall {
public:
    enum Mode {
        MODE_DETECT = 0,
        MODE_HIDE = 1,
        MODE_GAME = 2,
    };

    explicit ApexFirewall(uint32_t apex_uid);
    ~ApexFirewall();

    int load();
    int load_with_baseline(const CleanBaseline &baseline);
    void unload();
    bool is_running() const;

    int switch_mode(Mode mode);
    Mode current_mode() const;

    std::string last_error() const { return last_error_; }

private:
    struct ApexFirewallImpl;
    ApexFirewallImpl *impl_;
    uint32_t apex_uid_;
    Mode current_mode_;
    std::string last_error_;
};
