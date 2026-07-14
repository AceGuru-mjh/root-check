package com.apex.root.ui

import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.ui.Modifier
import androidx.lifecycle.lifecycleScope
import com.apex.root.core.NativeLibraryLoader
import com.apex.root.ui.theme.ApexRootTheme
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

/**
 * v1.0.4: M3 UI 入口 — 连接 Qwen 写的 ApexRootApp() Composable。
 * 保留: enableEdgeToEdge + native 预加载 + 全局异常兜底。
 */
class MainActivity : ComponentActivity() {

    companion object {
        private const val TAG = "ApexPerms"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        try {
            enableEdgeToEdge()
            setContent {
                ApexRootTheme {
                    Surface(
                        modifier = Modifier.fillMaxSize(),
                        color = MaterialTheme.colorScheme.background
                    ) {
                        ApexRootApp()
                    }
                }
            }
        } catch (e: Throwable) {
            Log.e(TAG, "Failed to set content", e)
        }

        // 预热 native 库：在 IO 线程后台加载 libapex_root.so
        lifecycleScope.launch(Dispatchers.IO) {
            try {
                NativeLibraryLoader.ensureLoaded()
            } catch (e: Throwable) {
                Log.e(TAG, "Native library preload failed (non-fatal)", e)
            }
        }
    }
}
