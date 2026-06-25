#ifndef ROOTDETECTOR_FD_LEAK_DETECTOR_H
#define ROOTDETECTOR_FD_LEAK_DETECTOR_H

#include "property_detector.h"
#include <string>
#include <vector>

struct FdInfo {
    int fd;
    std::string target;
    bool is_hidden_adb;
    bool is_image_file;
    bool is_suspicious;
    std::string suspicious_reason;
};

struct FdLeakResult {
    bool detected;
    std::string layer;
    std::string detail;
    std::vector<FdInfo> all_fds;
    int total_fds;
    int suspicious_fds;
    std::vector<std::string> findings;
};

class FdLeakDetector {
public:
    static FdLeakResult detect();
    static FdLeakResult detectForPid(pid_t pid);

private:
    static std::vector<FdInfo> readProcFd(pid_t pid);
    static bool isSuspiciousFd(const FdInfo &fd);
};

#endif
