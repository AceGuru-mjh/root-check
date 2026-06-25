#ifndef ROOTDETECTOR_ART_DETECTOR_H
#define ROOTDETECTOR_ART_DETECTOR_H

#include "property_detector.h"

class ArtDetector {
public:
    static DetectionResult detect();

private:
    static bool checkSuspiciousDexSources();
    static bool checkArtHookSignatures();
    static bool checkXposedFrameworks();
};

#endif // ROOTDETECTOR_ART_DETECTOR_H
