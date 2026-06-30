cmake_minimum_required(VERSION 3.22.1)
project("apex_root" C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ─── liboqs post-quantum crypto ─────────────────────────────
option(APEX_USE_LIBOQS "Enable post-quantum crypto via liboqs" ON)
set(APEX_LIBOQS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/liboqs" CACHE PATH "Path to liboqs source/build")

if(APEX_USE_LIBOQS)
    if(EXISTS "${APEX_LIBOQS_DIR}/src/oqsconfig.h.cmake")
        # Source tree present: build liboqs as a subdirectory
        set(OQS_BUILD_ONLY_LIB ON CACHE BOOL "Build only the library" FORCE)
        set(OQS_USE_OPENSSL OFF CACHE BOOL "Do not use OpenSSL" FORCE)
        set(OQS_ENABLE_SIG_DILITHIUM ON CACHE BOOL "Enable Dilithium" FORCE)
        set(OQS_ENABLE_SIG_ALL OFF CACHE BOOL "Disable all other sigs" FORCE)
        set(OQS_ENABLE_KEM_ALL OFF CACHE BOOL "Disable all KEM" FORCE)
        set(OQS_OPT_FLAGS "-O2 -ffunction-sections -fdata-sections" CACHE STRING "liboqs optimization" FORCE)
        add_subdirectory(${APEX_LIBOQS_DIR} liboqs_build)
        set(LIBOQS_TARGET oqs)
        set(HAVE_LIBOQS 1)
        message(STATUS "liboqs: building from source at ${APEX_LIBOQS_DIR}")
    elseif(EXISTS "${APEX_LIBOQS_DIR}/lib/liboqs.a")
        # Prebuilt library
        add_library(oqs STATIC IMPORTED)
        set_target_properties(oqs PROPERTIES
            IMPORTED_LOCATION "${APEX_LIBOQS_DIR}/lib/liboqs.a"
            INTERFACE_INCLUDE_DIRECTORIES "${APEX_LIBOQS_DIR}/include"
        )
        set(LIBOQS_TARGET oqs)
        set(HAVE_LIBOQS 1)
        message(STATUS "liboqs: using prebuilt at ${APEX_LIBOQS_DIR}")
    else()
        message(WARNING "liboqs not found at ${APEX_LIBOQS_DIR}. "
            "Run scripts/build_liboqs_android.ps1 or set APEX_USE_LIBOQS=OFF")
        set(HAVE_LIBOQS 0)
    endif()
else()
    set(HAVE_LIBOQS 0)
endif()

if(HAVE_LIBOQS)
    add_compile_definitions(APEX_HAVE_LIBOQS)
endif()

# ─── Core include paths ─────────────────────────────────────
# 包含 ctrl/ 和 legacy/ 目录（位于仓库根 apex-root/ 下）
set(APEX_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")
set(CORE_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/detect
    ${CMAKE_CURRENT_SOURCE_DIR}/ebpf
    ${CMAKE_CURRENT_SOURCE_DIR}/namespace
    ${CMAKE_CURRENT_SOURCE_DIR}/cure
    ${CMAKE_CURRENT_SOURCE_DIR}/guard
    ${CMAKE_CURRENT_SOURCE_DIR}/game
    ${CMAKE_CURRENT_SOURCE_DIR}/hid
    ${CMAKE_CURRENT_SOURCE_DIR}/bare_syscall
    ${CMAKE_CURRENT_SOURCE_DIR}/trusted_root
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel_observer
    ${CMAKE_CURRENT_SOURCE_DIR}/consensus
    ${CMAKE_CURRENT_SOURCE_DIR}/micro_services
    ${CMAKE_CURRENT_SOURCE_DIR}/ai_engine
    ${CMAKE_CURRENT_SOURCE_DIR}/updater
    # 新增：ctrl/ 和 legacy/ 在仓库根
    ${APEX_ROOT_DIR}/ctrl
    ${APEX_ROOT_DIR}/legacy
    ${APEX_LIBOQS_DIR}/include  # liboqs headers for prebuilt case
)

# ─── Source file collections ────────────────────────────────
file(GLOB_RECURSE BARESYSCALL_SRC "bare_syscall/*.c" "bare_syscall/*.cpp")
file(GLOB_RECURSE TRUSTED_ROOT_SRC "trusted_root/*.cpp" "trusted_root/*.c")
list(FILTER TRUSTED_ROOT_SRC EXCLUDE REGEX "daemon_main\\.cpp$")
file(GLOB_RECURSE KERNEL_OBSERVER_SRC "kernel_observer/*.cpp")
file(GLOB_RECURSE CONSENSUS_SRC "consensus/*.cpp")
file(GLOB_RECURSE AI_ENGINE_SRC "ai_engine/*.cpp")
file(GLOB_RECURSE UPDATER_SRC "updater/*.cpp")
file(GLOB_RECURSE MICRO_ENGINE_SRC "micro_services/engine/*.cpp")
file(GLOB_RECURSE MICRO_SANDBOX_SRC "micro_services/sandbox/*.cpp")
file(GLOB_RECURSE DETECT_SRC "detect/*.cpp")
file(GLOB_RECURSE EBPF_SRC "ebpf/*.cpp")
file(GLOB_RECURSE NAMESPACE_SRC "namespace/*.cpp")
file(GLOB_RECURSE CURE_SRC "cure/*.cpp")
file(GLOB_RECURSE GUARD_SRC "guard/*.cpp")
file(GLOB_RECURSE GAME_SRC "game/*.cpp")
file(GLOB_RECURSE HID_SRC "hid/*.cpp")
file(GLOB_RECURSE COMMON_SRC "common/*.cpp")

# ─── Hide mode sources (位于仓库根 ctrl/ 和 legacy/) ────────
file(GLOB CTRL_SRC
    "${APEX_ROOT_DIR}/ctrl/apex_self_hide.cpp"
    "${APEX_ROOT_DIR}/ctrl/apex_firewall_ctrl.cpp"
    "${APEX_ROOT_DIR}/ctrl/apex_runtime_engine.cpp"
    "${APEX_ROOT_DIR}/ctrl/apex_pmu_spoof.c"
    "${APEX_ROOT_DIR}/ctrl/apex_timing_spoof.c"
    "${APEX_ROOT_DIR}/ctrl/apex_mem_spoof.c"
)
file(GLOB LEGACY_SRC
    "${APEX_ROOT_DIR}/legacy/apex_legacy_ctrl.cpp"
)
# JNI hide mode bridge
set(HIDE_JNI_SRC "${APEX_ROOT_DIR}/jni/native_bridge.cpp")

# ─── Library: bare syscall (static) ────────────────────────
add_library(baresyscall STATIC ${BARESYSCALL_SRC})
target_include_directories(baresyscall PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/bare_syscall)
target_compile_options(baresyscall PRIVATE -fvisibility=hidden -fstack-protector-strong)
set_target_properties(baresyscall PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ─── Library: trusted_root (static) ────────────────────────
add_library(trusted_root STATIC ${TRUSTED_ROOT_SRC})
target_include_directories(trusted_root PUBLIC ${CORE_INCLUDE_DIRS})
target_link_libraries(trusted_root PUBLIC baresyscall ${LIBOQS_TARGET})
target_compile_options(trusted_root PRIVATE -fvisibility=hidden -fstack-protector-strong)

# ─── Main shared library: apex_root (all features + JNI) ──
add_library(apex_root SHARED
    native-lib.cpp
    ${HIDE_JNI_SRC}
    ${COMMON_SRC}
    ${DETECT_SRC}
    ${EBPF_SRC}
    ${NAMESPACE_SRC}
    ${CURE_SRC}
    ${GUARD_SRC}
    ${GAME_SRC}
    ${HID_SRC}
    ${KERNEL_OBSERVER_SRC}
    ${CONSENSUS_SRC}
    ${AI_ENGINE_SRC}
    ${UPDATER_SRC}
    ${MICRO_ENGINE_SRC}
    ${MICRO_SANDBOX_SRC}
    # 新增：隐藏功能模块
    ${CTRL_SRC}
    ${LEGACY_SRC}
)
target_include_directories(apex_root PRIVATE ${CORE_INCLUDE_DIRS})
target_link_libraries(apex_root PRIVATE baresyscall trusted_root ${LIBOQS_TARGET} dl log)
target_link_options(apex_root PRIVATE -rdynamic)
target_compile_options(apex_root PRIVATE -fvisibility=hidden -fstack-protector-strong)

# ─── 20 micro-service plugins (individual .so files) ─────
set(PLUGIN_DIRS
    ms001_file_scanner    ms002_proc_scanner    ms003_mem_scanner
    ms004_mount_check     ms005_sidechannel     ms006_kernel_probe
    ms007_tee_bypass      ms008_bootloader      ms009_custom_rom
    ms010_dangerous_apps  ms011_pif_check       ms012_zygisk
    ms013_hook_detector   ms014_selinux_check   ms015_property_check
    ms016_network_monitor ms017_crypto_verify   ms018_firmware_check
    ms019_hardware_probe  ms020_sandbox_detect
)

foreach(SVC ${PLUGIN_DIRS})
    set(SVC_PATH "micro_services/services/${SVC}")
    file(GLOB SVC_SRC "${SVC_PATH}/*.cpp" "${SVC_PATH}/*.c")
    if(SVC_SRC)
        add_library(${SVC} SHARED ${SVC_SRC})
        target_include_directories(${SVC} PRIVATE ${CORE_INCLUDE_DIRS})
        # Plugins reference apex::engine::service_engine::register_service which
        # lives in the main apex_root library. Link against it so the symbol
        # resolves at link time (the host process provides it at runtime).
        target_link_libraries(${SVC} PRIVATE baresyscall trusted_root apex_root ${LIBOQS_TARGET} dl log)
        set_target_properties(${SVC} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/plugins"
        )
        target_compile_options(${SVC} PRIVATE -fvisibility=hidden -fstack-protector-strong)
    endif()
endforeach()

# ─── Executable: trusted_daemon (independent process) ─────
add_executable(trusted_daemon
    trusted_root/daemon_main.cpp
)
target_include_directories(trusted_daemon PRIVATE ${CORE_INCLUDE_DIRS})
target_link_libraries(trusted_daemon PRIVATE baresyscall trusted_root ${LIBOQS_TARGET} dl log)
target_compile_options(trusted_daemon PRIVATE -fvisibility=hidden -fstack-protector-strong)
