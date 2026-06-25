#ifndef ROOTDETECTOR_MOUNT_DETECTOR_H
#define ROOTDETECTOR_MOUNT_DETECTOR_H

#include "property_detector.h"

class MountDetector {
public:
    static DetectionResult detectNamespaceManipulation();
};

#endif // ROOTDETECTOR_MOUNT_DETECTOR_H
