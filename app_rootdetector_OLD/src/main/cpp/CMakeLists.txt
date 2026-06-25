cmake_minimum_required(VERSION 3.22.1)
project("rootguard" C CXX ASM)

# ====================================================================
# 全域编译选项 — 逆向免疫：PIE/PIC/RELRO/STACK/NX
# ====================================================================
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(
    -Wall -Wextra -Werror=implicit-function-declaration
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -fno-rtti -fno-exceptions
    -fstack-protector-strong -D_FORTIFY_SOURCE=2
    -DPROTECTION_ENABLED -DANTI_DEBUG_ENABLED
)

set(CMAKE_C_FLAGS_RELEASE "-O2 -s -fdata-sections -ffunction-sections")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -s -fdata-sections -ffunction-sections")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--exclude-libs,ALL")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-pie")

find_library(log-lib log)

# ====================================================================
# 域 D：底层裸系统汇编抽象层（全架构底层支撑，无libc依赖）
# 所有上层模块禁止直接调用 libc 标准函数，统一调用本层汇编封装接口
# ====================================================================
set(BARE_SYSCALL_SRC
    cpp/bare_syscall/syscall_arm64.S       # 纯arm64裸syscall封装
    cpp/bare_syscall/syscall_arm32.S       # arm32兼容层
    cpp/bare_syscall/cpu_control.c          # CPU核心绑定/调频锁定
    cpp/bare_syscall/raw_file_io.c          # 原始mmap/无缓存裸读写
    cpp/bare_syscall/memory_raw.c           # 进程内存裸读写/匿名分配
    cpp/bare_syscall/pmu_counter.c          # PMU硬件性能计数器
    cpp/bare_syscall/page_table_walker.c    # 内核页表遍历原语
    cpp/bare_syscall/interrupt_raw.c        # 中断控制器底层读取
)
add_library(bare_syscall STATIC ${BARE_SYSCALL_SRC})
target_include_directories(bare_syscall PUBLIC cpp/bare_syscall)

# ====================================================================
# 加密原语模块（自研轻量化 AES-GCM + SHA3-512，无第三方依赖）
# ====================================================================
set(CRYPTO_CORE_SRC
    cpp/crypto/core/sha3.c
    cpp/crypto/core/aes_gcm.c
    cpp/crypto/core/crypto_basic.c
    cpp/crypto/core/blake2.c
    cpp/crypto/core/xchacha20.c
)
add_library(crypto_core STATIC ${CRYPTO_CORE_SRC})
target_include_directories(crypto_core PUBLIC cpp/crypto/core)

# ====================================================================
# 全域加密 IPC 通信层（域 A ↔ B ↔ C 共享）
# Unix Socket 通信，禁止 Binder（Binder 易被 LSPosed Hook）
# 所有消息 AES-GCM 加密 + 64bit序列号防重放 + 时间戳校验
# ====================================================================
set(IPC_COMM_SRC
    cpp/ipc/core/ipc_crypto.c
    cpp/ipc/core/ipc_protocol.c
    cpp/ipc/server/unix_socket_server.c
    cpp/ipc/client/unix_socket_client.c
)
add_library(ipc_comm STATIC ${IPC_COMM_SRC})
target_include_directories(ipc_comm PUBLIC
    cpp/ipc
    cpp/ipc/core
    cpp/ipc/server
    cpp/ipc/client
)
target_link_libraries(ipc_comm PUBLIC bare_syscall crypto_core)

# ====================================================================
# 域 B：沙箱检测执行域（微内核插件式 7 层检测引擎）
# 每层检测插件为独立无依赖 .so，按需加载/卸载，单模块崩溃不影响整体
# ====================================================================

# 插件接口头文件路径
set(PLUGIN_INCLUDE_DIR cpp/sandbox_microkernel/plugins)
set(RULE_PARSER_INCLUDE_DIR cpp/sandbox_microkernel/rule_parser)

# --- L1：原始属性裸读插件 ---
# 直接 mmap /dev/__properties__ 绕过 getprop，裸 openat 遍历隐藏 su 路径
add_library(plugin_l1 SHARED
    cpp/sandbox_microkernel/plugins/l1_property/plugin_l1.c
)
target_include_directories(plugin_l1 PRIVATE ${PLUGIN_INCLUDE_DIR})
target_link_libraries(plugin_l1 PRIVATE bare_syscall crypto_core)
set_target_properties(plugin_l1 PROPERTIES OUTPUT_NAME "guard_plugin_01")

# --- L2：ART 虚拟机注入检测插件 ---
# 裸读 /proc/self/maps 扫描匿名可执行内存，libart.so 函数段哈希比对
add_library(plugin_l2 SHARED
    cpp/sandbox_microkernel/plugins/l2_art/plugin_l2.c
)
target_include_directories(plugin_l2 PRIVATE ${PLUGIN_INCLUDE_DIR})
target_link_libraries(plugin_l2 PRIVATE bare_syscall crypto_core)
set_target_properties(plugin_l2 PROPERTIES OUTPUT_NAME "guard_plugin_02")

# --- L3：进程 & 内存指纹扫描插件 ---
# 纯 syscall getdents64 遍历 /proc，/proc/self/mem 逐段匹配 Rootkit 特征
add_library(plugin_l3 SHARED
    cpp/sandbox_microkernel/plugins/l3_memory/plugin_l3.c
)
target_include_directories(plugin_l3 PRIVATE ${PLUGIN_INCLUDE_DIR})
target_link_libraries(plugin_l3 PRIVATE bare_syscall crypto_core)
set_target_properties(plugin_l3 PROPERTIES OUTPUT_NAME "guard_plugin_03")

# --- L4：命名空间挂载对比插件（独家核心） ---
# fork 子线程建独立 mnt 命名空间，父子并行读 mountinfo 差分对比
add_library(plugin_l4 SHARED
    cpp/sandbox_microkernel/plugins/l4_mount/plugin_l4.c
)
target_include_directories(plugin_l4 PRIVATE ${PLUGIN_INCLUDE_DIR})
target_link_libraries(plugin_l4 PRIVATE bare_syscall crypto_core)
set_target_properties(plugin_l4 PROPERTIES OUTPUT_NAME "guard_plugin_04")

# --- L5：侧信道时延分析插件（APatch 专属检测） ---
# 锁 CPU 频率绑核消除调度误差，truncate 纳秒级采样，多轮基线比对
add_library(plugin_l5 SHARED
    cpp/sandbox_microkernel/plugins/l5_sidechannel/plugin_l5.c
)
target_include_directories(plugin_l5 PRIVATE ${PLUGIN_INCLUDE_DIR})
target_link_libraries(plugin_l5 PRIVATE bare_syscall crypto_core)
set_target_properties(plugin_l5 PROPERTIES OUTPUT_NAME "guard_plugin_05")

# --- L6：内核完整性探针插件（竞品空白） ---
# 裸读 /proc/kallsyms 解析 syscall 表基址，内核函数段哈希校验
add_library(plugin_l6 SHARED
    cpp/sandbox_microkernel/plugins/l6_kernel/plugin_l6.c
)
target_include_directories(plugin_l6 PRIVATE ${PLUGIN_INCLUDE_DIR})
target_link_libraries(plugin_l6 PRIVATE bare_syscall crypto_core)
set_target_properties(plugin_l6 PROPERTIES OUTPUT_NAME "guard_plugin_06")

# --- L7：TEE TrustZone 硬件可信旁路插件 ---
# 绕过 KeyStore API，裸读写 /dev/tee /dev/qseecom，硬件基线指纹比对
add_library(plugin_l7 SHARED
    cpp/sandbox_microkernel/plugins/l7_tee/plugin_l7.c
)
target_include_directories(plugin_l7 PRIVATE ${PLUGIN_INCLUDE_DIR})
target_link_libraries(plugin_l7 PRIVATE bare_syscall crypto_core)
set_target_properties(plugin_l7 PROPERTIES OUTPUT_NAME "guard_plugin_07")

# --- L8：Magisk 专检插件 ---
# Magisk 进程/套接字/启动分区特征扫描
add_library(plugin_l8 SHARED
    cpp/sandbox_microkernel/plugins/specialized/layer8_magisk.c
)
target_include_directories(plugin_l8 PRIVATE ${PLUGIN_INCLUDE_DIR})
target_link_libraries(plugin_l8 PRIVATE bare_syscall crypto_core)
set_target_properties(plugin_l8 PROPERTIES OUTPUT_NAME "guard_plugin_08")

# --- L9：KernelSU 专检插件 ---
# KSU overlayfs/文件/内核符号/模块检测
add_library(plugin_l9 SHARED
    cpp/sandbox_microkernel/plugins/specialized/layer9_ksu.c
)
target_include_directories(plugin_l9 PRIVATE ${PLUGIN_INCLUDE_DIR})
target_link_libraries(plugin_l9 PRIVATE bare_syscall crypto_core)
set_target_properties(plugin_l9 PROPERTIES OUTPUT_NAME "guard_plugin_09")

# --- L10：APatch 专检插件 ---
# APatch ftruncate 侧信道/文件/kallsyms 掩码检测
add_library(plugin_l10 SHARED
    cpp/sandbox_microkernel/plugins/specialized/layer10_apatch.c
)
target_include_directories(plugin_l10 PRIVATE ${PLUGIN_INCLUDE_DIR})
target_link_libraries(plugin_l10 PRIVATE bare_syscall crypto_core)
set_target_properties(plugin_l10 PROPERTIES OUTPUT_NAME "guard_plugin_10")

# --- L11：LSPosed 专检插件 ---
# LSPosed 内存/文件/跨进程注入检测
add_library(plugin_l11 SHARED
    cpp/sandbox_microkernel/plugins/specialized/layer11_lsposed.c
)
target_include_directories(plugin_l11 PRIVATE ${PLUGIN_INCLUDE_DIR})
target_link_libraries(plugin_l11 PRIVATE bare_syscall crypto_core)
set_target_properties(plugin_l11 PROPERTIES OUTPUT_NAME "guard_plugin_11")

# --- L12：Shamiko 专检插件 ---
# Shamiko SELinux 上下文/隐藏检测/进程扫描
add_library(plugin_l12 SHARED
    cpp/sandbox_microkernel/plugins/specialized/layer12_shamiko.c
)
target_include_directories(plugin_l12 PRIVATE ${PLUGIN_INCLUDE_DIR})
target_link_libraries(plugin_l12 PRIVATE bare_syscall crypto_core)
set_target_properties(plugin_l12 PROPERTIES OUTPUT_NAME "guard_plugin_12")

# --- 微内核主调度器（独立可执行文件，fork 后脱离 UI 进程运行） ---
# 负责：插件动态加载/卸载 / 动态乱序调度 / CPU绑定 / 内存隔离 / 规则解析
add_executable(sandbox_main
    cpp/sandbox_microkernel/sandbox_main.c
    cpp/sandbox_microkernel/micro_kernel.c
    cpp/sandbox_microkernel/rule_parser/rule_parser.c
    cpp/sandbox_microkernel/rule_parser/custom_template.c
)
target_include_directories(sandbox_main PRIVATE
    cpp/sandbox_microkernel
    ${PLUGIN_INCLUDE_DIR}
    ${RULE_PARSER_INCLUDE_DIR}
)
target_link_libraries(sandbox_main
    bare_syscall crypto_core ipc_comm
    plugin_l1 plugin_l2 plugin_l3 plugin_l4
    plugin_l5 plugin_l6 plugin_l7 plugin_l8
    plugin_l9 plugin_l10 plugin_l11 plugin_l12
    ${log-lib}
)
# 标记为可执行文件，不安装到默认 JNI lib 目录
set_target_properties(sandbox_main PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/../bin"
)

# ====================================================================
# 域 C：可信安全守护域（常驻独立守护进程，信任根）
# 独立 fork 进程，最高优先级，不受 UI/沙箱生命周期影响
# 模块：内存校验 / 反调试 / 全局验签 / 内核探针 / 自防护
# ====================================================================
add_executable(trusted_daemon
    cpp/trusted_daemon/daemon_main.c
    cpp/trusted_daemon/memory_checker.c
    cpp/trusted_daemon/anti_debug.c
    cpp/trusted_daemon/sign_center.c
    cpp/trusted_daemon/kernel_probe.c
    cpp/trusted_daemon/self_verify.c
    cpp/trusted_daemon/instruction_obfuscator.c
    cpp/trusted_daemon/anti_hook.c
    cpp/trusted_daemon/random_layout.c
    cpp/trusted_daemon/triple_arbiter.c
    cpp/trusted_daemon/log_center.c
    cpp/trusted_daemon/dir_isolation.c
)
target_include_directories(trusted_daemon PRIVATE
    cpp/trusted_daemon
    cpp/sandbox_microkernel/plugins
)
target_link_libraries(trusted_daemon bare_syscall crypto_core ipc_comm ${log-lib})
set_target_properties(trusted_daemon PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/../bin"
)

# ====================================================================
# 域 A ↔ B 桥接：JNI 库（UI 进程 -> 加密 Socket -> 沙箱进程）
# 仅封装加密 Socket 通信，零检测逻辑，无 JNI 攻击面
# ====================================================================
add_library(rootguard_jni SHARED
    cpp/jni/jni_bridge.cpp
)
target_include_directories(rootguard_jni PRIVATE
    cpp
    cpp/ipc
)
target_link_libraries(rootguard_jni ipc_comm crypto_core ${log-lib})

# ====================================================================
# 向后兼容层：旧版 rootdetector 库（当新架构守护进程不可用时降级）
# ====================================================================
add_library(rootdetector SHARED
    jni_bridge.cpp
    detector/property_detector.cpp
    detector/file_detector.cpp
    detector/memory_detector.cpp
    detector/memory_fingerprint_detector.cpp
    detector/mount_detector.cpp
    detector/apatch_detector.cpp
    detector/selinux_detector.cpp
    detector/selinux_jump_detector.cpp
    detector/kernel_detector.cpp
    detector/kernel_module_detector.cpp
    detector/self_protection.cpp
    detector/process_detector.cpp
    detector/syscall_timing_detector.cpp
    detector/art_detector.cpp
    detector/special_detector.cpp
    detector/shamiko_detector.cpp
    detector/zygisknext_detector.cpp
    detector/apatch_kpm_detector.cpp
    detector/kernelsu_overlay_detector.cpp
    detector/lsposed_detector.cpp
    detector/interrupt_timing_detector.cpp
    detector/cache_timing_detector.cpp
    detector/binder_latency_detector.cpp
    detector/pagefault_monitor.cpp
    detector/firmware_detector.cpp
    detector/thread_stack_detector.cpp
    detector/socket_scanner.cpp
    detector/fd_leak_detector.cpp
    detector/hardware_id.cpp
    detector/snapshot.cpp
    detector/perf_monitor.cpp
    detector/cloud_sync.cpp
    detector/cpu_baseline_db.cpp
)
target_include_directories(rootdetector PRIVATE . detector utils)
target_link_libraries(rootdetector crypto_core bare_syscall ${log-lib})

# ====================================================================
# 验证工具（仅在 VERIFICATION_BUILD=ON 时编译）
# ====================================================================
option(VERIFICATION_BUILD "Build verification/debug tools" OFF)
if(VERIFICATION_BUILD)
    add_executable(apatch_sidechannel verification/apatch_sidechannel.c)
    target_link_libraries(apatch_sidechannel ${log-lib} m)

    add_executable(namespace_check verification/namespace_check.c)
    target_link_libraries(namespace_check ${log-lib} rt)

    add_executable(direct_syscall verification/direct_syscall.c)
    target_link_libraries(direct_syscall ${log-lib} dl)

    add_executable(tee_probe verification/tee_probe.c)
    target_link_libraries(tee_probe ${log-lib})
endif()
