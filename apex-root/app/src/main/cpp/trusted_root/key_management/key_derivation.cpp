#include "key_derivation.h"
#include "crypto/crypto_primitives.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>
#include <cinttypes>

namespace apex {
namespace key {

static DeviceKeyPair g_device_key;
static bool g_initialized = false;

bool initialize_device_key() {
    if (g_initialized) return true;

    // Generate a Dilithium3 keypair using liboqs via our crypto wrapper
    auto kp = crypto::generate_dilithium_keypair();
    g_device_key.public_key = kp.public_key;
    g_device_key.secret_key = kp.secret_key;

    if (g_device_key.public_key.empty() || g_device_key.secret_key.empty()) {
        return false;
    }

    g_initialized = true;
    return true;
}

DeviceKeyPair get_device_keypair() {
    return g_device_key;
}

SessionKey generate_session_key() {
    SessionKey sk;
    sk.key.resize(32);
    // Get random bytes from kernel
    for (size_t i = 0; i < 32; i += 8) {
        uint64_t r = bs_get_random();
        std::memcpy(sk.key.data() + i, &r, (32 - i < 8) ? (32 - i) : 8);
    }
    sk.created_at = bs_clock_ns();
    return sk;
}

bool store_encrypted(const uint8_t* data, size_t len, const char* name) {
    auto ct = crypto::aes256_gcm_encrypt(data, len, g_device_key.secret_key.data(), 32);
    if (ct.empty()) return false;
    // Write to app's internal storage via fd
    auto fd = bs_openat(-100, name, 0x41, 0600); // O_CREAT|O_RDWR
    if (fd < 0) return false;
    auto written = bs_write(fd, ct.data(), ct.size());
    bs_close(fd);
    return written == static_cast<int64_t>(ct.size());
}

bool load_encrypted(uint8_t* out, size_t* len, const char* name) {
    auto fd = bs_openat(-100, name, 0, 0); // O_RDONLY
    if (fd < 0) return false;
    char buf[4096];
    auto n = bs_read(fd, buf, sizeof(buf));
    bs_close(fd);
    if (n <= 0) return false;

    auto pt = crypto::aes256_gcm_decrypt(
        reinterpret_cast<const uint8_t*>(buf), n,
        g_device_key.secret_key.data(), 32);
    if (pt.empty()) return false;

    if (*len < pt.size()) return false;
    std::memcpy(out, pt.data(), pt.size());
    *len = pt.size();
    return true;
}

} // namespace key
} // namespace apex
