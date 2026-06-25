#include "file_detector.h"
#include "property_detector.h"
#include "../utils/syscall_utils.h"
#include <fstream>
#include <sstream>
#include <android/log.h>

#define LOG_TAG "FileDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

std::vector<std::string> FileDetector::getSuPaths() {
    return {
        "/system/bin/su", "/system/xbin/su", "/system/bin/.ext/su",
        "/system/usr/we-need-root/su-backup", "/system/xbin/mu",
        "/system/etc/init.d/su", "/data/local/su", "/data/local/bin/su",
        "/data/local/xbin/su", "/cache/su", "/cache/.su",
        "/dev/mem", "/dev/kmem", "/dev/ptmx",
        "/system/app/Superuser.apk", "/system/app/SuperSU.apk",
        "/system/app/SuperSU", "/system/app/Superuser",
        "/data/app/Superuser.apk", "/data/app/SuperSU.apk",
        "/data/app/com.noshufou.android.su", "/data/app/eu.chainfire.supersu",
        "/data/app/com.koushikdutta.superuser", "/data/app/com.koushikdutta.rommanager",
        "/data/app/com.thirdparty.superuser", "/data/app/com.yellowes.su",
        "/data/app/com.topjohnwu.magisk", "/data/app/com.topjohnwu.magisk.hide",
        "/sbin/su", "/sbin/.magisk", "/sbin/.core",
        "/vendor/bin/su", "/vendor/bin/hw/su",
        "/product/bin/su", "/product/bin/hw/su",
        "/system_ext/bin/su", "/system_ext/bin/hw/su",
        "/odm/bin/su", "/odm/bin/hw/su",
        "/my_product/bin/su", "/my_product/bin/hw/su",
        "/cust/bin/su", "/cust/bin/hw/su",
        "/preas/bin/su", "/preas/bin/hw/su",
        "/spec/bin/su", "/spec/bin/hw/su",
        "/system/bin/.magisk", "/system/bin/.core",
        "/system/xbin/.magisk", "/system/xbin/.core",
        "/data/adb/magisk", "/data/adb/magisk.img",
        "/data/adb/magisk_simple", "/data/adb/magisk_merge.img",
        "/data/adb/modules", "/data/adb/modules.img",
        "/data/adb/post-fs-data.d", "/data/adb/service.d",
        "/data/adb/.magisk", "/data/adb/.core",
        "/data/adb/magisk.db", "/data/adb/magisk/util_functions.sh",
        "/data/adb/magisk/boot.img", "/data/adb/magisk/recovery.img",
        "/data/adb/magisk/uninstall.sh", "/data/adb/magisk/addon.d.sh",
        "/data/adb/magisk/magiskboot", "/data/adb/magisk/magiskinit",
        "/data/adb/magisk/magiskpolicy", "/data/adb/magisk/stub.apk",
        "/data/adb/magisk/zygisk", "/data/adb/magisk/zygisk.img",
        "/data/adb/ksu", "/data/adb/ksud", "/data/adb/ksu.img",
        "/data/adb/ksu/modules", "/data/adb/ksu/modules.img",
        "/data/adb/lspd", "/data/adb/lspd.img",
        "/data/adb/riru", "/data/adb/riru.img",
        "/data/adb/shamiko", "/data/adb/shamiko.img",
        "/dev/socket/magisk", "/dev/socket/magiskd",
        "/dev/socket/magisk_log", "/dev/socket/magisk_pipe",
        "/dev/socket/zygisk", "/dev/socket/zygisk_pipe",
        "/dev/socket/lsposed", "/dev/socket/lspd",
        "/dev/socket/riru", "/dev/socket/riru_pipe",
        "/dev/.magisk", "/dev/.core", "/dev/.magisk.unblock",
        "/dev/.magisk.lock", "/dev/.magisk.log", "/dev/.magisk.tmp",
        "/dev/.magisk.db", "/dev/.magisk.img", "/dev/.magisk.mnt",
        "/dev/.magisk.pid", "/dev/.magisk.sock", "/dev/.magisk.stat",
        "/dev/.magisk.ver", "/dev/.magisk.wd", "/dev/.magisk.wlock",
        "/dev/.magisk.wpid", "/dev/.magisk.wsock", "/dev/.magisk.wstat",
        "/dev/.magisk.wver", "/dev/.magisk.wwwd", "/dev/.magisk.wwwlock",
        "/dev/.magisk.wwwpid", "/dev/.magisk.wwwsock", "/dev/.magisk.wwwstat",
        "/dev/.magisk.wwwver", "/dev/.magisk.wwwwd", "/dev/.magisk.wwwwlock",
        "/dev/.magisk.wwwwpid", "/dev/.magisk.wwwwsock", "/dev/.magisk.wwwwstat",
        "/dev/.magisk.wwwwver", "/dev/.magisk.wwwwwd", "/dev/.magisk.wwwwwlock",
        "/dev/.magisk.wwwwwpid", "/dev/.magisk.wwwwwsock", "/dev/.magisk.wwwwwstat",
        "/dev/.magisk.wwwwwver"
    };
}

std::vector<std::string> FileDetector::getAdbPaths() {
    return {
        "/data/adb", "/data/adb/magisk", "/data/adb/ksu",
        "/data/adb/lspd", "/data/adb/riru", "/data/adb/shamiko",
        "/data/adb/modules", "/data/adb/post-fs-data.d",
        "/data/adb/service.d", "/data/adb/.magisk", "/data/adb/.core"
    };
}

bool FileDetector::checkKallsymsPermission() {
    // 检查 /proc/kallsyms 权限
    // 正常情况应该是 r--r--r-- (444)
    // 如果被修改为其他权限，说明可能被篡改
    struct stat st;
    if (stat("/proc/kallsyms", &st) == 0) {
        mode_t mode = st.st_mode & 0777;
        if (mode != 0444 && mode != 0400) {
            LOGI("/proc/kallsyms 权限异常: %o", mode);
            return true; // 异常
        }
    }
    return false; // 正常
}

bool FileDetector::checkLoopDevices() {
    // 扫描 loop 设备检测 magisk.img 等镜像挂载
    auto loopFiles = SyscallUtils::listFilesDirect("/dev/block");
    for (const auto& file : loopFiles) {
        if (file.find("loop") != std::string::npos) {
            // 检查 loop 设备的挂载点
            std::string mountInfo = SyscallUtils::readFileDirect("/proc/mounts");
            if (mountInfo.find("magisk") != std::string::npos ||
                mountInfo.find("ksu") != std::string::npos) {
                return true;
            }
        }
    }
    return false;
}

bool FileDetector::checkOverlayfs() {
    // 检测 overlayfs 挂载点（KernelSU 特征）
    std::string mountInfo = SyscallUtils::readFileDirect("/proc/mounts");
    if (mountInfo.find("overlay") != std::string::npos ||
        mountInfo.find("overlayfs") != std::string::npos) {
        // 检查是否有可疑的 overlay 挂载
        if (mountInfo.find("/system") != std::string::npos ||
            mountInfo.find("/vendor") != std::string::npos ||
            mountInfo.find("/product") != std::string::npos) {
            return true;
        }
    }
    return false;
}

DetectionResult FileDetector::detect() {
    DetectionResult result;
    result.layer = "第4层：文件系统检测";
    result.detected = false;
    
    std::vector<std::string> foundPaths;
    
    // 检测 su 路径
    for (const auto& path : getSuPaths()) {
        if (SyscallUtils::fileExistsDirect(path)) {
            foundPaths.push_back(path);
            LOGI("检测到可疑文件: %s", path.c_str());
        }
    }
    
    // 检测 /data/adb 目录
    for (const auto& path : getAdbPaths()) {
        if (SyscallUtils::fileExistsDirect(path)) {
            foundPaths.push_back(path);
            LOGI("检测到 ADB 目录: %s", path.c_str());
        }
    }
    
    // 检测 /proc/kallsyms 权限
    if (checkKallsymsPermission()) {
        foundPaths.push_back("/proc/kallsyms (权限异常)");
    }
    
    // 检测 loop 设备
    if (checkLoopDevices()) {
        foundPaths.push_back("loop 设备 (可疑镜像挂载)");
    }
    
    // 检测 overlayfs
    if (checkOverlayfs()) {
        foundPaths.push_back("overlayfs (KernelSU 特征)");
    }
    
    if (!foundPaths.empty()) {
        result.detected = true;
        result.detail = "发现 " + std::to_string(foundPaths.size()) + " 个可疑路径: ";
        for (size_t i = 0; i < std::min(foundPaths.size(), size_t(5)); ++i) {
            result.detail += foundPaths[i];
            if (i < foundPaths.size() - 1 && i < 4) {
                result.detail += "; ";
            }
        }
        if (foundPaths.size() > 5) {
            result.detail += "...";
        }
    } else {
        result.detail = "未发现可疑文件";
    }
    
    return result;
}
