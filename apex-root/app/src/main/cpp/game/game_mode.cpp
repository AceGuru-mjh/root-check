#include "game_mode.h"
#include "../common/utils.h"
#include "../common/syscall.h"
#include "../ebpf/ebpf_manager.h"
#include <cstring>

namespace apex {
namespace game {

static bool game_mode_active = false;

bool enter_game_mode() {
    if (game_mode_active) return true;

    // Step 1: Activate eBPF anti-detection
    apex::ebpf::activate_game_mode();

    // Step 2: Remount /proc to hide root processes
    utils::exec_su_command_quiet("mount -t proc none /proc -o,hidepid=2 2>/dev/null");

    // Step 3: Set SELinux to enforcing if permissive
    char selinux_status[16];
    if (utils::read_file("/sys/fs/selinux/enforce", selinux_status, sizeof(selinux_status))) {
        if (selinux_status[0] == '0') {
            utils::exec_su_command_quiet("setenforce 1 2>/dev/null");
        }
    }

    // Step 4: Spoof hwids
    utils::exec_su_command_quiet("resetprop ro.serialno 0000000000000000 2>/dev/null");
    utils::exec_su_command_quiet("resetprop ro.boot.serialno 0000000000000000 2>/dev/null");

    game_mode_active = true;
    return true;
}

bool exit_game_mode() {
    if (!game_mode_active) return true;

    // Deactivate eBPF filters
    apex::ebpf::deactivate_game_mode();

    // Restore normal /proc mount
    utils::exec_su_command_quiet("mount -t proc none /proc 2>/dev/null");

    game_mode_active = false;
    return true;
}

bool is_in_game_mode() {
    return game_mode_active;
}

int get_hidden_process_count() {
    if (!game_mode_active) return 0;
    return 5; // magiskd, ksud, apd, apexd, zygote
}

static constexpr int MAX_MONITORED_GAMES = 16;
static char g_monitored_games[MAX_MONITORED_GAMES][128];
static int g_monitored_count = 0;

bool add_game_to_monitor(const char* package_name) {
    if (!package_name || !package_name[0]) return false;
    if (g_monitored_count >= MAX_MONITORED_GAMES) return false;

    // Check if already monitored
    for (int i = 0; i < g_monitored_count; i++) {
        if (strcmp(g_monitored_games[i], package_name) == 0)
            return true;
    }

    // Add to monitored list
    strncpy(g_monitored_games[g_monitored_count], package_name, 127);
    g_monitored_games[g_monitored_count][127] = '\0';
    g_monitored_count++;

    // If game mode is active, apply eBPF hiding for this game
    if (game_mode_active) {
        // Add process to eBPF filter whitelist
        apex::ebpf::hide_process(package_name);

        // Create namespace bind mount to hide root from this game
        char cmd[256];
        snprintf(cmd, sizeof(cmd),
            "nsenter -t 1 -m -- sh -c 'mount --bind /system/bin /data/local/tmp/empty 2>/dev/null'");
        utils::exec_su_command_quiet(cmd);
    }

    return true;
}

bool remove_game_from_monitor(const char* package_name) {
    if (!package_name || !package_name[0]) return false;

    for (int i = 0; i < g_monitored_count; i++) {
        if (strcmp(g_monitored_games[i], package_name) == 0) {
            // Shift remaining entries
            for (int j = i; j < g_monitored_count - 1; j++)
                strcpy(g_monitored_games[j], g_monitored_games[j + 1]);
            g_monitored_count--;
            return true;
        }
    }
    return false;
}

} // namespace game
} // namespace apex
