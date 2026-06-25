#ifndef ROOTDETECTOR_KERNELSU_OVERLAY_DETECTOR_H
#define ROOTDETECTOR_KERNELSU_OVERLAY_DETECTOR_H

#include "property_detector.h"
#include <vector>
#include <string>

struct OverlayMountEntry {
    std::string source;
    std::string target;
    std::string fstype;
    std::string options;
    std::string lowerdir;
    std::string upperdir;
    std::string workdir;
    bool is_ksu_hijack;
    std::string reason;
};

struct KsuPrivateParam {
    std::string param_name;
    std::string param_value;
    bool is_ksu_specific;
};

struct ImageInodeFeature {
    std::string path;
    unsigned long inode;
    unsigned long size;
    bool is_ksu_overlay;
};

struct KernelSUOverlayDetectionDetail {
    bool has_ksu_overlay;
    bool has_ksu_private_params;
    bool has_ksu_image_inode;
    bool has_ksu_module;
    std::vector<OverlayMountEntry> overlay_entries;
    std::vector<KsuPrivateParam> private_params;
    std::vector<ImageInodeFeature> image_features;
    std::vector<std::string> findings;
};

class KernelSUOverlayDetector {
public:
    static DetectionResult detect();
    static KernelSUOverlayDetectionDetail detectDetailed();

private:
    static bool detectKsuOverlayMounts(std::vector<OverlayMountEntry>& entries);
    static bool detectKsuPrivateParams(std::vector<KsuPrivateParam>& params);
    static bool detectKsuImageInode(std::vector<ImageInodeFeature>& features);
    static bool detectKsuModule();
    static std::string readFile(const std::string& path);
};

#endif
