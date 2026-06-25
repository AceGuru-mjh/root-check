#include "firmware_detector.h"
#include "../utils/syscall_utils.h"
#include <crypto_core.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <android/log.h>
#include <dirent.h>

#define LOG_TAG "FirmwareDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

#define CHUNK_SIZE (64 * 1024)
#define MAX_PART_READ (4 * 1024 * 1024)

std::string FirmwareDetector::bytesToHex(const uint8_t* data, size_t len) {
    static const char hex[] = "0123456789abcdef";
    std::string out;
    size_t max = len > 32 ? 32 : len;
    for (size_t i = 0; i < max; i++) {
        out += hex[data[i] >> 4];
        out += hex[data[i] & 0xf];
    }
    return out;
}

std::string FirmwareDetector::hashFile(const std::string& path, size_t max_read) {
    std::string content = SyscallUtils::readFileDirect(path);
    if (content.empty()) {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return "";
        std::stringstream ss;
        ss << f.rdbuf();
        content = ss.str();
        f.close();
    }
    if (content.empty()) return "";
    if (content.size() > max_read) content.resize(max_read);

    uint8_t digest[SHA3_256_DIGEST_LEN];
    sha3_256((const uint8_t*)content.data(), content.size(), digest);
    return bytesToHex(digest, SHA3_256_DIGEST_LEN);
}

std::string FirmwareDetector::hashBlockDevice(const std::string& path, size_t max_read) {
    std::ifstream dev(path, std::ios::binary);
    if (!dev.is_open()) return "";

    uint8_t buf[CHUNK_SIZE];
    sha3_ctx ctx;
    sha3_256_init(&ctx);
    size_t total = 0;

    while (total < max_read) {
        size_t to_read = std::min(sizeof(buf), max_read - total);
        dev.read((char*)buf, to_read);
        size_t got = (size_t)dev.gcount();
        if (got == 0) break;
        sha3_256_update(&ctx, buf, got);
        total += got;
    }
    dev.close();

    uint8_t digest[SHA3_256_DIGEST_LEN];
    sha3_256_final(&ctx, digest);
    return bytesToHex(digest, SHA3_256_DIGEST_LEN);
}

std::string FirmwareDetector::hashBlockDeviceStream(const std::string& path, size_t max_read) {
    return hashBlockDevice(path, max_read);
}

bool FirmwareDetector::readBlockDevice(const std::string& path, uint8_t* buf, size_t offset, size_t size) {
    std::ifstream dev(path, std::ios::binary);
    if (!dev.is_open()) return false;
    dev.seekg(offset, std::ios::beg);
    dev.read((char*)buf, size);
    bool ok = dev.gcount() > 0;
    dev.close();
    return ok;
}

std::string FirmwareDetector::findPartition(const std::vector<std::string>& candidates) {
    for (const auto& path : candidates) {
        std::ifstream f(path, std::ios::binary);
        if (f.is_open()) {
            f.close();
            return path;
        }
    }
    return "";
}

bool FirmwareDetector::scanBootForGap(const std::string& path, int threshold) {
    std::ifstream dev(path, std::ios::binary);
    if (!dev.is_open()) return false;
    int run = 0;
    char c;
    while (dev.get(c)) {
        if (c == 0) { run++; if (run > threshold) { dev.close(); return true; } }
        else { run = 0; }
    }
    dev.close();
    return false;
}

void FirmwareDetector::scanBootPartition(FirmwareDetectDetail& detail, std::vector<std::string>& findings) {
    std::string boot = findPartition({
        "/dev/block/by-name/boot", "/dev/block/by-name/boot_a",
        "/dev/block/by-name/boot_b", "/dev/block/boot"
    });
    if (boot.empty()) {
        findings.push_back("BOOT: no access");
        return;
    }

    detail.boot_hash = hashBlockDevice(boot, MAX_PART_READ);
    findings.push_back("BOOT hash: " + detail.boot_hash);

    detail.boot_magisk_gap = scanBootForGap(boot, 4096);
    if (detail.boot_magisk_gap) findings.push_back("BOOT: magisk gap");

    uint8_t chunk[CHUNK_SIZE];
    if (!readBlockDevice(boot, chunk, 0, sizeof(chunk))) return;
    std::string data((char*)chunk, sizeof(chunk));

    const struct { const char* sig; int risk; const char* label; } sigs[] = {
        /* Magisk variants */
        {"magisk", 30, "Magisk"}, {"MAGISK", 30, "Magisk"}, {"zygisk", 25, "Zygisk"},
        {"magisksu", 30, "MagiskSU"}, {"MagiskAlpha", 30, "MagiskAlpha"},
        {"alpha_magisk", 30, "MagiskAlpha"}, {"MagiskDelta", 30, "MagiskDelta"},
        {"delta_magisk", 30, "MagiskDelta"},
        /* APatch variants */
        {"kpatch", 30, "APatch"}, {"APATCH", 30, "APatch"}, {"apatch", 30, "APatch"},
        {"APatch.apk", 35, "APatch"}, {"kpm", 30, "APatch KPM"},
        /* KernelSU variants */
        {"KernelSU", 30, "KernelSU"}, {"kernelsu", 30, "KernelSU"},
        {"ksu_drv", 30, "KernelSU"}, {"ksud", 30, "KernelSU"},
        {"KSU", 25, "KernelSU"}, {"kernel_su", 30, "KernelSU"},
        {"susfs", 30, "SusFS"}, {"suhide", 30, "SuHide"},
        {"ksu_susfs", 30, "KSU+SusFS"},
        /* Rootkits */
        {"diamorphine", 50, "Diamorphine"}, {"suterusu", 50, "Suterusu"},
        {"reptile", 50, "Reptile"}, {"p3scan", 40, "P3Scan"},
        /* dm-verity */
        {"verity_warning", 15, "AVB warning"}, {"dm_verity", 10, "dm-verity"},
        {"allow_disable", 15, "AVB disable"}, {nullptr, 0, nullptr}
    };

    for (int i = 0; sigs[i].sig; i++) {
        if (data.find(sigs[i].sig) != std::string::npos) {
            if (strstr(sigs[i].sig, "magisk") || sigs[i].risk == 30) {
                detail.boot_magisk_ramdisk = true;
            }
            if (strstr(sigs[i].sig, "kpatch") || strstr(sigs[i].sig, "APATCH")) {
                detail.boot_apatch_kernel = true;
            }
            if (strstr(sigs[i].sig, "KernelSU") || strstr(sigs[i].sig, "kernelsu") ||
                strstr(sigs[i].sig, "KSU") || strstr(sigs[i].sig, "ksu")) {
                detail.boot_kernelsu = true;
            }
            if (sigs[i].risk >= 40) {
                detail.boot_rootkits.push_back(sigs[i].label);
            }
            findings.push_back(std::string("BOOT: ") + sigs[i].label);
        }
    }

    int vboot = 0;
    std::string vbmeta_str = data;
    size_t pos = 0;
    while ((pos = vbmeta_str.find("vbmeta", pos)) != std::string::npos) {
        std::string ctx = vbmeta_str.substr(pos, 80);
        if (ctx.find("disable") != std::string::npos ||
            ctx.find("unlock") != std::string::npos ||
            ctx.find("error") != std::string::npos) {
            vboot++;
        }
        pos += 6;
    }
    if (vboot > 0) {
        findings.push_back("BOOT: vbmeta disable marker");
    }

    detail.boot_risk = (detail.boot_magisk_gap ? 25 : 0) +
                       (detail.boot_magisk_ramdisk ? 30 : 0) +
                       (detail.boot_apatch_kernel ? 30 : 0) +
                       (detail.boot_kernelsu ? 30 : 0) +
                       (int)(detail.boot_rootkits.size() * 40);
}

void FirmwareDetector::scanVendorPartition(FirmwareDetectDetail& detail, std::vector<std::string>& findings) {
    std::string vendor = findPartition({
        "/dev/block/by-name/vendor", "/dev/block/by-name/vendor_a",
        "/dev/block/by-name/vendor_b", "/dev/block/vendor"
    });
    if (!vendor.empty()) {
        detail.vendor_hash = hashBlockDevice(vendor, MAX_PART_READ);
        findings.push_back("VENDOR hash: " + detail.vendor_hash);
    } else {
        findings.push_back("VENDOR: no access");
    }

    const char* rootkit_sos[] = {
        "libhijack", "libinject", "libroot", "libhide", "libsu",
        "libkernelsu", "libzygisk", "libsusfs", "libksud", nullptr
    };
    for (const char** so = rootkit_sos; *so; so++) {
        for (const char* dir : {"/vendor/lib64/", "/vendor/lib/",
                                "/vendor_dlkm/lib64/", "/vendor_dlkm/lib/"}) {
            std::string path = std::string(dir) + *so + ".so";
            std::ifstream f(path);
            if (f.is_open()) {
                detail.vendor_dlkm_rootkit = true;
                detail.vendor_rootkit_sos.push_back(*so);
                findings.push_back(std::string("VENDOR: ") + *so + ".so in " + dir);
                f.close();
            }
        }
    }
    detail.vendor_rootkit_so = !detail.vendor_rootkit_sos.empty();
    detail.vendor_risk = detail.vendor_rootkit_so ? 40 : 0;
}

void FirmwareDetector::scanRecoveryPartition(FirmwareDetectDetail& detail, std::vector<std::string>& findings) {
    std::string recovery = findPartition({
        "/dev/block/by-name/recovery", "/dev/block/by-name/recovery_a",
        "/dev/block/by-name/recovery_b", "/dev/block/recovery"
    });
    if (recovery.empty()) {
        findings.push_back("RECOVERY: no access");
    } else {
        detail.recovery_hash = hashBlockDevice(recovery, MAX_PART_READ);
        findings.push_back("RECOVERY hash: " + detail.recovery_hash);

        uint8_t chunk[CHUNK_SIZE];
        if (readBlockDevice(recovery, chunk, 0, sizeof(chunk))) {
            std::string data((char*)chunk, sizeof(chunk));

            const struct { const char* sig; const char* name; } rec_sigs[] = {
                {"TeamWin", "TWRP"}, {"TW_", "TWRP"}, {"TWRP", "TWRP"},
                {"OrangeFox", "OrangeFox"}, {"FOX_", "OrangeFox"},
                {"PitchBlack", "PitchBlack Recovery"}, {"PBRP", "PitchBlack"},
                {"SHRP", "SHRP"}, {"skyhawk", "SkyHawk Recovery"},
                {"RedWolf", "RedWolf Recovery"}, {"melina", "Melina Recovery"},
                {nullptr, nullptr}
            };
            for (int i = 0; rec_sigs[i].sig; i++) {
                if (data.find(rec_sigs[i].sig) != std::string::npos) {
                    detail.recovery_custom = true;
                    detail.recovery_type = rec_sigs[i].name;
                    findings.push_back(std::string("RECOVERY: ") + rec_sigs[i].name);
                }
            }
        }
    }

    std::string misc = findPartition({"/dev/block/by-name/misc", "/dev/block/misc"});
    if (!misc.empty()) {
        uint8_t chunk[4096];
        if (readBlockDevice(misc, chunk, 0, sizeof(chunk))) {
            std::string data((char*)chunk, sizeof(chunk));
            const char* msgs[] = {"boot-recovery", "update-recovery",
                                  "recovery-update", "wipe-data", nullptr};
            for (const char** m = msgs; *m; m++) {
                if (data.find(*m) != std::string::npos) {
                    findings.push_back(std::string("MISC: ") + *m);
                    detail.recovery_risk += 8;
                }
            }
        }
    }
    detail.recovery_risk += detail.recovery_custom ? 20 : 0;
}

void FirmwareDetector::scanSecureStorage(FirmwareDetectDetail& detail, std::vector<std::string>& findings) {
    const char* tee_devs[] = {
        "/dev/tee0", "/dev/tee1", "/dev/trusty0",
        "/dev/trusty", "/dev/tzdev", "/dev/gzdev", nullptr
    };
    for (const char** d = tee_devs; *d; d++) {
        std::ifstream f(*d);
        if (f.is_open()) {
            detail.tee_devices_found = true;
            findings.push_back(std::string("TEE dev: ") + *d);
            f.close();
        }
    }

    std::string keys = SyscallUtils::readFileDirect("/proc/keys");
    if (keys.empty()) {
        std::ifstream kf("/proc/keys");
        if (kf.is_open()) {
            std::stringstream ss;
            ss << kf.rdbuf();
            keys = ss.str();
            kf.close();
        }
    }
    if (!keys.empty()) {
        const char* key_types[] = {"trusty", "widevine", "keymaster",
                                    "gatekeeper", "auth", "otp", "tee", nullptr};
        for (const char** k = key_types; *k; k++) {
            if (keys.find(*k) != std::string::npos) {
                detail.tee_keys.push_back(*k);
                findings.push_back(std::string("TEE key: ") + *k);
                detail.tee_risk += 4;
            }
        }
    }

    const char* sec_parts[] = {
        "/dev/block/by-name/sec", "/dev/block/by-name/keystore",
        "/dev/block/by-name/trusty", "/dev/block/by-name/tee",
        "/dev/block/by-name/ta", "/dev/block/by-name/efs", nullptr
    };
    for (const char** p = sec_parts; *p; p++) {
        std::string hash = hashBlockDevice(*p, MAX_PART_READ);
        if (!hash.empty()) {
            detail.sec_partition_hashes.push_back(std::string(*p) + ": " + hash);
            findings.push_back(std::string("TEE sec: ") + *p + " hash=" + hash);
            detail.tee_risk += 5;
        }
    }

    if (!detail.tee_devices_found) {
        findings.push_back("TEE: no TEE devices");
    }
}

void FirmwareDetector::scanLogicalPartitions(FirmwareDetectDetail& detail, std::vector<std::string>& findings) {
    std::string super = findPartition({
        "/dev/block/by-name/super", "/dev/block/by-name/super_a",
        "/dev/block/by-name/super_b", "/dev/block/super"
    });
    if (!super.empty()) {
        detail.super_hash = hashBlockDevice(super, MAX_PART_READ);
        findings.push_back("SUPER hash: " + detail.super_hash);
    } else {
        findings.push_back("SUPER: no access");
    }

    const char* vbmeta_parts[] = {
        "/dev/block/by-name/vbmeta", "/dev/block/by-name/vbmeta_a",
        "/dev/block/by-name/vbmeta_b", "/dev/block/by-name/vbmeta_system",
        "/dev/block/by-name/vbmeta_system_a", "/dev/block/by-name/vbmeta_system_b",
        nullptr
    };
    for (const char** v = vbmeta_parts; *v; v++) {
        uint8_t chunk[4096];
        if (readBlockDevice(*v, chunk, 0, sizeof(chunk))) {
            std::string data((char*)chunk, sizeof(chunk));
            std::transform(data.begin(), data.end(), data.begin(), ::tolower);
            if (data.find("disable") != std::string::npos ||
                data.find("unlocked") != std::string::npos ||
                data.find("error") != std::string::npos) {
                detail.vbmeta_verify_off = true;
                findings.push_back(std::string("VBMETA: verification off in ") + *v);
            }
        }
    }
    detail.logical_risk += detail.vbmeta_verify_off ? 30 : 0;

    std::string dtbo = findPartition({
        "/dev/block/by-name/dtbo", "/dev/block/by-name/dtbo_a",
        "/dev/block/by-name/dtbo_b", "/dev/block/dtbo"
    });
    if (!dtbo.empty()) {
        detail.dtbo_hash = hashBlockDevice(dtbo, MAX_PART_READ);
        findings.push_back("DTBO hash: " + detail.dtbo_hash);
    }

    std::string persist = findPartition({
        "/dev/block/by-name/persist", "/dev/block/persist"
    });
    if (!persist.empty()) {
        uint8_t chunk[8192];
        if (readBlockDevice(persist, chunk, 0, sizeof(chunk))) {
            std::string data((char*)chunk, sizeof(chunk));
            const char* root_markers[] = {"modified_by_root", "su_installed",
                                          "boot_unlocked", nullptr};
            for (const char** m = root_markers; *m; m++) {
                if (data.find(*m) != std::string::npos) {
                    detail.persist_modified = true;
                    findings.push_back(std::string("PERSIST: ") + *m);
                    detail.logical_risk += 10;
                }
            }
        }
    }
}

void FirmwareDetector::scanKernelLogs(FirmwareDetectDetail& detail, std::vector<std::string>& findings) {
    const char* log_paths[] = {
        "/sys/fs/pstore", "/sys/fs/pstore/console-ramoops",
        "/sys/fs/pstore/dmesg-ramoops-0", "/sys/fs/pstore/dmesg-ramoops-1",
        "/proc/last_kmsg", nullptr
    };
    for (const char** p = log_paths; *p; p++) {
        std::string content = SyscallUtils::readFileDirect(*p);
        if (content.empty()) {
            std::ifstream f(*p);
            if (f.is_open()) {
                std::stringstream ss;
                ss << f.rdbuf();
                content = ss.str();
                f.close();
            }
        }
        if (content.empty()) continue;

        if (strstr(*p, "pstore")) detail.has_pstore = true;

        const char* markers[] = {
            "rooted", "su:", "magisk", "kernelsu", "apatch",
            "init: untrusted", "SELinux: permissive", "kernel: locked",
            "tampered", "verifiedboot", "dm-verity", "avb:",
            "wdt: trigger", "panic", "oops", nullptr
        };
        for (const char** m = markers; *m; m++) {
            if (content.find(*m) != std::string::npos) {
                detail.log_markers.push_back(*m);
                findings.push_back(std::string("LOG: ") + *m + " in " + *p);
                detail.log_risk += 3;
            }
        }
    }
}

void FirmwareDetector::scanSelfCheck(FirmwareDetectDetail& detail, std::vector<std::string>& findings) {
    std::string tracer = SyscallUtils::readFileDirect("/proc/self/status");
    if (tracer.empty()) {
        std::ifstream f("/proc/self/status");
        if (f.is_open()) {
            std::stringstream ss;
            ss << f.rdbuf();
            tracer = ss.str();
            f.close();
        }
    }
    if (!tracer.empty()) {
        auto pos = tracer.find("TracerPid:");
        if (pos != std::string::npos) {
            std::string val = tracer.substr(pos + 10);
            val.erase(0, val.find_first_not_of(" \t"));
            int pid = std::atoi(val.c_str());
            if (pid > 0) {
                detail.traced = true;
                detail.self_risk += 30;
                findings.push_back("SELF: TracerPid=" + std::to_string(pid));
            }
        }
    }

    std::string maps = SyscallUtils::readFileDirect("/proc/self/maps");
    if (maps.empty()) {
        std::ifstream f("/proc/self/maps");
        if (f.is_open()) {
            std::stringstream ss;
            ss << f.rdbuf();
            maps = ss.str();
            f.close();
        }
    }
    if (!maps.empty()) {
        const char* hook_libs[] = {"frida", "xposed", "substrate", "cydia",
                                    "dobby", "badger", "lint", nullptr};
        for (const char** h = hook_libs; *h; h++) {
            if (maps.find(*h) != std::string::npos) {
                detail.hooked = true;
                detail.self_risk += 25;
                findings.push_back(std::string("SELF: hook lib ") + *h);
            }
        }
    }
}

FirmwareDetectDetail FirmwareDetector::detectDetailed() {
    FirmwareDetectDetail detail = {};
    std::vector<std::string> findings;

    LOGI("=== L7 Firmware/Partition Integrity Check ===");

    scanBootPartition(detail, findings);
    scanVendorPartition(detail, findings);
    scanRecoveryPartition(detail, findings);
    scanSecureStorage(detail, findings);
    scanLogicalPartitions(detail, findings);
    scanKernelLogs(detail, findings);
    scanSelfCheck(detail, findings);

    detail.findings = findings;
    detail.detected = detail.boot_risk > 0 || detail.vendor_risk > 0 ||
                      detail.recovery_risk > 0 || detail.tee_risk > 15 ||
                      detail.logical_risk > 0 || detail.log_risk > 0 ||
                      detail.self_risk > 0;

    return detail;
}

DetectionResult FirmwareDetector::detect() {
    DetectionResult result;
    result.layer = "第7层(TEE)：固件/分区完整性校验";
    result.detected = false;

    auto detail = detectDetailed();

    std::string detail_str;
    for (const auto& f : detail.findings) {
        detail_str += f + "\n";
    }

    if (detail.detected) {
        result.detected = true;
        result.detail = "分区完整性异常\n" + detail_str;
    } else {
        result.detail = "分区完整性校验通过\n" + detail_str;
    }

    return result;
}
