#include "micro_services/engine/service_engine.h"
#include "bare_syscall/syscall_bridge.h"
#include <cstring>

static const char* DANGEROUS_PACKAGES[] = {
    "com.topjohnwu.magisk", "com.topjohnwu.magisk.alpha",
    "com.topjohnwu.magisk.stable", "com.topjohnwu.magisk.canary",
    "me.weishu.kernelsu", "me.weishu.kernelsu.ksud",
    "com.apatch.app", "biz.binary.su", "com.noshufou.android.su",
    "eu.chainfire.supersu", "com.thirdparty.superuser",
    "com.koushikdutta.superuser", "com.zachspong.temprootremovejb",
    "com.ramdroid.appquarantine", "com.devadvance.rootchecker",
    "com.scottyab.rootbeer", "com.jrummyapps.rootchecker",
    "com.jrummyapps.busybox", "stericson.busybox",
    "stericson.busybox.donate", "com.saurik.substrate",
    "com.saurik.substrate.cydif", "de.robv.android.xposed.installer",
    "com.solohsu.xposed.edxposed", "org.meowcat.edxposed",
    "com.lsposed.lsposed", "org.lsposed.lsposed",
    "com.android.vending.billing.InAppBillingService.COIN",
    "com.kimjio.umamusume.enpiya"
};

extern "C" bool init() { return true; }

extern "C" apex::engine::ServiceResult execute(const apex::engine::ScanConfig& config) {
    apex::engine::ServiceResult result;
    result.service_id = 10;
    result.service_name = "Dangerous Apps Scanner";
    result.success = true;
    result.confidence = 1.0f;

    // Read /proc/self/maps to identify loaded suspicious packages
    int fd = static_cast<int>(bs_openat(-100, "/proc/self/maps", 0, 0));
    if (fd < 0) {
        result.description = "Cannot scan app packages";
        return result;
    }

    char buf[8192];
    int64_t n = bs_read(fd, buf, sizeof(buf) - 1);
    bs_close(fd);
    if (n <= 0) return result;
    buf[n] = '\0';

    int found = 0;
    for (size_t i = 0; i < sizeof(DANGEROUS_PACKAGES) / sizeof(DANGEROUS_PACKAGES[0]); i++) {
        if (strstr(buf, DANGEROUS_PACKAGES[i])) {
            found++;
        }
    }

    if (found > 0) {
        result.success = false;
        result.confidence = 0.9f;
        result.description = "Dangerous packages detected: " + std::to_string(found);
    } else {
        result.description = "No dangerous packages found";
    }

    return result;
}

extern "C" void cleanup() {}

static apex::engine::ServicePlugin plugin = {
    10, "Dangerous Apps Scanner", "1.0.0", init, execute, cleanup
};
__attribute__((constructor)) void reg() { apex::engine::service_engine::register_service(plugin); }
