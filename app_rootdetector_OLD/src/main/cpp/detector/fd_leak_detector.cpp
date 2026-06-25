#include "fd_leak_detector.h"
#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <cstring>

FdLeakResult FdLeakDetector::detect() {
    return detectForPid(getpid());
}

FdLeakResult FdLeakDetector::detectForPid(pid_t pid) {
    FdLeakResult result;
    result.detected = false;
    result.layer = "FdLeakDetector";
    result.total_fds = 0;
    result.suspicious_fds = 0;

    result.all_fds = readProcFd(pid);
    result.total_fds = (int)result.all_fds.size();

    for (auto &fd : result.all_fds) {
        if (fd.is_suspicious) {
            result.suspicious_fds++;
            result.detected = true;
            result.findings.push_back("FD " + std::to_string(fd.fd) + " -> " + fd.target);
        }
    }

    if (result.detected) {
        result.detail = "Found " + std::to_string(result.suspicious_fds) +
                        " suspicious file descriptors out of " +
                        std::to_string(result.total_fds);
    } else {
        result.detail = "No suspicious file descriptors found";
    }

    return result;
}

bool FdLeakDetector::isSuspiciousFd(const FdInfo &fd_info) {
    const std::string &target = fd_info.target;

    if (target.find("/data/adb/") != std::string::npos) {
        fd_info.is_hidden_adb = true;
        fd_info.suspicious_reason = "hidden adb access: " + target;
        return true;
    }

    if (target.find(".img") != std::string::npos ||
        target.find(".ext4") != std::string::npos ||
        target.find(".erofs") != std::string::npos) {
        fd_info.is_image_file = true;
        fd_info.suspicious_reason = "image file: " + target;
        return true;
    }

    if (target.find("/dev/block") != std::string::npos) {
        fd_info.suspicious_reason = "block device access: " + target;
        return true;
    }

    if (target.find("anon_inode:")) {
        return false;
    }

    if (target.find("/data/local/tmp") != std::string::npos &&
        (target.find("magisk") != std::string::npos ||
         target.find("ksu") != std::string::npos ||
         target.find("apatch") != std::string::npos)) {
        fd_info.suspicious_reason = "root tool tmp file: " + target;
        return true;
    }

    return false;
}

std::vector<FdInfo> FdLeakDetector::readProcFd(pid_t pid) {
    std::vector<FdInfo> fds;
    std::string fd_path = "/proc/" + std::to_string(pid) + "/fd";
    DIR *dir = opendir(fd_path.c_str());
    if (!dir) return fds;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] < '0' || entry->d_name[0] > '9') continue;

        FdInfo fd_info;
        fd_info.fd = std::atoi(entry->d_name);
        fd_info.is_hidden_adb = false;
        fd_info.is_image_file = false;
        fd_info.is_suspicious = false;

        std::string link_path = fd_path + "/" + entry->d_name;
        char buf[512];
        ssize_t len = readlink(link_path.c_str(), buf, sizeof(buf) - 1);
        if (len > 0) {
            buf[len] = '\0';
            fd_info.target = std::string(buf);
            fd_info.is_suspicious = isSuspiciousFd(fd_info);
        } else {
            fd_info.target = "(unreadable)";
        }

        fds.push_back(fd_info);
    }
    closedir(dir);
    return fds;
}
