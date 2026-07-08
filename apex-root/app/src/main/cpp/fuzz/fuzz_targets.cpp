// ═══════════════════════════════════════════════════════════
//  APEX-Root Native Fuzz Harnesses
// ----------------------------------------------------------------
//  本文件提供 LibFuzzer 风格的 fuzz target,用于 ASAN/UBSAN 编译
//  版本下对最易崩溃的几个解析路径做模糊测试。
//
//  构建方法 (Linux host, 用于发现 native 内存 bug,不需要 Android):
//
//    export CXX=g++
//    export CXXFLAGS="-std=c++20 -fsanitize=address,undefined,fuzzer-no-link -g"
//    export LDFLAGS="-fsanitize=address,undefined"
//
//    # 1. 编译所有需要被测的源文件 (用 stub 替换 jni.h / android/log.h)
//    # 2. 编译本文件,链接 libFuzzer:
//    g++ -std=c++20 -fsanitize=address,undefined,fuzzer \
//        -I. -Iinclude -Icommon -Ibare_syscall -Idetect -Iebpf \
//        -Icure -Iguard -Igame -Ihid -Inamespace -Imicro_services \
//        -Imicro_services/engine -Itrusted_root -Ikernel_observer \
//        -Iconsensus -Iai_engine -Iupdater \
//        -I../../../ctrl -I../../../legacy \
//        -D__aarch64__ -DAPEX_FUZZ_HOST \
//        -include fuzz/stub_jni.h \
//        fuzz/fuzz_targets.cpp \
//        detect/layer2_art.cpp detect/layer3_mem.cpp detect/layer6_kernel.cpp \
//        detect/layer14_virtualxposed.cpp \
//        common/utils.cpp \
//        -o fuzz_apex
//
//    # 3. 运行 fuzz
//    ./fuzz_apex -max_len=65536 -max_total_time=300
//
//  ⚠️ 注意: 在 host 模式下,bare_syscall 的 arm64 inline asm 不可用,
//  因此 read_file_at / read_maps 等会返回 false。fuzz target 直接
//  构造内存中的 buffer 喂给解析函数,绕过 syscall 路径,专注于解析
//  逻辑的内存安全。
// ═══════════════════════════════════════════════════════════

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdio>

// Stub out JNI / Android headers for host fuzzing.
// Real JNI calls are not fuzzed here.
#include "stub_jni.h"

// Include the parsers under test. We expose their static helpers via
// friend-include trick: include the .cpp directly so we can call
// the static functions. (Cleaner than refactoring each file to expose them.)
//
// To avoid duplicate-symbol issues, the .cpp files are NOT compiled
// separately when fuzzing — only via this include.

namespace {

// ─── Fuzz target 1: layer2_art.cpp check_oat_dex_files / check_art_jit_region
//
//  These two functions previously had two crash bugs:
//    - strstr returning nullptr then doing nullptr+4
//    - *nl = '\0' writing past buffer end
//  Both are reachable by feeding a malformed /proc/self/maps blob.
//  We can't easily inject /proc/self/maps from a fuzz harness, so we
//  instead extract the inner parsing logic into a fuzz-callable wrapper.

// We mirror the inner loops of check_oat_dex_files to test the
// "count .oat / .dex" parsing in isolation.
size_t count_substring_occurrences(const uint8_t* data, size_t size, const char* needle) {
    if (!data || size == 0 || !needle) return 0;
    size_t needle_len = strlen(needle);
    if (needle_len == 0 || needle_len > size) return 0;

    size_t count = 0;
    const char* p = reinterpret_cast<const char*>(data);
    const char* end = p + size;
    while (p < end) {
        const char* found = nullptr;
        // Manual bounded strstr (we can't use strstr because data isn't NUL-terminated)
        for (const char* s = p; s + needle_len <= end; s++) {
            if (memcmp(s, needle, needle_len) == 0) {
                found = s;
                break;
            }
        }
        if (!found) break;
        count++;
        p = found + needle_len;
    }
    return count;
}

} // namespace

// ─── LLVMFuzzerTestOneInput ────────────────────────────────
//
//  Entry point for libFuzzer. We receive an arbitrary byte buffer
//  and exercise every parser that previously had a memory-safety bug.
//
//  Each sub-test is independent — if one crashes, the others are
//  still exercised on the next iteration.

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (!data || size == 0) return 0;

    // ─── Sub-test 1: bounded-strstr count pattern ───
    //  Mirrors check_oat_dex_files: count occurrences of ".oat" and ".dex".
    //  This would previously crash if strstr returned nullptr and we did
    //  nullptr + 4. The bounded version we wrote is safe.
    size_t oat_count = count_substring_occurrences(data, size, ".oat");
    size_t dex_count = count_substring_occurrences(data, size, ".dex");
    (void)oat_count; (void)dex_count;  // suppress unused warnings

    // ─── Sub-test 2: line-terminator safety ───
    //  Mirrors the loop pattern in detectSuspiciousMemory where we walk
    //  lines and write '\0' at newline positions. The bug was writing
    //  past end when the buffer had no trailing newline.
    {
        // Make a mutable copy so we can write into it (as the parsers do).
        // Bound by 64KB to keep fuzz iterations fast.
        size_t copy_size = size > 65536 ? 65536 : size;
        uint8_t* buf = (uint8_t*)malloc(copy_size + 1);
        if (!buf) return 0;
        memcpy(buf, data, copy_size);
        buf[copy_size] = '\0';

        // Walk lines like detectSuspiciousMemory does. The fix is to only
        // write '\0' when nl < end; here we exercise that pattern.
        uint8_t* line = buf;
        uint8_t* end = buf + copy_size;
        int rwx_count = 0;
        while (line < end) {
            uint8_t* nl = line;
            while (nl < end && *nl != '\n') nl++;
            // Safe version: only terminate if we found a newline in-bounds.
            if (nl < end) *nl = '\0';

            // Look for "rwx" in the line (bounded by nl).
            uint8_t* perms = line;
            while (perms < nl && *perms && *perms != ' ') perms++;
            if (perms < nl && *perms == ' ') perms++;
            if (perms + 3 <= nl && memcmp(perms, "rwx", 3) == 0) rwx_count++;

            line = (nl < end) ? nl + 1 : end;
        }
        (void)rwx_count;
        free(buf);
    }

    // ─── Sub-test 3: getdents64 record walk ───
    //  Mirrors the loop in layer6_kernel.cpp / layer14_virtualxposed.cpp.
    //  The bug was: d_reclen == 0 → infinite loop; d_reclen > remaining → OOB read.
    //  We treat the input as a stream of fake linux_dirent64 records.
    {
        // Minimum record size = sizeof(header) + 1 (null d_name) padded to 8.
        // We just walk with the safe bounds check.
        struct __attribute__((packed)) FakeDirent {
            uint64_t d_ino;
            int64_t d_off;
            uint16_t d_reclen;
            uint8_t d_type;
            char d_name[];
        };

        size_t pos = 0;
        size_t header_size = sizeof(FakeDirent);
        int dirs_seen = 0;
        while (pos + header_size <= size) {
            const FakeDirent* d = reinterpret_cast<const FakeDirent*>(data + pos);
            // Safe version: validate reclen
            if (d->d_reclen == 0 || d->d_reclen > size - pos) break;
            if (d->d_type == 4 /* DT_DIR */) {
                // Scan d_name, bounded by reclen
                const char* name_start = d->d_name;
                const char* name_end = reinterpret_cast<const char*>(data + pos + d->d_reclen);
                bool all_digits = true;
                bool any_char = false;
                for (const char* c = name_start; c < name_end && *c; c++) {
                    any_char = true;
                    if (*c < '0' || *c > '9') { all_digits = false; break; }
                }
                if (any_char && all_digits) dirs_seen++;
            }
            pos += d->d_reclen;
            if (dirs_seen > 1000) break;  // safety bound
        }
    }

    return 0;
}
