package com.apex.root.ui

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.Surface
import androidx.compose.ui.Modifier
import androidx.core.content.ContextCompat
import com.apex.root.ui.compose.AppNavigation

class MainActivity : ComponentActivity() {

    companion object {
        private const val TAG = "ApexPerms"
    }

    /**
     * 权限请求 Launcher — 即使回调为空也必须注册（在 STARTED 之前）。
     * 回调中记录授权结果，便于诊断。
     */
    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { results ->
        results.forEach { (perm, granted) ->
            Log.i(TAG, "Permission result: $perm granted=$granted")
        }
    }

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

        // 首次安装时统一请求运行时权限（仅请求一次，避免每次 onResume 重复弹窗）
        requestNecessaryPermissionsOnce()
    }

    /**
     * 仅在首次启动时请求必要权限。
     * - 通知权限（Android 13+）：用于推送扫描结果与安全告警
     * 首次启动引导页（GlassPermissionGuideScreen）会再次以可视化方式引导用户授权，
     * 此处仅作为返回用户（已完成引导）时的兜底请求。
     */
    private fun requestNecessaryPermissionsOnce() {
        try {
            val prefs = getSharedPreferences("apex_perms", MODE_PRIVATE)
            if (prefs.getBoolean("requested_once", false)) return

            val perms = mutableListOf<String>()
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                if (ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS)
                    != PackageManager.PERMISSION_GRANTED) {
                    perms.add(Manifest.permission.POST_NOTIFICATIONS)
                }
            }
            if (perms.isNotEmpty()) {
                Log.i(TAG, "Requesting runtime permissions: $perms")
                permissionLauncher.launch(perms.toTypedArray())
            }
            prefs.edit().putBoolean("requested_once", true).apply()
        } catch (e: Throwable) {
            Log.e(TAG, "Failed to request permissions", e)
        }
    }
}
