package com.rootdetector.ui

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.os.Bundle
import android.view.View
import android.widget.LinearLayout
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.rootdetector.R
import com.rootdetector.data.FixRecommendation
import com.rootdetector.databinding.ActivityReportBinding
import com.rootdetector.detect.DetectionResult
import com.rootdetector.util.FixRecommendationGenerator
import java.util.Locale

class ReportActivity : AppCompatActivity() {

    private lateinit var binding: ActivityReportBinding
    private var expandedRecommendations = mutableSetOf<String>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityReportBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // 从 Intent 获取检测结果
        val results = intent.getParcelableArrayListExtra<DetectionResult>("results")
        val reportText = intent.getStringExtra("report_text")

        // 显示检测报告
        binding.tvReportContent.text = reportText ?: "无检测报告"

        // 生成并显示修复建议
        if (results != null && results.isNotEmpty()) {
            displayFixRecommendations(results)
        } else {
            binding.tvFixTitle.visibility = View.GONE
            binding.tvFixSummary.visibility = View.GONE
            binding.fixRecommendationsContainer.visibility = View.GONE
        }
    }

    private fun displayFixRecommendations(results: List<DetectionResult>) {
        val locale = Locale.getDefault()
        val recommendations = FixRecommendationGenerator.generateRecommendations(results, locale)

        // 显示摘要
        val summary = FixRecommendationGenerator.getRecommendationSummary(recommendations, locale)
        binding.tvFixSummary.text = if (locale.language == "zh") {
            "威胁等级: $summary | 发现 ${recommendations.size} 项修复建议"
        } else {
            "Threat Level: $summary | Found ${recommendations.size} fix recommendations"
        }

        // 设置摘要颜色
        val summaryColor = when (summary) {
            "严重", "Critical" -> ContextCompat.getColor(this, android.R.color.holo_red_dark)
            "高", "High" -> ContextCompat.getColor(this, android.R.color.holo_orange_dark)
            "中", "Medium" -> ContextCompat.getColor(this, android.R.color.holo_orange_light)
            "低", "Low" -> ContextCompat.getColor(this, android.R.color.holo_green_dark)
            else -> ContextCompat.getColor(this, android.R.color.darker_gray)
        }
        binding.tvFixSummary.setTextColor(summaryColor)

        // 清空容器
        binding.fixRecommendationsContainer.removeAllViews()

        // 如果没有建议,显示安全提示
        if (recommendations.isEmpty()) {
            val safeText = FixRecommendationGenerator.formatRecommendationText(recommendations, locale)
            val tvSafe = TextView(this).apply {
                text = safeText
                setPadding(32, 32, 32, 32)
                setTextColor(ContextCompat.getColor(this@ReportActivity, android.R.color.holo_green_dark))
                textSize = 14f
            }
            binding.fixRecommendationsContainer.addView(tvSafe)
            return
        }

        // 为每个建议创建可展开的卡片
        recommendations.forEach { recommendation ->
            val cardView = createRecommendationCard(recommendation, locale)
            binding.fixRecommendationsContainer.addView(cardView)
        }
    }

    private fun createRecommendationCard(recommendation: FixRecommendation, locale: Locale): View {
        val container = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(16, 16, 16, 16)
            setBackgroundColor(0xFFF5F5F5.toInt())
            val params = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            )
            params.setMargins(0, 0, 0, 16)
            layoutParams = params
        }

        // 标题行(可点击展开)
        val titleView = TextView(this).apply {
            text = if (locale.language == "zh") {
                "▶ ${recommendation.titleZh}"
            } else {
                "▶ ${recommendation.titleEn}"
            }
            textSize = 16f
            setTextColor(0xFF333333.toInt())
            setPadding(8, 8, 8, 8)
            setBackgroundColor(0xFFE0E0E0.toInt())
            setOnClickListener {
                toggleRecommendation(recommendation.id, container, recommendation, locale)
            }
        }
        container.addView(titleView)

        // 描述
        val descView = TextView(this).apply {
            text = if (locale.language == "zh") recommendation.descriptionZh else recommendation.descriptionEn
            textSize = 14f
            setPadding(8, 12, 8, 8)
            visibility = if (expandedRecommendations.contains(recommendation.id)) View.VISIBLE else View.GONE
            tag = "desc"
        }
        container.addView(descView)

        // 步骤列表
        val stepsView = TextView(this).apply {
            val steps = if (locale.language == "zh") recommendation.stepsZh else recommendation.stepsEn
            text = steps.joinToString("\n")
            textSize = 13f
            setPadding(16, 8, 8, 8)
            visibility = if (expandedRecommendations.contains(recommendation.id)) View.VISIBLE else View.GONE
            tag = "steps"
        }
        container.addView(stepsView)

        // 复制按钮
        val copyButton = TextView(this).apply {
            text = if (locale.language == "zh") "复制修复步骤" else "Copy Fix Steps"
            textSize = 14f
            setTextColor(0xFF2196F3.toInt())
            setPadding(8, 12, 8, 12)
            visibility = if (expandedRecommendations.contains(recommendation.id)) View.VISIBLE else View.GONE
            setOnClickListener {
                copyStepsToClipboard(recommendation, locale)
            }
            tag = "copy"
        }
        container.addView(copyButton)

        return container
    }

    private fun toggleRecommendation(
        id: String,
        container: LinearLayout,
        recommendation: FixRecommendation,
        locale: Locale
    ) {
        if (expandedRecommendations.contains(id)) {
            expandedRecommendations.remove(id)
            // 折叠
            for (i in 1 until container.childCount) {
                container.getChildAt(i).visibility = View.GONE
            }
            (container.getChildAt(0) as TextView).text = if (locale.language == "zh") {
                "▶ ${recommendation.titleZh}"
            } else {
                "▶ ${recommendation.titleEn}"
            }
        } else {
            expandedRecommendations.add(id)
            // 展开
            for (i in 1 until container.childCount) {
                container.getChildAt(i).visibility = View.VISIBLE
            }
            (container.getChildAt(0) as TextView).text = if (locale.language == "zh") {
                "▼ ${recommendation.titleZh}"
            } else {
                "▼ ${recommendation.titleEn}"
            }
        }
    }

    private fun copyStepsToClipboard(recommendation: FixRecommendation, locale: Locale) {
        val stepsText = FixRecommendationGenerator.copyStepsToClipboard(recommendation, locale)
        val clipboard = getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
        val clip = ClipData.newPlainText("Fix Steps", stepsText)
        clipboard.setPrimaryClip(clip)

        Toast.makeText(
            this,
            if (locale.language == "zh") "修复步骤已复制到剪贴板" else "Fix steps copied to clipboard",
            Toast.LENGTH_SHORT
        ).show()
    }
}
