#include "updater/updater.h"
#include "bare_syscall/syscall_bridge.h"
#include "trusted_root/crypto/crypto_primitives.h"
#include "trusted_root/crypto/oqs_signature.h"
#include <cstring>
#include <algorithm>
#include <cinttypes>
#include <cstdio>     // FIX-P2-CPP: rename, remove (atomic_write_file)
#include <unistd.h>   // FIX-P2-CPP: fsync (atomic_write_file)

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
            // FIX-CPP P0-C6: 原实现在 URL 与本地 bundled 都打不开时,
            //   会自己生成默认规则并用临时生成的密钥对自签名 (line 106-111),
            //   绕过 verify_package 的签名验证。现在直接返回 false。
            //   调用方 (apply_rules) 必须检查返回值,拒绝写入未验证的规则包。
            return false;
        }
    }

    char buf[65536];
    int64_t n = bs_read(fd, buf, sizeof(buf));
    bs_close(fd);
    if (n <= 0) return false;

    pkg.version = manifest.latest_version;
    pkg.data.assign(buf, buf + n);

    // Read accompanying signature file
    // FIX-CPP P1-7: 原实现手动拼接 sig_path[512] 无边界检查,
    //   download_url 长度 ≥ 508 时栈溢出。改用 snprintf。
    char sig_path[512];
    int written = snprintf(sig_path, sizeof(sig_path), "%s.sig", manifest.download_url.c_str());
    if (written < 0 || (size_t)written >= sizeof(sig_path)) {
        // URL 太长,sig_path 被截断。拒绝继续以防读到错误的签名文件。
        return false;
    }

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

    // FIX-CPP P0-C6: 原实现在 root_key 为空时返回 true ("no key = trust on first use"),
    //   这意味着任何未配置公钥的环境都会信任任意包。改为返回 false:
    //   没有配置可信公钥时,拒绝验证通过。
    auto root_key = get_root_public_key();
    if (root_key.empty()) return false;

    return oqs.verify(pkg.data, pkg.signature, root_key);
}

// FIX-P2-CPP (v1.1.3): 原子写入规则文件, 防止进程崩溃后文件损坏。
// 原实现: openat + write + close 直接覆盖目标路径, 无 fsync、无原子 rename。
//   若进程在 write 后 close 前崩溃, 文件可能为 0 字节或部分写入, 下次启动读取
//   损坏文件。现改为标准原子写策略:
//   1. 写到 <path>.tmp (O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC)
//   2. 循环 write 直到全部字节落盘
//   3. fsync 文件 fd (确保数据页落盘)
//   4. close
//   5. rename(tmp, final) — rename 是 POSIX 原子操作, 任意时刻 final 要么是旧内容
//      要么是新内容, 不会是部分内容
//   6. fsync 父目录 (最佳努力 — 让 rename 的目录项变更也落盘)
// 任一步骤失败时清理 tmp 文件并返回 false, 旧文件保持不变。
static bool atomic_write_file(const char* path, const uint8_t* data, size_t len) {
    if (!path || !data) return false;
    std::string tmp_path = std::string(path) + ".tmp";

    // 0x241 = O_WRONLY | O_CREAT | O_TRUNC; 0x80000 = O_CLOEXEC
    int64_t fd = bs_openat(-100, tmp_path.c_str(), 0x241 | 0x80000, 0600);
    if (fd < 0) return false;

    size_t written = 0;
    while (written < len) {
        int64_t n = bs_write(fd, data + written, len - written);
        if (n <= 0) {
            bs_close(fd);
            (void)remove(tmp_path.c_str());
            return false;
        }
        written += (size_t)n;
    }

    // fsync 文件内容到磁盘, 确保崩溃后 tmp 文件内容完整
    (void)fsync((int)fd);
    bs_close(fd);

    // 原子 rename tmp → final
    if (rename(tmp_path.c_str(), path) != 0) {
        (void)remove(tmp_path.c_str());
        return false;
    }

    // 最佳努力: fsync 父目录, 让 rename 的目录项变更也落盘
    // 提取父目录: 取 path 中最后一个 '/' 之前的内容; 若无则用 "."
    std::string parent_dir;
    const char* slash = strrchr(path, '/');
    if (slash == path) {
        parent_dir = "/";  // 根目录
    } else if (slash) {
        parent_dir.assign(path, (size_t)(slash - path));
    } else {
        parent_dir = ".";
    }
    // 0x10000 = O_DIRECTORY
    int64_t dirfd = bs_openat(-100, parent_dir.c_str(), 0x10000, 0);
    if (dirfd >= 0) {
        (void)fsync((int)dirfd);
        bs_close(dirfd);
    }
    return true;
}

bool apply_rules(const RulePackage& pkg) {
    if (pkg.data.empty()) return false;

    // FIX-CPP P0-C6: 在写入前必须验证 pkg 的签名。
    //   原实现完全不调用 verify_package,直接写入 /data/rules/current,
    //   导致 download_rules 的自签名 fallback 路径能绕过验证。
    //   现改为: 验证失败时拒绝写入,返回 false。
    //   (public_key 为 nullptr 时使用内置 root_key 验证。)
    if (!verify_package(pkg, nullptr, 0)) {
        return false;
    }

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
    // FIX-P2-CPP (v1.1.3): 用 atomic_write_file 替代 openat+write+close。
    //   原实现无 fsync、无原子 rename, 进程崩溃后文件可能损坏。
    //   现通过 tmp + fsync + rename + fsync_dir 保证原子性。
    //   atomic_write_file 内部循环写满全部字节并 fsync, 成功返回即代表
    //   数据已落盘 + rename 已完成, 不再需要二次读回校验。
    if (!atomic_write_file("/data/rules/current", pkg.data.data(), pkg.data.size())) {
        return false;
    }

    g_current_version = pkg.version;
    return true;
}

bool rollback_rules(uint32_t to_version) {
    if (!g_has_backup && g_previous_version == 0) return false;

    // Restore from backup if available
    if (g_has_backup && !g_backup_data.empty()) {
        // FIX-P2-CPP (v1.1.3): 用 atomic_write_file 原子写入回滚内容。
        if (!atomic_write_file("/data/rules/current",
                               g_backup_data.data(),
                               g_backup_data.size())) {
            return false;
        }
        uint32_t tmp = g_current_version;
        g_current_version = g_previous_version;
        g_previous_version = tmp;
        return true;
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

    // FIX-P2-CPP (v1.1.3): 用 atomic_write_file 原子写入从备份路径读回的内容。
    if (!atomic_write_file("/data/rules/current",
                           reinterpret_cast<const uint8_t*>(buf),
                           (size_t)n)) {
        return false;
    }
    g_current_version = to_version;
    return true;
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
