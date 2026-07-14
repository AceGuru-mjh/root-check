#include "key_management/key_derivation.h"
#include "signing/signing_center.h"
#include "self_protection/self_protection.h"
#include "bare_syscall/syscall_bridge.h"

int main(int argc, char* argv[]) {
    bs_getpid(); // warm up

    // Initialize self-protection (exits if debugger detected)
    apex::protection::init_protection();

    // Initialize key management
    apex::key::initialize_device_key();
    apex::signing::initialize_signing_center();

    // Monitor loop
    while (true) {
        // Periodic self-protection checks
        if (apex::protection::check_debugger() ||
            apex::protection::check_injection()) {
            bs_exit(1);
        }

        bs_nanosleep(3000000000ULL); // 3 second heartbeat
    }

    return 0;
}
