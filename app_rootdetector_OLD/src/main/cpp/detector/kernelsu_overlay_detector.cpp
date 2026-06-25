#include "kernelsu_overlay_detector.h"
#include "../utils/syscall_utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <android/log.h>

#define LOG_TAG "KernelSUOverlay"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

std::string KernelSUOverlayDetector::readFile(const std::string& path) {
    std::string c = SyscallUtils::readFileDirect(path);
    if (!c.empty()) return c;
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool KernelSUOverlayDetector::detectKsuOverlayMounts(std::vector<OverlayMountEntry>& entries) {
    std::string mountinfo = readFile("/proc/self/mountinfo");
    if (mountinfo.empty()) return false;

    std::istringstream stream(mountinfo);
    std::string line;
    bool found = false;

    while (std::getline(stream, line)) {
        if (line.find("overlay") == std::string::npos &&
            line.find("overlayfs") == std::string::npos) continue;

        OverlayMountEntry entry;
        entry.source = "";
        entry.target = "";
        entry.fstype = "overlay";
        entry.is_ksu_hijack = false;

        size_t targetStart = std::string::npos;
        size_t optStart = std::string::npos;
        std::vector<std::string> parts;
        std::istringstream ls(line);
        std::string token;
        while (ls >> token) parts.push_back(token);

        if (parts.size() >= 10) {
            entry.source = parts[3];
            entry.target = parts[4];
            entry.fstype = parts[7];
            entry.options = parts[9];

            size_t lowerPos = entry.options.find("lowerdir=");
            size_t upperPos = entry.options.find("upperdir=");
            size_t workPos = entry.options.find("workdir=");

            if (lowerPos != std::string::npos) {
                size_t end = entry.options.find(',', lowerPos);
                entry.lowerdir = entry.options.substr(lowerPos + 9, end - lowerPos - 9);
            }
            if (upperPos != std::string::npos) {
                size_t end = entry.options.find(',', upperPos);
                entry.upperdir = entry.options.substr(upperPos + 9, end - upperPos - 9);
            }
            if (workPos != std::string::npos) {
                size_t end = entry.options.find(',', workPos);
                entry.workdir = entry.options.substr(workPos + 8, end - workPos - 8);
            }

            bool isSystemOverlay = (entry.target.find("/system") != std::string::npos ||
                                    entry.target.find("/vendor") != std::string::npos ||
                                    entry.target.find("/product") != std::string::npos);

            bool targetsAdb = (entry.target.find("/data/adb") != std::string::npos);
            bool hasKsuUpper = (entry.upperdir.find("ksu") != std::string::npos ||
                                entry.upperdir.find("KernelSU") != std::string::npos);

            if (targetsAdb || hasKsuUpper) {
                entry.is_ksu_hijack = true;
                entry.reason = (targetsAdb ? "targets /data/adb" : "") +
                               (targetsAdb && hasKsuUpper ? " + " : "") +
                               (hasKsuUpper ? "ksu upperdir" : "");
                entries.push_back(entry);
                LOGW("KernelSU overlay 劫持: %s -> %s (%s)",
                     entry.source.c_str(), entry.target.c_str(), entry.reason.c_str());
                found = true;
            } else {
                std::string lowerDir = entry.lowerdir;
                std::transform(lowerDir.begin(), lowerDir.end(), lowerDir.begin(), ::tolower);
                if (lowerDir.find("ksu") != std::string::npos ||
                    lowerDir.find("kernelsu") != std::string::npos) {
                    entry.is_ksu_hijack = true;
                    entry.reason = "lowerdir contains ksu";
                    entries.push_back(entry);
                    found = true;
                }

                if (isSystemOverlay) {
                    if (entry.upperdir.empty() && entry.workdir.empty()) {
                        entry.is_ksu_hijack = false;
                        entry.reason = "native overlay (system, no upper/work)";
                    } else if (entry.upperdir.find("/data/") != std::string::npos) {
                        entry.is_ksu_hijack = true;
                        entry.reason = "system overlay with /data/ upperdir (KSU style)";
                        entries.push_back(entry);
                        found = true;
                    }
                }
            }
        }
    }

    if (!found) {
        std::string mounts = readFile("/proc/self/mounts");
        if (!mounts.empty()) {
            std::istringstream ms(mounts);
            while (std::getline(ms, line)) {
                if (line.find("overlay") == std::string::npos &&
                    line.find("overlayfs") == std::string::npos) continue;
                if (line.find("ksu") != std::string::npos ||
                    line.find("KernelSU") != std::string::npos) {
                    OverlayMountEntry e;
                    e.source = line;
                    e.is_ksu_hijack = true;
                    e.reason = "mounts 中存在 ksu 关键词";
                    entries.push_back(e);
                    found = true;
                }
            }
        }
    }

    return found;
}

bool KernelSUOverlayDetector::detectKsuPrivateParams(std::vector<KsuPrivateParam>& params) {
    std::string mountinfo = readFile("/proc/self/mountinfo");
    if (mountinfo.empty()) return false;

    std::istringstream stream(mountinfo);
    std::string line;
    bool found = false;

    while (std::getline(stream, line)) {
        if (line.find("overlay") == std::string::npos) continue;

        size_t optPos = line.find("rootdir=");
        if (optPos != std::string::npos) {
            size_t end = line.find(',', optPos);
            std::string rootdir = line.substr(optPos + 8, end - optPos - 8);
            if (rootdir.find("ksu") != std::string::npos ||
                rootdir.find("KSU") != std::string::npos) {
                KsuPrivateParam p;
                p.param_name = "rootdir";
                p.param_value = rootdir;
                p.is_ksu_specific = true;
                params.push_back(p);
                LOGW("KernelSU 私有挂载参数: rootdir=%s", rootdir.c_str());
                found = true;
            }
        }

        if (line.find("redirect_dir=") != std::string::npos &&
            line.find("ksu") != std::string::npos) {
            KsuPrivateParam p;
            p.param_name = "redirect_dir";
            p.is_ksu_specific = true;
            params.push_back(p);
            found = true;
        }

        size_t dataPos = line.find("user_xattr_options=");
        if (dataPos != std::string::npos) {
            KsuPrivateParam p;
            p.param_name = "user_xattr_options";
            p.is_ksu_specific = true;
            params.push_back(p);
            found = true;
        }
    }

    std::string cmdline = readFile("/proc/self/cmdline");
    if (!cmdline.empty()) {
        std::string p = cmdline;
        p.erase(std::remove(p.begin(), p.end(), '\0'), p.end());
        if (p.find("overlay") != std::string::npos &&
            p.find("system") != std::string::npos &&
            p.find("ksu") != std::string::npos) {
            KsuPrivateParam param;
            param.param_name = "cmdline";
            param.param_value = p;
            param.is_ksu_specific = true;
            params.push_back(param);
            found = true;
        }
    }

    return found;
}

bool KernelSUOverlayDetector::detectKsuImageInode(std::vector<ImageInodeFeature>& features) {
    auto modules = SyscallUtils::listFilesDirect("/data/adb/modules");
    bool found = false;

    for (const auto& mod : modules) {
        if (mod == "." || mod == "..") continue;
        std::string imgPath = "/data/adb/modules/" + mod + "/img";
        std::string lower = mod;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        struct stat st;
        if (stat(imgPath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            ImageInodeFeature feat;
            feat.path = imgPath;
            feat.inode = st.st_ino;
            feat.size = st.st_size;
            feat.is_ksu_overlay = true;
            features.push_back(feat);
            LOGW("KernelSU overlay 镜像: %s (inode=%lu, size=%lu)",
                 imgPath.c_str(), st.st_ino, st.st_size);
            found = true;
        }

        std::string vendorImg = "/data/adb/modules/" + mod + "/vendor_img";
        if (stat(vendorImg.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            ImageInodeFeature feat;
            feat.path = vendorImg;
            feat.inode = st.st_ino;
            feat.size = st.st_size;
            feat.is_ksu_overlay = true;
            features.push_back(feat);
            found = true;
        }
    }

    std::string mounts = readFile("/proc/self/mounts");
    if (!mounts.empty()) {
        std::istringstream ms(mounts);
        std::string line;
        while (std::getline(ms, line)) {
            if (line.find("/data/adb/modules") != std::string::npos) {
                struct stat st;
                if (stat("/data/adb/modules", &st) == 0) {
                    ImageInodeFeature feat;
                    feat.path = "/data/adb/modules";
                    feat.inode = st.st_ino;
                    feat.size = st.st_size;
                    feat.is_ksu_overlay = true;
                    features.push_back(feat);
                    found = true;
                }
                break;
            }
        }
    }

    return found;
}

bool KernelSUOverlayDetector::detectKsuModule() {
    auto modules = SyscallUtils::listFilesDirect("/data/adb/modules");
    for (const auto& mod : modules) {
        std::string lower = mod;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("ksu") != std::string::npos ||
            lower.find("kernelsu") != std::string::npos) {
            LOGW("检测到 KernelSU 模块: %s", mod.c_str());
            return true;
        }
    }

    if (SyscallUtils::fileExistsDirect("/data/adb/ksu")) {
        LOGW("检测到 KernelSU 目录: /data/adb/ksu");
        return true;
    }

    std::string version = readFile("/proc/version");
    if (!version.empty()) {
        std::string lower = version;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("kernelsu") != std::string::npos ||
            lower.find("ksu") != std::string::npos) {
            LOGW("内核版本包含 KernelSU 标记: %s", version.c_str());
            return true;
        }
    }

    return false;
}

KernelSUOverlayDetectionDetail KernelSUOverlayDetector::detectDetailed() {
    KernelSUOverlayDetectionDetail detail;
    detail.has_ksu_overlay = false;
    detail.has_ksu_private_params = false;
    detail.has_ksu_image_inode = false;
    detail.has_ksu_module = false;

    LOGI("=== KernelSU overlayfs 伪装检测 ===");

    if (detectKsuOverlayMounts(detail.overlay_entries)) {
        detail.has_ksu_overlay = true;
        detail.findings.push_back("检测到 KernelSU overlay 劫持挂载 (" +
                                  std::to_string(detail.overlay_entries.size()) + " 处)");
    }

    if (detectKsuPrivateParams(detail.private_params)) {
        detail.has_ksu_private_params = true;
        detail.findings.push_back("检测到 KernelSU 私有挂载参数");
    }

    if (detectKsuImageInode(detail.image_features)) {
        detail.has_ksu_image_inode = true;
        detail.findings.push_back("检测到 KernelSU overlay 镜像文件 (inode 特征)");
    }

    if (detectKsuModule()) {
        detail.has_ksu_module = true;
        detail.findings.push_back("检测到 KernelSU 模块/目录/内核标记");
    }

    return detail;
}

DetectionResult KernelSUOverlayDetector::detect() {
    DetectionResult result;
    result.layer = "对抗隐藏-KernelSU overlayfs伪装探针";
    result.detected = false;

    auto detail = detectDetailed();

    if (detail.has_ksu_overlay || detail.has_ksu_private_params ||
        detail.has_ksu_image_inode || detail.has_ksu_module) {
        result.detected = true;
        std::string summary;
        for (size_t i = 0; i < detail.findings.size(); i++) {
            if (i > 0) summary += "; ";
            summary += detail.findings[i];
        }
        result.detail = "KernelSU 异常: " + summary;
    } else {
        result.detail = "未发现 KernelSU overlayfs 伪装特征";
    }

    return result;
}
