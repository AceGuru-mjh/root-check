#ifndef APEX_ROOT_HWID_SPOOF_H
#define APEX_ROOT_HWID_SPOOF_H

namespace apex {
namespace hid {

// Spoof all hardware identifiers for anti-detection
bool spoof_all_hwids();

// Spoof individual hardware IDs
bool spoof_serial();
bool spoof_mac_address();
bool spoof_android_id();
bool spoof_device_fingerprint();

// Reset to real hardware IDs
bool restore_real_hwids();

// Generate unique HWID for each sandbox
bool generate_sandbox_hwid(const char* sandbox_name);

} // namespace hid
} // namespace apex

#endif
