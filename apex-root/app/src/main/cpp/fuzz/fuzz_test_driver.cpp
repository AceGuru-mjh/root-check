// ═══════════════════════════════════════════════════════════
//  Standalone test driver for fuzz targets (no libFuzzer required)
// ----------------------------------------------------------------
//  This file provides a main() that exercises the same code paths
//  as fuzz_targets.cpp but with deterministic inputs, so the test
//  can be run with any C++20 compiler (g++ or clang++).
//
//  Build (with ASAN/UBSAN for max coverage):
//    g++ -std=c++20 -fsanitize=address,undefined -g \
//        -I. -Iinclude -Icommon -Ibare_syscall -Idetect \
//        -include fuzz/stub_jni.h -DAPEX_FUZZ_HOST \
//        fuzz/fuzz_test_driver.cpp \
//        -o fuzz_test_driver
//    ./fuzz_test_driver
//
//  If any of the deterministic inputs trigger a sanitizer error,
//  the binary will exit non-zero with an ASAN/UBSAN report.
// ═══════════════════════════════════════════════════════════

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include "stub_jni.h"

// Forward declaration of the fuzz entry point (defined in fuzz_targets.cpp).
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

namespace {

struct TestCase {
    const char* name;
    const uint8_t* data;
    size_t size;
};

// Helper to build a test case from a string literal
#define MAKE_CASE(name, str) \
    { name, reinterpret_cast<const uint8_t*>(str), sizeof(str) - 1 }

int g_pass_count = 0;
int g_fail_count = 0;

void run_test(const TestCase& tc) {
    printf("  [RUN ] %s (%zu bytes) ... ", tc.name, tc.size);
    fflush(stdout);
    int r = LLVMFuzzerTestOneInput(tc.data, tc.size);
    if (r == 0) {
        printf("OK\n");
        g_pass_count++;
    } else {
        printf("FAIL (return %d)\n", r);
        g_fail_count++;
    }
}

} // namespace

int main() {
    printf("=== APEX-Root fuzz target test driver ===\n");
    printf("Exercising previously-buggy parsers with deterministic inputs.\n");
    printf("If any of these crashes, ASAN/UBSAN will report the bug.\n\n");

    // ─── Test cases targeting each previously-fixed bug ───

    // Bug 1: strstr returning nullptr → nullptr+4 deref
    {
        static const char* buf1 = "no oat or dex here at all, just plain text";
        run_test(MAKE_CASE("no_oat_no_dex", buf1));
    }
    // Bug 2: *nl = '\0' writing past buffer end when no trailing newline
    {
        static const char* buf2 = "12c00000-12c10000 rwxp 00000000 fd:00 1234 [anon:dalvik-jit-code-cache]";
        run_test(MAKE_CASE("no_trailing_newline_rwxp", buf2));
    }
    // Bug 3: getdents64 with d_reclen == 0 → infinite loop
    {
        struct __attribute__((packed)) BadDirent {
            uint64_t d_ino;
            int64_t d_off;
            uint16_t d_reclen;  // we'll set to 0
            uint8_t d_type;
            char d_name[4];
        };
        static BadDirent bad = { 1, 0, 0, 4, { '1', '2', '3', '\0' } };
        run_test({ "dirent_zero_reclen",
                   reinterpret_cast<const uint8_t*>(&bad),
                   sizeof(bad) });
    }
    // Bug 4: getdents64 with d_reclen > remaining → OOB read
    {
        struct __attribute__((packed)) BadDirent {
            uint64_t d_ino;
            int64_t d_off;
            uint16_t d_reclen;  // we'll set to 9999
            uint8_t d_type;
            char d_name[4];
        };
        static BadDirent bad = { 1, 0, 9999, 4, { '1', '2', '3', '\0' } };
        run_test({ "dirent_huge_reclen",
                   reinterpret_cast<const uint8_t*>(&bad),
                   sizeof(bad) });
    }
    // Edge: empty input
    run_test({ "empty", reinterpret_cast<const uint8_t*>(""), 0 });

    // Edge: 1 byte
    {
        static const uint8_t b = 'x';
        run_test({ "one_byte", &b, 1 });
    }

    // Stress: large buffer with many .oat entries
    {
        static char big[65536];
        size_t pos = 0;
        for (int i = 0; i < 1000 && pos + 32 < sizeof(big); i++) {
            pos += snprintf(big + pos, sizeof(big) - pos,
                "addr%d r-xp 0 0 0 /system/lib/libfoo.oat\n", i);
        }
        run_test({ "many_oats_1000",
                   reinterpret_cast<const uint8_t*>(big), pos });
    }

    // Stress: 64KB of random-looking data
    {
        static uint8_t rand_buf[65536];
        // Pseudo-random deterministic seed
        uint32_t state = 0xdeadbeef;
        for (size_t i = 0; i < sizeof(rand_buf); i++) {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            rand_buf[i] = (uint8_t)(state & 0xff);
        }
        run_test({ "random_64kb", rand_buf, sizeof(rand_buf) });
    }

    // Edge: all nulls
    {
        static uint8_t nulls[4096];
        memset(nulls, 0, sizeof(nulls));
        run_test({ "all_zero_4kb", nulls, sizeof(nulls) });
    }

    // Realistic /proc/self/maps blob
    {
        static const char* realistic =
            "12c00000-12c10000 r-xp 00000000 fd:00 1234   /system/lib/libfoo.oat\n"
            "12c10000-12c20000 r--p 00001000 fd:00 1234   /system/lib/libfoo.oat\n"
            "12c20000-12c30000 rw-p 00002000 fd:00 1234   /system/lib/libfoo.oat\n"
            "70000000-70010000 rwxp 00000000 00:00 0      [anon:dalvik-jit-code-cache]\n"
            "70010000-70020000 r-xp 00000000 fd:00 5678   /apex/com.android.art/lib64/libart.so\n"
            "70020000-70030000 rw-p 00000000 00:00 0      [heap]\n"
            "7ffffff000-800000000 rw-p 00000000 00:00 0   [stack]\n";
        run_test(MAKE_CASE("realistic_maps", realistic));
    }

    printf("\n=== Results: %d passed, %d failed ===\n", g_pass_count, g_fail_count);
    return g_fail_count == 0 ? 0 : 1;
}
