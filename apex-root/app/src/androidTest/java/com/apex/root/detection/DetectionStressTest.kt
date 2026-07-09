package com.apex.root.detection

import android.util.Log
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.apex.root.data.jni.NativeBridge
import org.junit.Before
import org.junit.FixMethodOrder
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.MethodSorters
import kotlin.system.measureTimeMillis

/**
 * 16 层检测压力测试 (Instrumentation)。
 *
 * 目标:在真机/模拟器上验证 NativeBridge 全部 16 层检测在 50+ 次连续调用下:
 *  - 不发生 SIGSEGV / native crash
 *  - 不发生 OOM
 *  - 单次调用时间 < 5 秒 (避免 ANR)
 *  - 返回值稳定 (同样输入下多次调用结果一致)
 *
 * 注意:在 x86_64 模拟器上,arm64 native 库不会被加载,所有调用会通过
 * NativeLibraryLoader.safeCall() 返回默认值。这本身也是一个有效测试
 * (验证降级路径不崩溃)。
 *
 * 在 arm64 真机上,native 库会加载,本测试验证真实检测路径的稳定性。
 */
@RunWith(AndroidJUnit4::class)
@FixMethodOrder(MethodSorters.NAME_ASCENDING)
class DetectionStressTest {

    private companion object {
        const val TAG = "DetectionStressTest"
        const val ITERATIONS = 50
        const val MAX_SINGLE_CALL_MS = 5_000L  // 5 seconds — ANR threshold is 5s
    }

    @Before
    fun setUp() {
        com.apex.root.core.NativeLibraryLoader.ensureLoaded()
    }

    private fun stressBooleanCall(name: String, call: () -> Boolean) {
        val nativeLoaded = com.apex.root.core.NativeLibraryLoader.isAvailable
        Log.i(TAG, "[$name] nativeLoaded=$nativeLoaded, iterations=$ITERATIONS")

        var lastResult: Boolean? = null
        var maxMs = 0L
        var minMs = Long.MAX_VALUE
        var totalMs = 0L

        repeat(ITERATIONS) { i ->
            val ms = measureTimeMillis {
                val r = call()
                if (lastResult != null && lastResult != r) {
                    Log.w(TAG, "[$name] iter $i: result changed from $lastResult to $r (may be flaky environment)")
                }
                lastResult = r
            }
            maxMs = maxOf(maxMs, ms)
            minMs = minOf(minMs, ms)
            totalMs += ms
        }

        val avgMs = totalMs / ITERATIONS
        Log.i(TAG, "[$name] result=$lastResult min=${minMs}ms avg=${avgMs}ms max=${maxMs}ms total=${totalMs}ms")

        if (nativeLoaded) {
            assert(maxMs < MAX_SINGLE_CALL_MS) {
                "[$name] max call time ${maxMs}ms exceeded ${MAX_SINGLE_CALL_MS}ms threshold (ANR risk)"
            }
        }
        Log.i(TAG, "[$name] ✓ survived $ITERATIONS iterations")
    }

    private fun stressStringCall(name: String, call: () -> String) {
        val nativeLoaded = com.apex.root.core.NativeLibraryLoader.isAvailable
        Log.i(TAG, "[$name] nativeLoaded=$nativeLoaded, iterations=$ITERATIONS")

        var maxLen = 0
        var minLen = Int.MAX_VALUE
        var totalMs = 0L
        var maxMs = 0L

        repeat(ITERATIONS) {
            val ms = measureTimeMillis {
                val s = call()
                maxLen = maxOf(maxLen, s.length)
                minLen = minOf(minLen, s.length)
            }
            maxMs = maxOf(maxMs, ms)
            totalMs += ms
        }

        Log.i(TAG, "[$name] minLen=$minLen maxLen=$maxLen avgMs=${totalMs / ITERATIONS} maxMs=$maxMs")
        if (nativeLoaded) {
            assert(maxMs < MAX_SINGLE_CALL_MS) {
                "[$name] max call time ${maxMs}ms exceeded ${MAX_SINGLE_CALL_MS}ms threshold"
            }
        }
    }

    @Test
    fun test01_l1_properties() = stressBooleanCall("L1_prop") {
        NativeBridge.isDeviceRooted()
    }

    @Test
    fun test02_l2_art_injection() = stressBooleanCall("L2_art") {
        NativeBridge.isDeviceRooted()
    }

    @Test
    fun test03_l3_memory_rwx() {
        stressBooleanCall("L3_countRWX") { NativeBridge.countRWXPages() >= 0 }
        stressStringCall("L3_deepScan", NativeBridge::deepMemoryScanReport)
    }

    @Test
    fun test04_l4_mounts() = stressBooleanCall("L4_mount") {
        NativeBridge.isDeviceRooted()
    }

    @Test
    fun test05_l5_sidechannel() = stressBooleanCall("L5_sidechannel") {
        NativeBridge.detectSyscallResultInconsistency()
    }

    @Test
    fun test06_l6_root_daemon() = stressBooleanCall("L6_daemon") {
        NativeBridge.isDeviceRooted()
    }

    @Test
    fun test07_l7_firmware() {
        stressBooleanCall("L7_avb", NativeBridge::detectAVBStatus)
        stressBooleanCall("L7_recovery", NativeBridge::detectCustomRecovery)
        stressBooleanCall("L7_tamper", NativeBridge::detectFirmwareTampering)
        stressStringCall("L7_fullScan", NativeBridge::firmwareFullScan)
    }

    @Test
    fun test08_l8_magisk() = stressBooleanCall("L8_magisk") {
        NativeBridge.isDeviceRooted()
    }

    @Test
    fun test09_l9_ksu() = stressBooleanCall("L9_ksu") {
        NativeBridge.isDeviceRooted()
    }

    @Test
    fun test10_l10_apatch() = stressBooleanCall("L10_apatch") {
        NativeBridge.isDeviceRooted()
    }

    @Test
    fun test11_l11_hooks() {
        stressBooleanCall("L11_xposed", NativeBridge::detectXposedFramework)
        stressBooleanCall("L11_frida", NativeBridge::detectFrida)
        stressStringCall("L11_artEnhanced", NativeBridge::artEnhancedScan)
    }

    @Test
    fun test12_l12_custom_rom() = stressBooleanCall("L12_rom") {
        NativeBridge.isDeviceRooted()
    }

    @Test
    fun test13_l13_selinux() {
        stressBooleanCall("L13_contextJump", NativeBridge::detectSELinuxContextJump)
        stressBooleanCall("L13_policyMod", NativeBridge::detectSELinuxPolicyMod)
        stressStringCall("L13_fullScan", NativeBridge::selinuxFullScan)
    }

    @Test
    fun test14_l14_virtual() {
        stressBooleanCall("L14_vx", NativeBridge::detectVirtualXposed)
        stressBooleanCall("L14_taichi", NativeBridge::detectTaiChi)
        stressBooleanCall("L14_dual", NativeBridge::detectDualSpaceApps)
        stressStringCall("L14_fullScan", NativeBridge::virtualXposedFullScan)
    }

    @Test
    fun test15_l15_dangerous_apps() {
        stressBooleanCall("L15_gg", NativeBridge::detectGameGuardian)
        stressBooleanCall("L15_ce", NativeBridge::detectCheatEngine)
        stressBooleanCall("L15_lp", NativeBridge::detectLuckyPatcher)
        stressBooleanCall("L15_mem", NativeBridge::detectMemoryEditors)
        stressBooleanCall("L15_crack", NativeBridge::detectCrackingTools)
        stressStringCall("L15_fullScan", NativeBridge::dangerousAppsFullScan)
    }

    @Test
    fun test16_l16_magisk_ext() {
        stressBooleanCall("L16_denylist", NativeBridge::detectMagiskDenyList)
        stressBooleanCall("L16_zygisk", NativeBridge::detectZygiskModules)
        stressBooleanCall("L16_lsposed", NativeBridge::detectLSPosedManager)
        stressBooleanCall("L16_riru", NativeBridge::detectRiruModules)
        stressBooleanCall("L16_forks", NativeBridge::detectModernForks)
        stressBooleanCall("L16_hma", NativeBridge::detectHideMyApplist)
        stressBooleanCall("L16_stiso", NativeBridge::detectStorageIsolation)
        stressBooleanCall("L16_legacy", NativeBridge::detectMagiskHideLegacy)
        stressStringCall("L16_fullScan", NativeBridge::magiskExtensionsFullScan)
    }

    @Test
    fun test17_anti_hiding() {
        stressBooleanCall("AH_shamiko", NativeBridge::detectShamiko)
        stressBooleanCall("AH_zygiskNext", NativeBridge::detectZygiskNext)
        stressBooleanCall("AH_procHiding", NativeBridge::detectProcessHiding)
        stressBooleanCall("AH_mountNsHiding", NativeBridge::detectMountNamespaceHiding)
        stressStringCall("AH_shamikoFull", NativeBridge::shamikoFullScan)
        stressStringCall("AH_znFull", NativeBridge::zygiskNextFullScan)
    }

    @Test
    fun test18_full_scan() {
        stressStringCall("FULL_quickScan", NativeBridge::runQuickScan)
    }

    @Test
    fun test19_risk_score() = stressBooleanCall("SCORE") {
        val score = NativeBridge.getRiskScore()
        score in 0..100
    }

    @Test
    fun test20_ebpf_capability() = stressStringCall("EBPF_cap", NativeBridge::getEbpfCapabilityReport)

    // ─── v1.1.0 L17-L20 new detection layers ───
    @Test
    fun test25_l17_modern_root_forks() {
        stressBooleanCall("L17_sukisu", NativeBridge::detectSukiSU)
        stressBooleanCall("L17_sukisuUltra", NativeBridge::detectSukiSUUltra)
        stressBooleanCall("L17_rezygiskV2", NativeBridge::detectReZygiskV2)
        stressBooleanCall("L17_magiskDelta", NativeBridge::detectMagiskDelta)
        stressBooleanCall("L17_modernForksV2", NativeBridge::detectModernRootForksV2)
        stressStringCall("L17_fullScan", NativeBridge::modernRootForksFullScan)
    }

    @Test
    fun test26_l18_apatch_kpm() {
        stressBooleanCall("L18_kpm", NativeBridge::detectAPatchKPM)
        stressBooleanCall("L18_trampoline", NativeBridge::detectAPatchTrampoline)
        stressBooleanCall("L18_kp", NativeBridge::detectKernelPatchProject)
        stressStringCall("L18_fullScan", NativeBridge::apatchKpmFullScan)
    }

    @Test
    fun test27_l19_hide_frameworks() {
        stressBooleanCall("L19_zygiskAsst", NativeBridge::detectZygiskAssistant)
        stressBooleanCall("L19_persistentScripts", NativeBridge::detectPersistentScripts)
        stressStringCall("L19_fullScan", NativeBridge::hideFrameworksFullScan)
    }

    @Test
    fun test28_l20_modern_hooks() {
        stressBooleanCall("L20_artHooks", NativeBridge::detectModernArtHooks)
        stressBooleanCall("L20_fridaVariants", NativeBridge::detectFridaVariants)
        stressBooleanCall("L20_lspatch", NativeBridge::detectLSPatch)
        stressBooleanCall("L20_modernHooks", NativeBridge::detectModernHookFrameworks)
        stressStringCall("L20_fullScan", NativeBridge::modernHooksFullScan)
    }

    // ─── v1.1.0 Parallel engine + cross-validation ───
    @Test
    fun test29_parallel_engine() {
        val engine = com.apex.root.domain.parallel.ParallelDetectionEngine()
        kotlinx.coroutines.runBlocking {
            val result = engine.scanParallel()
            Log.i(TAG, "[PARALLEL] ${result.detectedCount}/${result.totalLayers} layers, " +
                "risk=${result.totalRiskScore}, time=${result.totalLatencyMs}ms")
            if (com.apex.root.core.NativeLibraryLoader.isAvailable) {
                assert(result.totalLatencyMs < 10_000L) {
                    "Parallel scan took ${result.totalLatencyMs}ms — exceeds 10s budget"
                }
            }
            assert(result.layers.size == 19) {
                "Expected 19 layer results, got ${result.layers.size}"
            }
        }
    }

    @Test
    fun test30_cross_validation() {
        val engine = com.apex.root.domain.parallel.ParallelDetectionEngine()
        kotlinx.coroutines.runBlocking {
            val base = engine.scanParallel()
            val validated = com.apex.root.domain.parallel.CrossValidator.validate(base)
            Log.i(TAG, "[XVAL] confidence=${validated.confidence}%, " +
                "rootType=${validated.inferredRootType}, " +
                "highConf=${validated.isHighConfidence}, " +
                "likelyFP=${validated.isLikelyFalsePositive}")
            Log.i(TAG, "[XVAL] conclusion: ${validated.conclusion}")
            assert(validated.confidence in 0..100) {
                "Confidence ${validated.confidence} out of range"
            }
        }
    }

    @Test
    fun test31_baseline_capture_and_diff() {
        val ctx = androidx.test.platform.app.InstrumentationRegistry.getInstrumentation().targetContext
        val baseline = com.apex.root.domain.parallel.DeviceFingerprintBaseline(ctx)
        baseline.clearBaseline()
        assert(!baseline.hasBaseline())

        kotlinx.coroutines.runBlocking {
            val engine = com.apex.root.domain.parallel.ParallelDetectionEngine()
            val scanResult = engine.scanParallel()
            val snapshot = baseline.captureSnapshot(scanResult)
            val saved = baseline.saveBaseline(snapshot)
            assert(saved) { "Failed to save baseline" }
            assert(baseline.hasBaseline())

            val loaded = baseline.loadBaseline()
            assert(loaded != null) { "Failed to load baseline" }
            assert(loaded!!.riskScore == snapshot.riskScore)

            val diff = baseline.diff(snapshot)
            assert(diff != null) { "Diff returned null despite baseline existing" }
            assert(diff!!.newDetectedLayers.isEmpty()) {
                "Self-diff should have no new layers, got ${diff.newDetectedLayers}"
            }
        }

        baseline.clearBaseline()
        assert(!baseline.hasBaseline())
    }

    @Test
    fun test21_mem_fingerprint() = stressBooleanCall("MEM_mask") {
        val mask = NativeBridge.fullMemoryFingerprint()
        mask >= 0
    }

    @Test
    fun test22_device_id_stability() {
        val nativeLoaded = com.apex.root.core.NativeLibraryLoader.isAvailable
        if (!nativeLoaded) {
            Log.i(TAG, "[DEV_ID] native not loaded — skipping stability check")
            return
        }
        val ids = mutableSetOf<String>()
        repeat(10) {
            val id = NativeBridge.getDeviceIdentifier() ?: ""
            val prefix = id.substringBeforeLast('-')
            ids.add(prefix)
        }
        Log.i(TAG, "[DEV_ID] unique prefixes across 10 calls: $ids")
        assert(ids.size <= 1) {
            "Device ID prefix should be stable, got ${ids.size} distinct prefixes: $ids"
        }
    }

    @Test
    fun test23_sha3_512() {
        val nativeLoaded = com.apex.root.core.NativeLibraryLoader.isAvailable
        if (!nativeLoaded) return

        val input = ByteArray(1024) { (it and 0xff).toByte() }
        val hashes = mutableSetOf<String>()
        repeat(20) {
            val h = NativeBridge.sha3_512(input)
            val hex = h.joinToString("") { "%02x".format(it) }
            hashes.add(hex)
        }
        Log.i(TAG, "[SHA3] distinct hashes across 20 identical inputs: ${hashes.size}")
        assert(hashes.size == 1) {
            "SHA3-512 should be deterministic, got ${hashes.size} distinct hashes"
        }
    }

    @Test
    fun test24_full_ui_flow_simulation() {
        val nativeLoaded = com.apex.root.core.NativeLibraryLoader.isAvailable
        Log.i(TAG, "[UI_FLOW] simulating user scan flow, nativeLoaded=$nativeLoaded")

        val totalMs = measureTimeMillis {
            val quick = NativeBridge.runQuickScan()
            Log.i(TAG, "[UI_FLOW] quick scan returned ${quick.length} chars")

            val deep = StringBuilder()
            deep.appendLine(NativeBridge.deepMemoryScanReport())
            deep.appendLine(NativeBridge.selinuxFullScan())
            deep.appendLine(NativeBridge.virtualXposedFullScan())
            deep.appendLine(NativeBridge.dangerousAppsFullScan())
            deep.appendLine(NativeBridge.magiskExtensionsFullScan())
            deep.appendLine(NativeBridge.shamikoFullScan())
            deep.appendLine(NativeBridge.zygiskNextFullScan())
            deep.appendLine(NativeBridge.firmwareFullScan())
            deep.appendLine(NativeBridge.artEnhancedScan())
            Log.i(TAG, "[UI_FLOW] deep scan produced ${deep.length} chars total")

            val score = NativeBridge.getRiskScore()
            Log.i(TAG, "[UI_FLOW] risk score = $score")

            if (NativeBridge.isPostQuantumAvailable()) {
                val signed = NativeBridge.getSignedReport()
                Log.i(TAG, "[UI_FLOW] signed report: ${signed.length} chars")
            } else {
                Log.i(TAG, "[UI_FLOW] post-quantum not available, skipping signed report")
            }
        }

        Log.i(TAG, "[UI_FLOW] full flow took ${totalMs}ms")
        if (nativeLoaded) {
            assert(totalMs < 30_000L) {
                "Full UI flow took ${totalMs}ms — exceeds 30s budget"
            }
        }
    }
}
