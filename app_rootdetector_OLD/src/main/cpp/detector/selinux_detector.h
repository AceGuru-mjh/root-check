#ifndef ROOTDETECTOR_SELINUX_DETECTOR_H
#define ROOTDETECTOR_SELINUX_DETECTOR_H

#include "property_detector.h"

class SelinuxDetector {
public:
    static DetectionResult detect();
};

#endif // ROOTDETECTOR_SELINUX_DETECTOR_H
