package com.apex.root.ui

import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.Surface
import androidx.compose.ui.Modifier
import androidx.lifecycle.lifecycleScope
import com.apex.root.core.NativeLibraryLoader
import com.apex.root.ui.compose.AppNavigation
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

class MainActivity : ComponentActivity() {

    companion object {
        private const val TAG = "ApexPerms"
    }

    /*
     * 修复：移除了原先在 onCreate 中直接请求权限的 requestNecessaryPermissionsOnce()。
     *
     * 原实现的问题：
     *   1. 在 Activity.onCreate 中立即调用 permissionLauncher.launch()，此时应用窗口
     *      尚未完全显示，系统权限对话框可能被 Compose 内容覆盖或与首启动动画冲突。
     *   2. 设置 requested_once=true 后，即使权限被拒绝也永不再请求 —— 返回用户
     *      永远看不到权限弹窗。
     *   3. 与 GlassPermissionGuideScreen 中的权限请求重复，可能导致 Android 权限
     *      节流（连续请求同一权限时系统会不显示对话框）。
     *
     * 现在的流程：
     *   - 首次启动 → SplashScreen → GlassPermissionGuideScreen
     *     → 在引导页中通过 LaunchedEffect 可靠地弹出系统权限对话框
     *     → 用户点击"进入应用"后 completePermissionGuide() 标记完成
     *   - 返回用户 → 直接进入 MainApp
     *
     * 权限请求全部由 GlassPermissionGuideScreen 统一管理，确保首次进入必有弹窗。
     */

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // 全局异常兜底：若 setContent 前出现未捕获异常，至少记录而不至于黑屏
        try {
            enableEdgeToEdge()
            setContent {
                Surface(modifier = Modifier.fillMaxSize()) {
                    AppNavigation()
                }
            }
        } catch (e: Throwable) {
            Log.e(TAG, "Failed to set content", e)
        }

        // 预热 native 库：在 IO 线程后台加载 libapex_root.so
        // 避免首次扫描时在主线程触发 dlopen 导致卡顿
        // 借鉴 topjohnwu/Magisk 的预加载策略
        lifecycleScope.launch(Dispatchers.IO) {
            try {
                NativeLibraryLoader.ensureLoaded()
            } catch (e: Throwable) {
                Log.e(TAG, "Native library preload failed (non-fatal)", e)
            }
        }
    }
}
