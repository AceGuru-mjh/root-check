package com.apex.root.island

import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.apex.root.R

class ModeSwitchActivity : AppCompatActivity() {

    private lateinit var manager: HideModeManager
    private lateinit var statusText: TextView
    private lateinit var btnDetect: Button
    private lateinit var btnHide: Button
    private lateinit var btnGame: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_mode_switch)

        manager = HideModeManager(this)
        statusText = findViewById(R.id.status_text)
        btnDetect = findViewById(R.id.btn_detect)
        btnHide = findViewById(R.id.btn_hide)
        btnGame = findViewById(R.id.btn_game)

        btnDetect.setOnClickListener { switchMode(HideModeManager.MODE_DETECT) }
        btnHide.setOnClickListener { switchMode(HideModeManager.MODE_HIDE) }
        btnGame.setOnClickListener { switchMode(HideModeManager.MODE_GAME) }

        refreshStatus()
    }

    override fun onResume() {
        super.onResume()
        refreshStatus()
    }

    private fun switchMode(mode: Int) {
        val ok = manager.switchToMode(mode)
        val label = when (mode) {
            HideModeManager.MODE_DETECT -> "Detection"
            HideModeManager.MODE_HIDE -> "Hide"
            HideModeManager.MODE_GAME -> "Game"
            else -> "Unknown"
        }
        Toast.makeText(this, "$label mode: ${if (ok) "ON" else "FAILED"}", Toast.LENGTH_SHORT).show()
        refreshStatus()
    }

    private fun refreshStatus() {
        val mode = manager.currentMode()
        val active = manager.isActive()
        val modeName = when (mode) {
            HideModeManager.MODE_DETECT -> "Detection"
            HideModeManager.MODE_HIDE -> "Hide"
            HideModeManager.MODE_GAME -> "Game"
            else -> "Unknown"
        }
        statusText.text = """
            Mode: $modeName
            Active: ${if (active) "YES" else "NO"}
            
            Detection-only devices see: clean system
            APEX-Root sees: real Root status
        """.trimIndent()
    }
}
