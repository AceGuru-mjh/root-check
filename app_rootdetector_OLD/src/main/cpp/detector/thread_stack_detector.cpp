#include "thread_stack_detector.h"
#include <thread>
#include <chrono>
#include <sstream>
#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <cstring>

std::vector<pid_t> ThreadStackDetector::getThreadTids(pid_t pid) {
    std::vector<pid_t> tids;
    std::string task_path = "/proc/" + std::to_string(pid) + "/task";
    DIR *dir = opendir(task_path.c_str());
    if (!dir) return tids;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] >= '0' && entry->d_name[0] <= '9') {
            tids.push_back(std::atoi(entry->d_name));
        }
    }
    closedir(dir);
    return tids;
}

std::vector<std::pair<unsigned long, std::string>> ThreadStackDetector::readProcMaps(
    pid_t pid, std::vector<std::pair<unsigned long, std::string>> &maps) {
    std::string maps_path = "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream maps_file(maps_path);
    if (!maps_file.is_open()) return maps;
    std::string line;
    while (std::getline(maps_file, line)) {
        auto dash_pos = line.find('-');
        if (dash_pos == std::string::npos) continue;
        unsigned long start = std::stoul(line.substr(0, dash_pos), nullptr, 16);
        auto space_pos = line.find(' ', dash_pos + 1);
        if (space_pos == std::string::npos) continue;
        auto path_start = line.find('/');
        std::string lib;
        if (path_start != std::string::npos) {
            lib = line.substr(path_start);
        }
        maps.emplace_back(start, lib);
    }
    return maps;
}

bool ThreadStackDetector::isStackFrameHook(const StackFrameInfo &frame) {
    const std::vector<std::string> hook_libs = {
        "libfrida", "frida-agent", "libgadget", "libinject",
        "libxposed", "liblspd", "riru", "libwhale",
        "libsubstrate", "libcycript", "libhook"
    };
    for (const auto &hlib : hook_libs) {
        if (frame.library.find(hlib) != std::string::npos) return true;
    }
    if (frame.library.find("/data/data/") != std::string::npos &&
        frame.library.find("lib") != std::string::npos) return true;
    if (frame.library.empty() && frame.address > 0x700000000000UL) return true;
    return false;
}

std::vector<StackFrameInfo> ThreadStackDetector::readStackForTid(pid_t tid) {
    std::vector<StackFrameInfo> frames;
    std::string stack_path = "/proc/" + std::to_string(tid) + "/stack";
    std::ifstream stack_file(stack_path);
    if (!stack_file.is_open()) return frames;
    std::string line;
    while (std::getline(stack_file, line)) {
        StackFrameInfo frame;
        auto addr_end = line.find(' ');
        if (addr_end == std::string::npos) continue;
        frame.address = std::stoul(line.substr(0, addr_end), nullptr, 16);
        auto func_start = line.find_last_of(' ');
        if (func_start != std::string::npos) {
            frame.library = line.substr(func_start + 1);
        }
        frame.is_suspicious = isStackFrameHook(frame);
        frames.push_back(frame);
    }
    return frames;
}

StackBacktraceResult ThreadStackDetector::detect() {
    return detectForPid(getpid());
}

StackBacktraceResult ThreadStackDetector::detectForPid(pid_t pid) {
    StackBacktraceResult result;
    result.detected = false;
    result.layer = "ThreadStack";
    result.total_threads_scanned = 0;
    result.suspicious_threads = 0;

    std::vector<pid_t> tids = getThreadTids(pid);
    result.total_threads_scanned = (int)tids.size();

    for (pid_t tid : tids) {
        std::vector<StackFrameInfo> frames = readStackForTid(tid);
        for (auto &frame : frames) {
            if (frame.is_suspicious) {
                result.detected = true;
                result.suspicious_threads++;
                std::string finding = "TID " + std::to_string(tid) +
                    ": hook in " + frame.library +
                    " @ " + std::to_string(frame.address);
                result.findings.push_back(finding);
                result.frames.push_back(frame);
            }
        }
    }

    if (result.detected) {
        result.detail = "Found " + std::to_string(result.suspicious_threads) +
                        " suspicious thread stacks";
    } else {
        result.detail = "No hook stack frames detected";
    }

    return result;
}
