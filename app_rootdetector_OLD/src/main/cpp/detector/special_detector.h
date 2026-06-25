#ifndef ROOTDETECTOR_SPECIAL_DETECTOR_H
#define ROOTDETECTOR_SPECIAL_DETECTOR_H

#include "property_detector.h"
#include <vector>

struct SpecialDetectionResult {
    std::string toolName;
    bool detected;
    std::vector<std::string> evidences;
};

class SpecialDetector {
public:
    static SpecialDetectionResult detectMagisk();
    static SpecialDetectionResult detectZygisk();
    static SpecialDetectionResult detectKernelSU();
    static SpecialDetectionResult detectLSPosed();
    static SpecialDetectionResult detectShamiko();
    
private:
    static bool checkDirectory(const std::string& path);
    static bool checkMemoryPattern(const std::string& pattern);
    static bool checkSocket(const std::string& socketPath);
    static bool checkKernelVersion(const std::string& keyword);
    static bool checkMapsForPattern(const std::string& pattern);
    static bool checkZygoteInjection();
    static bool checkOverlayfsMounts();
    static bool checkArtHookTraces();
    static bool checkBinderAnomalies();
    static bool checkSelinuxContextChanges();
    static bool checkSyscallHookPatterns();
    static bool checkZygiskModuleList();
};

#endif // ROOTDETECTOR_SPECIAL_DETECTOR_H
