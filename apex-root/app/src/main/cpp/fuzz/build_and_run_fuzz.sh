#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════
#  build_and_run_fuzz.sh — Build APEX-Root native fuzz harness
# ----------------------------------------------------------------
#  在 Linux host 上构建 ASAN/UBSAN + libFuzzer 二进制,
#  用于发现 detect/ 解析路径的内存安全 bug。
#
#  不需要 Android NDK。inline arm64 syscall 不可用,所以
#  read_file_at / read_maps 会返回 false —— fuzz target
#  绕过这些,直接喂内存中的 buffer 给解析逻辑。
#
#  使用:
#    cd apex-root/app/src/main/cpp
#    bash fuzz/build_and_run_fuzz.sh
#
#  需要:
#    - clang++ (>= 14) with libFuzzer
#    - Ubuntu: sudo apt install clang llvm
# ═══════════════════════════════════════════════════════════
set -euo pipefail

CPP_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$CPP_DIR"

CXX=${CXX:-clang++}
FUZZ_FLAGS="-std=c++20 -fsanitize=address,undefined,fuzzer-no-link -g -O1 -fno-omit-frame-pointer"
LINK_FLAGS="-fsanitize=address,undefined,fuzzer"
INCLUDES=(
    -I.
    -Iinclude
    -Icommon
    -Ibare_syscall
    -Idetect
    -Iebpf
    -Icure
    -Iguard
    -Igame
    -Ihid
    -Inamespace
    -Imicro_services
    -Imicro_services/engine
    -Itrusted_root
    -Ikernel_observer
    -Iconsensus
    -Iai_engine
    -Iupdater
    -I../../../ctrl
    -I../../../legacy
)
DEFINES=(-D__aarch64__ -DAPEX_FUZZ_HOST)

echo "=== Building fuzz harness with $CXX ==="
echo "CXX flags: $FUZZ_FLAGS"

# Build a minimal subset of source files that the fuzz harness exercises.
# We don't build the full apex_root lib because most of it depends on
# baresyscall (arm64 inline asm) and JNI symbols.
SOURCES=(
    fuzz/fuzz_targets.cpp
)

OUTPUT="fuzz_apex"

$CXX $FUZZ_FLAGS "${INCLUDES[@]}" "${DEFINES[@]}" \
    -include fuzz/stub_jni.h \
    "${SOURCES[@]}" \
    $LINK_FLAGS \
    -o "$OUTPUT"

echo "=== Built $OUTPUT ==="
echo ""
echo "Run fuzzing with:"
echo "  ./$OUTPUT -max_len=65536 -max_total_time=300 -jobs=4 -workers=4"
echo ""
echo "Reproduce a crash:"
echo "  ./$OUTPUT crash-<sha>"
echo ""
echo "Minimize a corpus:"
echo "  ./$OUTPUT -merge=1 minimized_corpus seed_corpus/"

# Create a seed corpus directory with a few interesting inputs
mkdir -p seed_corpus
# Empty input
: > seed_corpus/empty
# A normal /proc/self/maps line
printf '12c00000-12c10000 r-xp 00000000 fd:00 1234 /system/lib/libfoo.oat\n' > seed_corpus/normal_line
# A line with no trailing newline (previous bug trigger)
printf '12c00000-12c10000 rwxp 00000000 fd:00 1234 [anon:dalvik-jit-code-cache]' > seed_corpus/no_trailing_newline
# Lots of .oat entries (counting loop stress test)
for i in $(seq 1 100); do
    printf 'addr-%d r-xp 0 0 0 /x.oat\n' "$i"
done > seed_corpus/many_oats
# A buffer that's all nulls (edge case)
head -c 4096 /dev/zero > seed_corpus/all_zero
# A buffer with no .oat/.dex at all
head -c 8192 /dev/urandom | tr -dc 'a-zA-Z0-9 \n' > seed_corpus/random_ascii

echo "Seed corpus created in seed_corpus/"
echo ""
echo "Quick smoke run (10 seconds):"
echo "  ./$OUTPUT seed_corpus/ -max_total_time=10"
