package com.apex.root.data.jni

import com.apex.root.core.NativeLibraryLoader

/**
 * JNI 桥接层 — 所有 native 方法调用都通过 NativeLibraryLoader 安全保护。
 * 避免 UnsatisfiedLinkError 直接崩溃。
 *
 * 架构诚实化说明 (v1.1.1):
 *   - 微服务 (micro_services): 实验性功能。通过 [initMicroServices] 触发
 *     service_engine::initialize(), 在 [ApexViewModel.checkNativeAvailability]
 *     中于 setPluginsDir 后调用。即便所有 plugin dlopen 失败, 主扫描仍走
 *     [runQuickScanNative] 传统路径, 不影响功能。
 *   - 共识 (consensus/replica_manager): **未通过任何 JNI 暴露**, 是死代码。
 *     详见 native-lib.cpp 顶部 P0-D6 注释与 consensus/replica_manager.h 顶部
 *     注释。三副本共识将在 v1.2.0 通过独立 root daemon 重新实现。
 *   - 命名空间隔离 (namespace_isolation): 已重写为安全 stub (P0-C4 修复),
 *     非 root 设备 [com.apex.root.island.NativeIsland.createIsolatedEnv] 始终
 *     返回 -1。详见 namespace/namespace_isolation.cpp 顶部注释。
 */
object NativeBridge {

    // ─── Quick Scan ────────────────────────────────────
    fun runQuickScan(): String = NativeLibraryLoader.safeCall("扫描不可用：原生库未加载") {
        runQuickScanNative()
    }

    fun isDeviceRooted(): Boolean = NativeLibraryLoader.safeCall(false) {
        isDeviceRootedNative()
    }

    fun getRiskScore(): Int = NativeLibraryLoader.safeCall(0) {
        getRiskScoreNative()
    }

    // ─── Post-quantum crypto ──────────────────────────
    fun isPostQuantumAvailable(): Boolean = NativeLibraryLoader.safeCall(false) {
        isPostQuantumAvailableNative()
    }

    fun getSignedReport(): String = NativeLibraryLoader.safeCall("") {
        getSignedReportNative()
    }

    fun getPostQuantumInfo(): String = NativeLibraryLoader.safeCall("") {
        getPostQuantumInfoNative()
    }

    // ─── Deep memory fingerprint scan ───────────────────
    fun fullMemoryFingerprint(): Int = NativeLibraryLoader.safeCall(0) {
        fullMemoryFingerprintNative()
    }

    fun deepMemoryScanReport(): String = NativeLibraryLoader.safeCall("") {
        deepMemoryScanReportNative()
    }

    fun countRWXPages(): Int = NativeLibraryLoader.safeCall(0) {
        countRWXPagesNative()
    }

    // ─── SELinux context detection ─────────────────────
    fun detectSELinuxContextJump(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectSELinuxContextJumpNative()
    }

    // ─── eBPF capability probe (Hide Mode UX) ─────────
    /**
     * Probe the kernel + SELinux environment for eBPF hide-mode support.
     * Returns a human-readable report describing whether eBPF path will
     * be used or whether hide-mode falls back to mount-namespace + rename.
     */
    fun getEbpfCapabilityReport(): String = NativeLibraryLoader.safeCall("eBPF capability probe unavailable") {
        getEbpfCapabilityReportNative()
    }

    /**
     * P1-1 修复: 设置微服务插件目录 (nativeLibraryDir + "/plugins")。
     * 必须在 [initMicroServices] / service_engine::initialize() 之前调用。
     */
    fun setPluginsDir(path: String) = NativeLibraryLoader.safeRun {
        setPluginsDirNative(path)
    }

    /**
     * P0-D5 修复 (v1.1.1): 触发微服务引擎初始化。
     *
     * 此前 service_engine::initialize() 从未被任何 JNI 调用, 导致 20 个 plugin.so
     * (ms001 ~ ms020) 虽然编译并打包到 apk, 但运行时永远没被 dlopen, 整个微服务
     * 架构是死代码。本方法接入后, 由 [ApexViewModel.checkNativeAvailability] 在
     * native 库加载成功且 [setPluginsDir] 调用完毕后执行。
     *
     * 注意: 微服务架构为实验性功能。service_engine::initialize() 内部对每个
     * plugin 的 dlopen 失败都有容错, 且恒返回 true (即便全部 plugin 加载失败,
     * 主扫描仍走 runQuickScanNative 的传统路径, 不会崩溃)。本方法的返回值仅
     * 用于日志展示, 不影响主流程。
     *
     * @return true 表示 initialize() 已执行 (无论 plugin 是否真正加载成功)
     */
    fun initMicroServices(): Boolean = NativeLibraryLoader.safeCall(false) {
        initMicroServicesNative()
    }

    fun detectSELinuxPolicyMod(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectSELinuxPolicyModNative()
    }

    fun selinuxFullScan(): String = NativeLibraryLoader.safeCall("") {
        selinuxFullScanNative()
    }

    // ─── Anti-hiding probes ────────────────────────────
    fun detectShamiko(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectShamikoNative()
    }

    fun shamikoFullScan(): String = NativeLibraryLoader.safeCall("") {
        shamikoFullScanNative()
    }

    fun detectZygiskNext(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectZygiskNextNative()
    }

    fun zygiskNextFullScan(): String = NativeLibraryLoader.safeCall("") {
        zygiskNextFullScanNative()
    }

    fun detectProcessHiding(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectProcessHidingNative()
    }

    fun detectMountNamespaceHiding(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectMountNamespaceHidingNative()
    }

    // Ring0 /proc/kallsyms-based detectSyscallTableHook() fully removed.
    // Syscall tamper detection is now Ring3-only:
    //   detectSyscallResultInconsistency() — user-mode syscall result consistency.
    fun detectSyscallResultInconsistency(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectSyscallResultInconsistencyNative()
    }

    // ─── ART enhanced detection ───────────────────────
    fun artEnhancedScan(): String = NativeLibraryLoader.safeCall("") {
        artEnhancedScanNative()
    }

    // ─── Xposed 框架检测 ──────────────────────────────
    /** Xposed / LSPosed / EdXposed 框架检测 */
    fun detectXposedFramework(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectXposedFrameworkNative()
    }

    // ─── L14: VirtualXposed / 太极 / 双开分身 ────────────
    fun detectVirtualXposed(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectVirtualXposedNative()
    }

    fun detectTaiChi(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectTaiChiNative()
    }

    fun detectDualSpaceApps(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectDualSpaceAppsNative()
    }

    fun virtualXposedFullScan(): String = NativeLibraryLoader.safeCall("") {
        virtualXposedFullScanNative()
    }

    // ─── L15: 危险应用检测 ────────────────────────────
    fun detectGameGuardian(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectGameGuardianNative()
    }

    fun detectCheatEngine(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectCheatEngineNative()
    }

    fun detectLuckyPatcher(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectLuckyPatcherNative()
    }

    fun detectMemoryEditors(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectMemoryEditorsNative()
    }

    fun detectCrackingTools(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectCrackingToolsNative()
    }

    // ─── Frida 检测 ────────────────────────────────────
    /** Frida 二进制 / 内存痕迹 / 端口检测（合并布尔结果） */
    fun detectFrida(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectFridaNative()
    }

    fun dangerousAppsFullScan(): String = NativeLibraryLoader.safeCall("") {
        dangerousAppsFullScanNative()
    }

    // ─── L16: Magisk 扩展检测 ────────────────────────
    fun detectMagiskDenyList(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectMagiskDenyListNative()
    }

    fun detectZygiskModules(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectZygiskModulesNative()
    }

    fun detectLSPosedManager(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectLSPosedManagerNative()
    }

    fun detectRiruModules(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectRiruModulesNative()
    }

    fun detectModernForks(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectModernForksNative()
    }

    fun detectHideMyApplist(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectHideMyApplistNative()
    }

    fun detectStorageIsolation(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectStorageIsolationNative()
    }

    fun detectMagiskHideLegacy(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectMagiskHideLegacyNative()
    }

    fun magiskExtensionsFullScan(): String = NativeLibraryLoader.safeCall("") {
        magiskExtensionsFullScanNative()
    }

    // ─── v1.1.0 新增: L17-L20 检测层 ──────────────────
    /** L17: 现代 Root Fork (SukiSU / Magisk Delta / Kitsune / ReZygisk variants) */
    fun detectModernRootForksV2(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectModernRootForksV2Native()
    }
    fun detectSukiSU(): Boolean = NativeLibraryLoader.safeCall(false) { detectSukiSUNative() }
    fun detectSukiSUUltra(): Boolean = NativeLibraryLoader.safeCall(false) { detectSukiSUUltraNative() }
    fun detectReZygiskV2(): Boolean = NativeLibraryLoader.safeCall(false) { detectReZygiskV2Native() }
    fun detectMagiskDelta(): Boolean = NativeLibraryLoader.safeCall(false) { detectMagiskDeltaNative() }
    fun modernRootForksFullScan(): String = NativeLibraryLoader.safeCall("") {
        modernRootForksFullScanNative()
    }

    /** L18: APatch KPM 用户态 + Trampoline + KernelPatch */
    fun detectAPatchKPM(): Boolean = NativeLibraryLoader.safeCall(false) { detectAPatchKPMNative() }
    fun detectAPatchTrampoline(): Boolean = NativeLibraryLoader.safeCall(false) { detectAPatchTrampolineNative() }
    fun detectKernelPatchProject(): Boolean = NativeLibraryLoader.safeCall(false) { detectKernelPatchProjectNative() }
    fun apatchKpmFullScan(): String = NativeLibraryLoader.safeCall("") {
        apatchKpmFullScanNative()
    }

    /** L19: 隐藏框架 (Zygisk-Assistant / AML / MagiskFrida / 持久化脚本) */
    fun detectZygiskAssistant(): Boolean = NativeLibraryLoader.safeCall(false) { detectZygiskAssistantNative() }
    fun detectPersistentScripts(): Boolean = NativeLibraryLoader.safeCall(false) { detectPersistentScriptsNative() }
    fun hideFrameworksFullScan(): String = NativeLibraryLoader.safeCall("") {
        hideFrameworksFullScanNative()
    }

    /** L20: 现代 Hook 框架 (Pine/SandHook/ByteHook/ShadowHook/Frida variants/LSPatch) */
    fun detectModernArtHooks(): Boolean = NativeLibraryLoader.safeCall(false) { detectModernArtHooksNative() }
    fun detectFridaVariants(): Boolean = NativeLibraryLoader.safeCall(false) { detectFridaVariantsNative() }
    fun detectLSPatch(): Boolean = NativeLibraryLoader.safeCall(false) { detectLSPatchNative() }
    fun detectModernHookFrameworks(): Boolean = NativeLibraryLoader.safeCall(false) { detectModernHookFrameworksNative() }
    fun modernHooksFullScan(): String = NativeLibraryLoader.safeCall("") {
        modernHooksFullScanNative()
    }

    // ─── v1.0.5 新增: L21-L23 检测层 ──────────────────
    // FIX-P1-CPP (v1.1.2): 原层号 L24_dhizuku 已重命名为 L23_dhiziku,
    // 补齐 detect/ 目录编号 (layer1-layer22 + layer23)。历史上 L23 曾计划用于
    // "高级反调试 / 系统调用拦截检测", 但因与 L5_sidechannel 重叠未落地, 实际
    // 将原 L24 (Dhizuku/Shizuku/Stellar 特权框架检测) 重新编号为 L23。
    // 当前 ParallelDetectionEngine.TOTAL_LAYERS 已体现该情况 (跳过 L23)。
    /** L21: Play Integrity 伪造检测 (PIF + TrickyStore) */
    fun detectPlayIntegrityTampering(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectPlayIntegrityTamperingNative()
    }
    fun detectPIFModule(): Boolean = NativeLibraryLoader.safeCall(false) { detectPIFModuleNative() }
    fun detectTrickyStoreModule(): Boolean = NativeLibraryLoader.safeCall(false) { detectTrickyStoreModuleNative() }
    fun playIntegrityFullScan(): String = NativeLibraryLoader.safeCall("") { playIntegrityFullScanNative() }

    /** L22: 模拟器检测 (QEMU/Genymotion/BlueStacks/Nox/LDPlayer) */
    fun detectEmulator(): Boolean = NativeLibraryLoader.safeCall(false) { detectEmulatorNative() }
    fun detectQEMU(): Boolean = NativeLibraryLoader.safeCall(false) { detectQEMUNative() }
    fun emulatorFullScan(): String = NativeLibraryLoader.safeCall("") { emulatorFullScanNative() }

    // L23: Dhizuku/Shizuku/Stellar 特权框架检测 (FIX-P1-CPP: 原 L24 已重编号为 L23)
    fun detectDhizuku(): Boolean = NativeLibraryLoader.safeCall(false) { detectDhizukuNative() }
    fun detectShizuku(): Boolean = NativeLibraryLoader.safeCall(false) { detectShizukuNative() }
    fun detectStellar(): Boolean = NativeLibraryLoader.safeCall(false) { detectStellarNative() }
    fun dhizukuFullScan(): String = NativeLibraryLoader.safeCall("") { dhizukuFullScanNative() }

    // ─── Hide Mode (隐藏模式控制) ──────────────────────
    // 启动隐藏模式：对除 APEX 外的应用隐藏 root 痕迹
    fun enableHideMode(appUid: Int): Int = NativeLibraryLoader.safeCall(-1) {
        enableHideModeNative(appUid)
    }

    // 启动游戏模式：aggressive 隐藏 + 性能优化
    fun enableGameMode(appUid: Int): Int = NativeLibraryLoader.safeCall(-1) {
        enableGameModeNative(appUid)
    }

    // 停止隐藏模式，回到 Detection
    fun disableHideMode() = NativeLibraryLoader.safeCall(Unit) {
        disableHideModeNative()
    }

    // 隐藏模式是否激活
    fun isHideModeActive(): Boolean = NativeLibraryLoader.safeCall(false) {
        isHideModeActiveNative()
    }

    // 获取当前模式 (0=Detect / 1=Hide / 2=Game)
    fun getCurrentMode(): Int = NativeLibraryLoader.safeCall(0) {
        getCurrentModeNative()
    }

    // 获取 native 层最近一次错误信息
    fun getLastError(): String = NativeLibraryLoader.safeCall("") {
        getLastErrorNative()
    }

    // ─── Firmware partition integrity ─────────────────
    fun detectFirmwareTampering(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectFirmwareTamperingNative()
    }

    fun detectAVBStatus(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectAVBStatusNative()
    }

    fun detectCustomRecovery(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectCustomRecoveryNative()
    }

    fun firmwareFullScan(): String = NativeLibraryLoader.safeCall("") {
        firmwareFullScanNative()
    }

    // ─── Device identity / crypto helpers ────────────
    fun getDeviceIdentifier(): String? = NativeLibraryLoader.safeCall(null) {
        getDeviceIdentifierNative()
    }

    fun sha3_512(data: ByteArray): ByteArray = NativeLibraryLoader.safeCall(ByteArray(0)) {
        sha3_512Native(data)
    }

    // ─── 原始 external 方法（私有）─────────────────────
    private external fun runQuickScanNative(): String
    private external fun isDeviceRootedNative(): Boolean
    private external fun getRiskScoreNative(): Int
    private external fun isPostQuantumAvailableNative(): Boolean
    private external fun getSignedReportNative(): String
    private external fun getPostQuantumInfoNative(): String
    private external fun fullMemoryFingerprintNative(): Int
    private external fun deepMemoryScanReportNative(): String
    private external fun countRWXPagesNative(): Int
    private external fun detectSELinuxContextJumpNative(): Boolean
    private external fun getEbpfCapabilityReportNative(): String
    private external fun setPluginsDirNative(path: String)
    // P0-D5 修复 (v1.1.1): 微服务引擎初始化 — 由 initMicroServices() 调用。
    // 注意: 微服务架构为实验性功能, 当前 service_engine::initialize() 恒返回 true,
    // 个别 plugin dlopen 失败会被内部容错吞掉, 仅记录 native 日志。
    private external fun initMicroServicesNative(): Boolean
    private external fun detectSELinuxPolicyModNative(): Boolean
    private external fun selinuxFullScanNative(): String
    private external fun detectShamikoNative(): Boolean
    private external fun shamikoFullScanNative(): String
    private external fun detectZygiskNextNative(): Boolean
    private external fun zygiskNextFullScanNative(): String
    private external fun detectProcessHidingNative(): Boolean
    private external fun detectMountNamespaceHidingNative(): Boolean
    // Ring0 detectSyscallTableHookNative removed — replaced by Ring3 side-channel.
    private external fun detectSyscallResultInconsistencyNative(): Boolean
    private external fun artEnhancedScanNative(): String
    private external fun detectXposedFrameworkNative(): Boolean
    // L14 / L15 / L16 native
    private external fun detectVirtualXposedNative(): Boolean
    private external fun detectTaiChiNative(): Boolean
    private external fun detectDualSpaceAppsNative(): Boolean
    private external fun virtualXposedFullScanNative(): String
    private external fun detectGameGuardianNative(): Boolean
    private external fun detectCheatEngineNative(): Boolean
    private external fun detectLuckyPatcherNative(): Boolean
    private external fun detectMemoryEditorsNative(): Boolean
    private external fun detectCrackingToolsNative(): Boolean
    private external fun detectFridaNative(): Boolean
    private external fun dangerousAppsFullScanNative(): String
    private external fun detectMagiskDenyListNative(): Boolean
    private external fun detectZygiskModulesNative(): Boolean
    private external fun detectLSPosedManagerNative(): Boolean
    private external fun detectRiruModulesNative(): Boolean
    private external fun detectModernForksNative(): Boolean
    private external fun detectHideMyApplistNative(): Boolean
    private external fun detectStorageIsolationNative(): Boolean
    private external fun detectMagiskHideLegacyNative(): Boolean
    private external fun magiskExtensionsFullScanNative(): String

    // v1.1.0 L17-L20 native
    private external fun detectModernRootForksV2Native(): Boolean
    private external fun detectSukiSUNative(): Boolean
    private external fun detectSukiSUUltraNative(): Boolean
    private external fun detectReZygiskV2Native(): Boolean
    private external fun detectMagiskDeltaNative(): Boolean
    private external fun modernRootForksFullScanNative(): String
    private external fun detectAPatchKPMNative(): Boolean
    private external fun detectAPatchTrampolineNative(): Boolean
    private external fun detectKernelPatchProjectNative(): Boolean
    private external fun apatchKpmFullScanNative(): String
    private external fun detectZygiskAssistantNative(): Boolean
    private external fun detectPersistentScriptsNative(): Boolean
    private external fun hideFrameworksFullScanNative(): String
    private external fun detectModernArtHooksNative(): Boolean
    private external fun detectFridaVariantsNative(): Boolean
    private external fun detectLSPatchNative(): Boolean
    private external fun detectModernHookFrameworksNative(): Boolean
    private external fun modernHooksFullScanNative(): String

    // v1.0.5 L21-L23 native (FIX-P1-CPP: 原 L24 已重编号为 L23)
    private external fun detectPlayIntegrityTamperingNative(): Boolean
    private external fun detectPIFModuleNative(): Boolean
    private external fun detectTrickyStoreModuleNative(): Boolean
    private external fun playIntegrityFullScanNative(): String
    private external fun detectEmulatorNative(): Boolean
    private external fun detectQEMUNative(): Boolean
    private external fun emulatorFullScanNative(): String
    private external fun detectDhizukuNative(): Boolean
    private external fun detectShizukuNative(): Boolean
    private external fun detectStellarNative(): Boolean
    private external fun dhizukuFullScanNative(): String
    private external fun detectFirmwareTamperingNative(): Boolean
    private external fun detectAVBStatusNative(): Boolean
    private external fun detectCustomRecoveryNative(): Boolean
    private external fun firmwareFullScanNative(): String
    private external fun getDeviceIdentifierNative(): String?
    private external fun sha3_512Native(data: ByteArray): ByteArray

    // Hide Mode native methods (由 jni/native_bridge.cpp 实现)
    private external fun enableHideModeNative(appUid: Int): Int
    private external fun enableGameModeNative(appUid: Int): Int
    private external fun disableHideModeNative()
    private external fun isHideModeActiveNative(): Boolean
    private external fun getCurrentModeNative(): Int
    private external fun getLastErrorNative(): String
}
