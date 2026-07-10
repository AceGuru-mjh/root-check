#include <jni.h>
#include <android/log.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include "bare_syscall/syscall_bridge.h"
#include "micro_services/engine/service_engine.h"
#include "trusted_root/signing/signing_center.h"

#ifndef CLONE_NEWNS
#define CLONE_NEWNS 0x00020000
#endif

#define LOG_TAG "ApexRoot-JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    (void)vm;
    LOGI("APEX Root v1.0.7 native library loaded (20-layer + parallel engine + scroll/crash fixes)");
    return JNI_VERSION_1_6;
}

// ─────────────────────────────────────────────────────────────
// Sandbox bootstrap (hardened).
//
// Previous implementation had three issues:
//   1. bs_unshare() return value was discarded — if CLONE_NEWNS failed,
//      the child kept running in the parent namespace, defeating isolation.
//   2. service_engine::initialize() failure was ignored — the child then
//      entered an infinite nanosleep loop, becoming a zombie that the
//      parent believed was a working sandbox.
//   3. The parent assumed success immediately after fork() — Java side
//      then tried to talk to a half-booted sandbox.
//
// Fix: use a pipe to synchronise init outcome. The child writes one byte
// (0 = OK, >0 = error code) before sleeping; the parent read()s it. If
// init fails, the child _exit()s and the parent waitpid()s the corpse.
// ─────────────────────────────────────────────────────────────
JNIEXPORT void JNICALL
Java_com_apex_root_ui_MainActivity_nativeStartSandbox(JNIEnv*, jobject) {
    LOGI("Starting sandbox process...");

    int notify_pipe[2];
    if (pipe(notify_pipe) != 0) {
        LOGE("pipe() failed: %s", strerror(errno));
        return;
    }

    int pid = static_cast<int>(bs_fork());
    if (pid < 0) {
        LOGE("fork() failed: %s", strerror(errno));
        close(notify_pipe[0]);
        close(notify_pipe[1]);
        return;
    }

    if (pid == 0) {
        // ─── Child ───
        close(notify_pipe[0]);

        auto signal_failure = [&](unsigned char code) {
            ssize_t w = write(notify_pipe[1], &code, 1);
            (void)w;
            close(notify_pipe[1]);
            _exit(1);
        };

        // Create a private mount namespace. If this fails the sandbox is
        // pointless — abort cleanly instead of running in the parent ns.
        if (bs_unshare(CLONE_NEWNS) != 0) {
            LOGE("sandbox: unshare(CLONE_NEWNS) failed: %s", strerror(errno));
            signal_failure(1);
        }

        bool init_ok = false;
        try {
            init_ok = apex::engine::service_engine::initialize();
        } catch (const std::exception& e) {
            LOGE("sandbox: service_engine threw: %s", e.what());
            init_ok = false;
        } catch (...) {
            LOGE("sandbox: service_engine threw unknown exception");
            init_ok = false;
        }
        if (!init_ok) {
            signal_failure(2);
        }

        // Tell the parent we're up.
        unsigned char ok = 0;
        ssize_t w = write(notify_pipe[1], &ok, 1);
        (void)w;
        close(notify_pipe[1]);

        // Event loop. We never abort from here without an external signal.
        while (true) {
            bs_nanosleep(500000000ULL);  // 500ms
        }
    }

    // ─── Parent ───
    close(notify_pipe[1]);

    unsigned char result = 0xFF;
    ssize_t r = read(notify_pipe[0], &result, 1);
    close(notify_pipe[0]);

    if (r != 1) {
        LOGE("Sandbox pipe closed without result — child died during init");
        // Reap the dead child to avoid zombies.
        int status = 0;
        (void)waitpid(pid, &status, 0);
        return;
    }

    if (result == 0) {
        LOGI("Sandbox started successfully with PID %d", pid);
    } else {
        LOGE("Sandbox initialization failed (code=%u) — see logs above", result);
        int status = 0;
        (void)waitpid(pid, &status, 0);
    }
}

JNIEXPORT jint JNICALL
Java_com_apex_root_ui_MainActivity_nativeGetOverallRisk(JNIEnv*, jobject) {
    auto report = apex::signing::signing_center::get_last_report();
    if (!report) return -1;
    return static_cast<jint>(report->overall_risk * 100);
}

} // extern "C"
