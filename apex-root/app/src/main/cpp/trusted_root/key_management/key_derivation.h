#ifndef APEX_ROOT_KEY_DERIVATION_H
#define APEX_ROOT_KEY_DERIVATION_H

#include <cstdint>
#include <cstddef>
#include <vector>

namespace apex {
namespace key {

struct DeviceKeyPair {
    std::vector<uint8_t> public_key;
    std::vector<uint8_t> secret_key;
};

struct SessionKey {
    std::vector<uint8_t> key;
    uint64_t created_at;
};

bool initialize_device_key();
DeviceKeyPair get_device_keypair();
SessionKey generate_session_key();
bool store_encrypted(const uint8_t* data, size_t len, const char* name);
bool load_encrypted(uint8_t* out, size_t* len, const char* name);

} // namespace key
} // namespace apex

#endif // APEX_ROOT_KEY_DERIVATION_H
