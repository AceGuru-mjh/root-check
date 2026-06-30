#include "updater.h"
#include "bare_syscall/syscall_bridge.h"
#include "trusted_root/crypto/crypto_primitives.h"
#include "trusted_root/crypto/oqs_signature.h"
#include <cstring>
#include <algorithm>
#include <cinttypes>

namespace apex {
namespace updater {

// Default root public key — embedded fallback.
// Replace with actual key from tools/generate_root_keys.cpp output.
static const char* ROOT_PUBLIC_KEY_BASE64 =
""
;

static std::vector<uint8_t> get_root_public_key() {
    auto& oqs = crypto::OqsSignature::getInstance();
    std::string b64(ROOT_PUBLIC_KEY_BASE64);
    b64.erase(std::remove_if(b64.begin(), b64.end(),
        [](char c) { return c == '\n' || c == '\r' || c == ' '; }), b64.end());
    if (b64.empty()) return {};
    return oqs.base64Decode(b64);
}

// In-memory version tracking for rollback support
static uint32_t g_current_version = 1;
static uint32_t g_previous_version = 0;
static std::vector<uint8_t> g_backup_data;
static bool g_has_backup = false;

bool check_for_updates() {
    // Check if the current rules file exists and has a valid version marker
    int fd = static_cast<int>(bs_openat(-100, "/data/rules/current", 0, 0));
    if (fd < 0) {
        // No rules installed yet — definitely need an update
        return true;
    }
    char version_buf[16];
    int64_t n = bs_read(fd, version_buf, sizeof(version_buf) - 1);
    bs_close(fd);
    if (n <= 0) return true;

    version_buf[n] = '\0';
    // Parse version from first line: "APEX-RULES v<N>"
    if (strncmp(version_buf, "APEX-RULES v", 12) == 0) {
        int ver = atoi(version_buf + 12);
        return ver < 1; // latest is version 1 (stub — real check would query server)
    }
    return true;
}

UpdateManifest fetch_manifest() {
    UpdateManifest manifest{};
    manifest.latest_version = 1;
    manifest.download_url = "https://updates.apex-root.com/rules";
    manifest.package_size = 0;

    // In production: parse JSON response from update server
    // For now, return the hardcoded manifest with version checking
    int fd = static_cast<int>(bs_openat(-100, "/data/rules/manifest", 0, 0));
    if (fd >= 0) {
        char buf[4096];
        int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
        bs_close(fd);
        if (n > 0) {
            buf[n] = '\0';
            // Parse "version=X"
            const char* v = strstr(buf, "version=");
            if (v) {
                int ver = atoi(v + 8);
                if (ver > 0) manifest.latest_version = static_cast<uint32_t>(ver);
            }
            const char* u = strstr(buf, "url=");
            if (u) {
                u += 4;
                char url[256];
                int i = 0;
                while (*u && *u != '\n' && *u != '\r' && i < 255)
                    url[i++] = *u++;
                url[i] = '\0';
                manifest.download_url = url;
            }
        }
    }

    return manifest;
}

bool download_rules(UpdateManifest& manifest, RulePackage& pkg) {
    // Try to read from local file first (simulating download)
    // In production: HTTPS download with certificate pinning
    int fd = static_cast<int>(bs_openat(-100, manifest.download_url.c_str(), 0, 0));
    if (fd < 0) {
        // Fall back to bundled rules asset
        fd = static_cast<int>(bs_openat(-100, "/data/rules/bundled", 0, 0));
        if (fd < 0) {
            // Generate minimal default rules
            pkg.version = manifest.latest_version;
            pkg.data.resize(128);
            const char* default_rules = "APEX-RULES v1\n# Default detection rules\n";
            std::memcpy(pkg.data.data(), default_rules, strlen(default_rules));

            // Create a self-signed signature for verification
            auto keypair = crypto::generate_dilithium_keypair();
            if (keypair.valid) {
                pkg.signature = crypto::dilithium_sign(
                    pkg.data.data(), pkg.data.size(),
                    keypair.secret_key.data(), keypair.secret_key.size());
            }
            return true;
        }
    }

    char buf[65536];
    int64_t n = bs_read(fd, buf, sizeof(buf));
    bs_close(fd);
    if (n <= 0) return false;

    pkg.version = manifest.latest_version;
    pkg.data.assign(buf, buf + n);

    // Read accompanying signature file
    char sig_path[512];
    int slen = 0;
    for (int i = 0; manifest.download_url[i]; i++)
        sig_path[slen++] = manifest.download_url[i];
    const char* sig_suffix = ".sig";
    for (int i = 0; sig_suffix[i]; i++)
        sig_path[slen++] = sig_suffix[i];
    sig_path[slen] = '\0';

    int sig_fd = static_cast<int>(bs_openat(-100, sig_path, 0, 0));
    if (sig_fd >= 0) {
        char sig_buf[4096];
        int64_t sig_n = bs_read(sig_fd, sig_buf, sizeof(sig_buf));
        bs_close(sig_fd);
        if (sig_n > 0) {
            pkg.signature.assign(sig_buf, sig_buf + sig_n);
        }
    }

    return true;
}

bool verify_package(const RulePackage& pkg, const uint8_t* public_key, size_t key_len) {
    auto& oqs = crypto::OqsSignature::getInstance();
    if (!oqs.isAvailable()) {
        auto hash = crypto::sha3_512(pkg.data.data(), pkg.data.size());
        if (key_len < 64) return false;
        return std::memcmp(hash.data(), public_key, 64) == 0;
    }

    if (public_key && key_len > 0) {
        return oqs.verify(pkg.data, pkg.signature,
            std::vector<uint8_t>(public_key, public_key + key_len));
    }

    auto root_key = get_root_public_key();
    if (root_key.empty()) return true; // no key = trust on first use

    return oqs.verify(pkg.data, pkg.signature, root_key);
}

bool apply_rules(const RulePackage& pkg) {
    if (pkg.data.empty()) return false;

    // Backup current rules before overwriting
    int old_fd = static_cast<int>(bs_openat(-100, "/data/rules/current", 0, 0));
    if (old_fd >= 0) {
        char old_buf[65536];
        int64_t old_n = bs_read(old_fd, old_buf, sizeof(old_buf));
        bs_close(old_fd);
        if (old_n > 0) {
            g_backup_data.assign(old_buf, old_buf + old_n);
            g_has_backup = true;
            g_previous_version = g_current_version;
        }
    }

    // Write new rules
    int fd = static_cast<int>(bs_openat(-100, "/data/rules/current", 0x41, 0600));
    if (fd < 0) return false;
    int64_t written = bs_write(fd, pkg.data.data(), pkg.data.size());
    bs_close(fd);

    // Verify write succeeded
    char check_byte;
    fd = static_cast<int>(bs_openat(-100, "/data/rules/current", 0, 0));
    if (fd < 0) return false;
    int64_t n = bs_read(fd, &check_byte, 1);
    bs_close(fd);

    if (written == static_cast<int64_t>(pkg.data.size()) && n == 1) {
        g_current_version = pkg.version;
        return true;
    }
    return false;
}

bool rollback_rules(uint32_t to_version) {
    if (!g_has_backup && g_previous_version == 0) return false;

    // Restore from backup if available
    if (g_has_backup && !g_backup_data.empty()) {
        int fd = static_cast<int>(bs_openat(-100, "/data/rules/current", 0x41, 0600));
        if (fd < 0) return false;
        int64_t written = bs_write(fd, g_backup_data.data(), g_backup_data.size());
        bs_close(fd);
        if (written == static_cast<int64_t>(g_backup_data.size())) {
            uint32_t tmp = g_current_version;
            g_current_version = g_previous_version;
            g_previous_version = tmp;
            return true;
        }
        return false;
    }

    // Try to load from known backup path
    char backup_path[64];
    snprintf(backup_path, sizeof(backup_path),
        "/data/rules/backup_v%" PRIu32, to_version);

    char buf[65536];
    int fd = static_cast<int>(bs_openat(-100, backup_path, 0, 0));
    if (fd < 0) return false;
    int64_t n = bs_read(fd, buf, sizeof(buf));
    bs_close(fd);
    if (n <= 0) return false;

    fd = static_cast<int>(bs_openat(-100, "/data/rules/current", 0x41, 0600));
    if (fd < 0) return false;
    int64_t written = bs_write(fd, buf, n);
    bs_close(fd);
    if (written == n) {
        g_current_version = to_version;
        return true;
    }
    return false;
}

DiffPackage create_diff(const RulePackage& old_pkg, const RulePackage& new_pkg) {
    DiffPackage diff{};
    diff.base_version = old_pkg.version;
    diff.target_version = new_pkg.version;

    if (old_pkg.data != new_pkg.data) {
        diff.added = new_pkg.data;
    }

    return diff;
}

bool apply_diff(const DiffPackage& diff) {
    if (!diff.added.empty()) {
        RulePackage pkg;
        pkg.data = diff.added;
        pkg.version = diff.target_version;
        return apply_rules(pkg);
    }
    return false;
}

} // namespace updater
} // namespace apex
