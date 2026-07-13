package com.apex.root.ui

import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.ui.Modifier
import androidx.lifecycle.lifecycleScope
import com.apex.root.core.NativeLibraryLoader
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

/**
 * v1.0.3: 最小化 MainActivity — 仅作为 Compose 入口。
 * 旧 UI 代码已全部删除,等待新 M3 UI 重写。
 *
 * 保留的业务逻辑:
 *  - enableEdgeToEdge()
 *  - native 库预加载 (IO 线程后台加载 libapex_root.so)
 *  - 全局异常兜底
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
                Surface(modifier = Modifier.fillMaxSize()) {
                    // v1.0.3: 占位 — 等待新 M3 UI 重写
                    Text("Apex Agent v1.0.3 — UI 重构中")
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
