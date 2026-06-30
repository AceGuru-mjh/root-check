#include "hwid_spoof.h"
#include "../common/utils.h"
#include "../common/syscall.h"
#include "../bare_syscall/syscall_bridge.h"
#include <cstring>
#include <cinttypes>

namespace apex {
namespace hid {

// Saved real HWID values for restore
static char g_real_serial[128] = {0};
static char g_real_mac[64] = {0};

static void capture_real_hwids() {
    if (g_real_serial[0] != 0) return; // already captured

    // Capture serial via getprop
    auto read_prop = [](const char* prop) -> std::string {
        FILE* f = fopen(prop, "r");
        if (!f) return "";
        char line[128] = {};
        if (fgets(line, sizeof(line), f)) {
            fclose(f);
            std::string s(line);
            while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back();
            return s;
        }
        fclose(f);
        return "";
    };

    // Read actual values from /proc/cmdline for serial
    char cmdline[1024];
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc/cmdline"), "i"(O_RDONLY), "i"(0));
    if (fd >= 0) {
        int64_t n;
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(cmdline), "r"((int64_t)sizeof(cmdline) - 1));
        if (n > 0) {
            cmdline[n] = '\0';
            const char* serial_tag = "androidboot.serialno=";
            const char* serial_pos = strstr(cmdline, serial_tag);
            if (serial_pos) {
                serial_pos += strlen(serial_tag);
                int i = 0;
                while (*serial_pos && *serial_pos != ' ' && *serial_pos != '\n' && i < 127)
                    g_real_serial[i++] = *serial_pos++;
                g_real_serial[i] = '\0';
            }
        }
        int64_t d;
        asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
    }
}

bool spoof_all_hwids() {
    capture_real_hwids();
    spoof_serial();
    spoof_mac_address();
    spoof_android_id();
    spoof_device_fingerprint();
    return true;
}

bool spoof_serial() {
    utils::exec_su_command_quiet("resetprop ro.serialno APEX-SANDBOX-0001 2>/dev/null");
    utils::exec_su_command_quiet("resetprop ro.boot.serialno APEX-SANDBOX-0001 2>/dev/null");
    return true;
}

bool spoof_mac_address() {
    utils::exec_su_command_quiet("resetprop ro.boot.wifimac 02:00:00:00:00:01 2>/dev/null");
    utils::exec_su_command_quiet("resetprop ro.boot.btmac 02:00:00:00:00:01 2>/dev/null");
    return true;
}

bool spoof_android_id() {
    utils::exec_su_command_quiet("settings put secure android_id a1b2c3d4e5f67890 2>/dev/null");
    return true;
}

bool spoof_device_fingerprint() {
    utils::exec_su_command_quiet("resetprop ro.build.fingerprint google/walleye/walleye:12/SP1A.210812.016/12345678:user/release-keys 2>/dev/null");
    utils::exec_su_command_quiet("resetprop ro.build.description walleye-user 12 SP1A.210812.016 12345678 release-keys 2>/dev/null");
    return true;
}

bool restore_real_hwids() {
    capture_real_hwids();

    if (g_real_serial[0] != '\0') {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "resetprop ro.serialno %s 2>/dev/null", g_real_serial);
        utils::exec_su_command_quiet(cmd);
        snprintf(cmd, sizeof(cmd), "resetprop ro.boot.serialno %s 2>/dev/null", g_real_serial);
        utils::exec_su_command_quiet(cmd);
    }

    // Clear spoofed properties
    utils::exec_su_command_quiet("resetprop ro.boot.wifimac '' 2>/dev/null");
    utils::exec_su_command_quiet("resetprop ro.boot.btmac '' 2>/dev/null");
    utils::exec_su_command_quiet("settings put secure android_id '' 2>/dev/null");
    utils::exec_su_command_quiet("resetprop ro.build.fingerprint '' 2>/dev/null");

    return true;
}

bool generate_sandbox_hwid(const char* sandbox_name) {
    if (!sandbox_name || !sandbox_name[0]) return false;

    // Generate deterministic HWID based on sandbox name
    uint64_t hash = 0;
    for (int i = 0; sandbox_name[i]; i++) {
        hash = hash * 31 + (uint8_t)sandbox_name[i];
    }

    char serial[64];
    snprintf(serial, sizeof(serial), "SANDBOX-%016" PRIx64, hash);

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "resetprop ro.serialno %s 2>/dev/null", serial);
    utils::exec_su_command_quiet(cmd);
    snprintf(cmd, sizeof(cmd), "resetprop ro.boot.serialno %s 2>/dev/null", serial);
    utils::exec_su_command_quiet(cmd);

    // Spoof MAC based on hash
    snprintf(cmd, sizeof(cmd),
        "resetprop ro.boot.wifimac 02:00:00:%02x:%02x:%02x 2>/dev/null",
        (uint8_t)(hash >> 16), (uint8_t)(hash >> 8), (uint8_t)hash);
    utils::exec_su_command_quiet(cmd);

    return true;
}

} // namespace hid
} // namespace apex
