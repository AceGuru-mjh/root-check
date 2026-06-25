#ifndef APEX_ROOT_NAMESPACE_ISOLATION_H
#define APEX_ROOT_NAMESPACE_ISOLATION_H

#include <cstdint>

namespace apex {
namespace island {

struct IsolatedEnvironment {
    int child_pid;
    int ns_fd_mnt;
    int ns_fd_pid;
    int ns_fd_net;
    int user_id; // Android user ID for the profile
    bool active;
};

int create_isolated_environment(const char* sandbox_name);
bool destroy_isolated_environment(int pid);
bool run_in_environment(int pid, const char* cmd);
bool apply_seccomp_bpf(int pid);
bool mount_pure_system(const char* sandbox_root);

} // namespace island
} // namespace apex

#endif
