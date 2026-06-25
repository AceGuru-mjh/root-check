#ifndef ROOTDETECTOR_THREAD_STACK_DETECTOR_H
#define ROOTDETECTOR_THREAD_STACK_DETECTOR_H

#include "property_detector.h"
#include <string>
#include <vector>

struct StackFrameInfo {
    std::string library;
    unsigned long address;
    unsigned long offset;
    bool is_suspicious;
    std::string suspicious_reason;
};

struct StackBacktraceResult {
    bool detected;
    std::string layer;
    std::string detail;
    std::vector<StackFrameInfo> frames;
    int total_threads_scanned;
    int suspicious_threads;
    std::vector<std::string> findings;
};

class ThreadStackDetector {
public:
    static StackBacktraceResult detect();
    static StackBacktraceResult detectForPid(pid_t pid);

private:
    static std::vector<pid_t> getThreadTids(pid_t pid);
    static std::vector<StackFrameInfo> readStackForTid(pid_t tid);
    static bool isStackFrameHook(const StackFrameInfo &frame);
    static bool readProcMaps(pid_t pid, std::vector<std::pair<unsigned long, std::string>> &maps);
};

#endif
