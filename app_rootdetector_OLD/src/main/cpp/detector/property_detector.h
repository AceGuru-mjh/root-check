#ifndef ROOTDETECTOR_PROPERTY_DETECTOR_H
#define ROOTDETECTOR_PROPERTY_DETECTOR_H

#include <string>
#include <vector>
#include <utility>

struct DetectionResult {
    bool detected;
    std::string layer;
    std::string detail;
};

class PropertyDetector {
public:
    static DetectionResult detect();

private:
    static std::vector<std::string> getRootKeywords();
    static std::vector<std::pair<std::string, std::string>> readProperties();
    static bool containsKeyword(const std::string& text,
                                const std::vector<std::string>& keywords,
                                std::string& matchedKeyword);
};

#endif // ROOTDETECTOR_PROPERTY_DETECTOR_H
