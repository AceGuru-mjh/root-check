#include "layer12_rom.h"
#include "../common/syscall.h"
#include <cstring>

static bool read_file(const char* path, char* buf, size_t size) {
    int64_t fd;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0));
    if (fd < 0) return false;
    int64_t n;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_read), "r"(fd), "r"(buf), "r"((int64_t)size) : "x0", "x1", "x2", "x8");
    int64_t d;
    asm volatile("mov x8,%1;mov x0,%2;svc #0" : "=r"(d) : "i"(__NR_close),"r"(fd) : "x0","x8");
    if (n <= 0) return false;
    buf[n < (int64_t)size ? n : (int64_t)size-1] = '\0';
    return true;
}

bool detectCustomROM() {
    char buf[8192];
    // Check build properties for custom ROM indicators
    bool found = false;
    if (read_file("/system/build.prop", buf, sizeof(buf))) {
        const char* custom[] = {
            // 经典 ROM
            "ro.custom.build", "ro.mod.version",
            "ro.lineage", "ro.pixelexperience",
            "ro.crdroid", "ro.havoc", "ro.evolution",
            "ro.aospa", "ro.cyanogen", "ro.omni",
            "ro.resurrection", "ro.aosip", "ro.bootleggers",
            "ro.dotui", "ro.stag", "ro.msmxtended",
            "ro.candyrom", "ro.bliss", "ro.xenonhd",
            // 扩充：新增 ROM
            // ArrowOS
            "ro.arrow",
            // RisingOS
            "ro.rising",
            // Project Elixir
            "ro.elixir",
            // PixelOS
            "ro.pixelos",
            // StatiXOS
            "ro.statix",
            // Nameless-AOSP
            "ro.nameless",
            // BananaHF
            "ro.banana",
            // Paranoid Android
            "ro.paranoia",
            // DerpFest
            "ro.derp",
            // AncientOS
            "ro.ancient",
            // CorvusOS
            "ro.corvus",
            // OctaviOS
            "ro.octavi",
            // OmniROM
            "ro.omni.rom",
            // LineageOS fork
            "ro.lineage.build.version",
            // RevengeOS
            "ro.revenge",
            // SuperiorOS
            "ro.superior",
            // Spark-OS
            "ro.spark",
            // LegionOS
            "ro.legion",
            // AICP (Android Ice Cold Project)
            "ro.aicp",
            // CarbonROM
            "ro.carbon",
            // SlimROMs
            "ro.slim",
            // TipsyOS
            "ro.tipsy",
            // AOSiP
            "ro.aosip",
            // Validus
            "ro.validus",
            // Nitrogen OS
            "ro.nitrogen",
            // CypherOS
            "ro.cypher",
            // Nitrogen-OS
            "ro.nitrogen",
            // Potato Open Sauce Project
            "ro.potato",
            // WaveOS
            "ro.wave",
            // ZenX OS
            "ro.zenx",
            // Abstract ROM
            "ro.abstract",
            nullptr
        };
        for (auto s = custom; *s; ++s) {
            if (strstr(buf, *s)) { found = true; break; }
        }
    }

    if (found) return true;

    // 扩充：检查 /vendor/build.prop 和 /product/build.prop
    if (read_file("/vendor/build.prop", buf, sizeof(buf))) {
        if (strstr(buf, "ro.lineage") || strstr(buf, "ro.crdroid") ||
            strstr(buf, "ro.pixelexperience") || strstr(buf, "ro.evolution") ||
            strstr(buf, "ro.arrow") || strstr(buf, "ro.rising")) {
            return true;
        }
    }
    if (read_file("/product/build.prop", buf, sizeof(buf))) {
        if (strstr(buf, "ro.lineage") || strstr(buf, "ro.crdroid") ||
            strstr(buf, "ro.pixelexperience") || strstr(buf, "ro.evolution")) {
            return true;
        }
    }

    // 检查 build.prop 中的 test-keys 标记
    if (read_file("/system/build.prop", buf, sizeof(buf))) {
        if (strstr(buf, "ro.build.tags=test-keys")) return true;
        if (strstr(buf, "ro.build.type=userdebug")) return true;
        if (strstr(buf, "ro.build.type=eng")) return true;
    }

    return false;
}

bool detectSELinuxStatus() {
    char buf[16];
    if (!read_file("/sys/fs/selinux/enforce", buf, sizeof(buf))) return false;
    // 0 = permissive, 1 = enforcing
    return buf[0] == '0';
}

bool detectSystemPartitionIntegrity() {
    // Check if system partition is mounted read-only (as expected)
    char buf[16384];
    if (!read_file("/proc/mounts", buf, sizeof(buf))) return false;

    // Check for rw mounts on system partitions
    const char* sys_parts[] = {"/system", "/vendor", "/product", "/odm"};
    char* line = buf;
    char* end = buf + strlen(buf);
    while (line < end) {
        char* nl = line;
        while (nl < end && *nl != '\n') nl++;
        *nl = '\0';
        for (auto part : sys_parts) {
            if (strstr(line, part) && strstr(line, " rw,")) {
                return true; // System partition writable = tampered
            }
        }
        line = nl + 1;
    }
    return false;
}
