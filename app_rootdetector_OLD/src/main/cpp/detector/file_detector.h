#ifndef ROOTDETECTOR_FILE_DETECTOR_H
#define ROOTDETECTOR_FILE_DETECTOR_H

#include <string>
#include <vector>

struct DetectionResult;

class FileDetector {
public:
    static DetectionResult detect();
    
private:
    static std::vector<std::string> getSuPaths();
    static std::vector<std::string> getAdbPaths();
    static bool checkKallsymsPermission();
    static bool checkLoopDevices();
    static bool checkOverlayfs();
};

#endif // ROOTDETECTOR_FILE_DETECTOR_H
