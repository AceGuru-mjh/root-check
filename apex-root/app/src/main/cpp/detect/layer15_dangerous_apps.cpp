#include "layer15_dangerous_apps.h"
#include "../common/syscall.h"
#include <cstring>

// ═══════════════════════════════════════════════════════════
//  第十五层 · 危险应用检测（root 级）
// ----------------------------------------------------------------
//  检测常见的游戏作弊 / 系统篡改工具：
//    - GameGuardian  (内存修改器，类似 CE)
//    - CheatEngine   (Android 移植版)
//    - Lucky Patcher (应用内购破解)
//    - GameKiller    (老式内存修改器)
//    - Freedom       (Play 内购破解)
//    - CreeHack      (内购破解)
//    - SB Game Hacker
// ═══════════════════════════════════════════════════════════

static int64_t check_access(const char* path) {
    int64_t ret;
    ret = apex_check_access(path);
    return ret;
}

static bool check_pkg_exists(const char* const* pkgs) {
    for (auto p = pkgs; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

bool detectGameGuardian() {
    static const char* paths[] = {
        "/data/data/catch_.me_.if_.you_.can_",
        "/data/data/com.aavt.savandica.gltools",
        "/data/data/.gg",
        "/data/data/.gameguardian",
        "/data/user/0/catch_.me_.if_.you_.can_",
        "/data/user/0/com.aavt.savandica.gltools",
        nullptr
    };
    return check_pkg_exists(paths);
}

bool detectCheatEngine() {
    static const char* paths[] = {
        "/data/data/org.cheatengine.ce",
        "/data/data/com.cheatengine.android",
        "/data/data/ce.android",
        "/data/data/.cheatengine",
        "/data/user/0/org.cheatengine.ce",
        nullptr
    };
    return check_pkg_exists(paths);
}

bool detectLuckyPatcher() {
    static const char* paths[] = {
        "/data/data/com.android.protector",
        "/data/data/com.android.vending.billing.InAppBillingService.LUCK",
        "/data/data/.lucky_patcher",
        "/data/data/com.chelpus.lackypatch",
        "/data/data/com.forp.sharedclone",
        "/data/user/0/com.android.vending.billing.InAppBillingService.LUCK",
        nullptr
    };
    return check_pkg_exists(paths);
}

bool detectGameKiller() {
    static const char* paths[] = {
        "/data/data/com.zune.gamekiller",
        "/data/data/cn.gamekiller",
        "/data/data/.gamekiller",
        "/data/user/0/com.zune.gamekiller",
        nullptr
    };
    return check_pkg_exists(paths);
}

bool detectMemoryEditors() {
    // 各类内存修改器
    static const char* paths[] = {
        // Xmodgames
        "/data/data/com.xmodgame",
        "/data/data/.xmodgames",
        // SB Game Hacker
        "/data/data/org.sbgamehacker",
        "/data/data/.sbgamehacker",
        // GameCIH (老牌内存修改器)
        "/data/data/com.cih.gamecih",
        "/data/data/com.cih.gamecih2",
        // GameCIH3
        "/data/data/com.cih.gamecih3",
        // DaxAttack
        "/data/data/com.daxattack",
        // DaxAttack Mobile
        "/data/data/com.daxattack.mobile",
        // Game Hacker
        "/data/data/org.sbtools.gamehacker",
        // XHack
        "/data/data/com.xhack.android",
        nullptr
    };
    return check_pkg_exists(paths);
}

bool detectCrackingTools() {
    // APK 破解 / 反编译工具
    static const char* paths[] = {
        // Freedom (Play 内购破解)
        "/data/data/cc.madkite.freedom",
        "/data/data/.freedom",
        // CreeHack
        "/data/data/com.creehack",
        "/data/data/.creehack",
        // iAP Cracker
        "/data/data/com.iapcracker",
        // LocalIAPStore
        "/data/data/com.julian.localiapstore",
        // AntiLVL (APK 反保护)
        "/data/data/com.famlabs.anti_lvl",
        // AntiBoot signature killer
        "/data/data/com.doomy.sigkill",
        // 大师助手
        "/data/data/com.dashi.zhushou",
        // 八门神器
        "/data/data/com.bm.android",
        "/data/data/cn.bmwm.bmwsq",
        nullptr
    };
    return check_pkg_exists(paths);
}

int dangerousAppsFullScan(char* out_report, size_t out_size) {
    int findings = 0;
    int pos = 0;
    auto append = [&](const char* s) {
        while (*s && pos < (int)out_size - 1) out_report[pos++] = *s++;
    };

    if (detectGameGuardian()) {
        append("[DangerousApp] GameGuardian detected\n"); findings++;
    }
    if (detectCheatEngine()) {
        append("[DangerousApp] CheatEngine detected\n"); findings++;
    }
    if (detectLuckyPatcher()) {
        append("[DangerousApp] Lucky Patcher detected\n"); findings++;
    }
    if (detectGameKiller()) {
        append("[DangerousApp] GameKiller detected\n"); findings++;
    }
    if (detectMemoryEditors()) {
        append("[DangerousApp] Memory editor detected\n"); findings++;
    }
    if (detectCrackingTools()) {
        append("[DangerousApp] Cracking tool detected\n"); findings++;
    }

    if (pos < (int)out_size) out_report[pos] = '\0';
    return findings;
}
