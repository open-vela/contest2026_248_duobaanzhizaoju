package com.fitness.management.ui.admin

import android.graphics.Color
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.widget.ArrayAdapter
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.fitness.management.R
import com.fitness.management.data.model.ClassInfo
import com.fitness.management.data.model.ClassStats
import com.fitness.management.data.model.TestTypeStat
import com.fitness.management.data.repository.FitnessRepository
import com.fitness.management.databinding.ActivityReportExportBinding
import com.fitness.management.utils.Constants
import com.fitness.management.utils.Extensions.toast
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class ReportExportActivity : AppCompatActivity() {

    private lateinit var binding: ActivityReportExportBinding
    private val repository = FitnessRepository()
    private var classes = listOf<ClassInfo>()
    private var selectedClassId: Int? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityReportExportBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.btnBack.setOnClickListener { finish() }

        setupSpinner()
        loadClasses()

        binding.btnExport.setOnClickListener {
            selectedClassId?.let { loadReport(it) }
                ?: toast("请先选择班级")
        }
    }

    private fun setupSpinner() {
        binding.spinnerClass.setOnItemSelectedListener(object : android.widget.AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: android.widget.AdapterView<*>?, view: View?, position: Int, id: Long) {
                selectedClassId = if (position > 0 && classes.isNotEmpty()) classes[position - 1].id else null
                binding.layoutReport.visibility = View.GONE
            }
            override fun onNothingSelected(parent: android.widget.AdapterView<*>?) {
                selectedClassId = null
            }
        })
    }

    private fun loadClasses() {
        lifecycleScope.launch {
            try {
                val result = repository.getAllClasses()
                result.onSuccess { classList ->
                    classes = classList
                    val names = mutableListOf("请选择班级")
                    names.addAll(classList.map { "${it.grade}${it.className}" })
                    val adapter = ArrayAdapter(this@ReportExportActivity, android.R.layout.simple_spinner_item, names)
                    adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
                    binding.spinnerClass.adapter = adapter
                }
                result.onFailure { toast("加载班级失败") }
            } catch (e: Exception) {
                toast("网络错误: ${e.message}")
            }
        }
    }

    private fun loadReport(classId: Int) {
        binding.progressLoading.visibility = View.VISIBLE
        binding.layoutReport.visibility = View.GONE

        lifecycleScope.launch {
            try {
                val statsDeferred = async { repository.getClassStatistics(classId) }
                val studentsDeferred = async { repository.getMyStudents(classId) }

                val statsResult = statsDeferred.await()
                val studentsResult = studentsDeferred.await()

                binding.progressLoading.visibility = View.GONE

                val stats = statsResult.getOrNull()
                if (stats == null) {
                    toast("加载统计失败")
                    return@launch
                }

                val className = classes.find { it.id == classId }
                val displayName = className?.let { "${it.grade}${it.className}" } ?: "未知班级"
                val studentCount = studentsResult.getOrNull()?.size ?: 0

                withContext(Dispatchers.Main) {
                    renderReport(displayName, stats, studentCount)
                }
            } catch (e: Exception) {
                binding.progressLoading.visibility = View.GONE
                toast("加载失败: ${e.message}")
            }
        }
    }

    private fun renderReport(className: String, stats: ClassStats, studentCount: Int) {
        binding.layoutReport.visibility = View.VISIBLE

        // Header
        binding.tvReportTitle.text = "${className}体测报告"
        val dateFormat = SimpleDateFormat("yyyy年M月d日 HH:mm", Locale.CHINESE)
        binding.tvReportDate.text = "生成时间：${dateFormat.format(Date())}"

        // Overview
        binding.tvStatStudents.text = "${stats.totalStudents}"
        binding.tvStatTested.text = "${stats.testedStudents}"
        binding.tvStatAvgScore.text = String.format("%.1f", stats.avgScore)

        val passRate = (stats.passRate * 100).toInt()
        binding.progressPassRate.progress = passRate
        binding.tvPassRate.text = "${passRate}%"

        val excellentRate = (stats.excellentRate * 100).toInt()
        binding.progressExcellentRate.progress = excellentRate
        binding.tvExcellentRate.text = "${excellentRate}%"

        // Score Distribution
        val dist = calculateDistribution(stats)
        binding.progressDistExcellent.progress = dist[0]
        binding.tvDistExcellent.text = "${dist[0]}%"
        binding.progressDistGood.progress = dist[1]
        binding.tvDistGood.text = "${dist[1]}%"
        binding.progressDistPass.progress = dist[2]
        binding.tvDistPass.text = "${dist[2]}%"
        binding.progressDistFail.progress = dist[3]
        binding.tvDistFail.text = "${dist[3]}%"

        // Per Test Type Analysis
        renderTestTypeAnalysis(stats.testTypeStats)

        // Recommendations
        renderRecommendations(stats, dist)
    }

    private fun calculateDistribution(stats: ClassStats): IntArray {
        val total = stats.totalStudents
        if (total == 0) return intArrayOf(0, 0, 0, 0)

        val excellent = ((stats.excellentRate) * total).toInt()
        val passTotal = ((stats.passRate) * total).toInt()
        val good = ((passTotal - excellent) * 0.4).toInt().coerceAtLeast(0)
        val pass = passTotal - excellent - good
        val fail = total - passTotal

        val sum = excellent + good + pass + fail
        return if (sum > 0) {
            intArrayOf(
                excellent * 100 / sum,
                good * 100 / sum,
                pass * 100 / sum,
                fail * 100 / sum
            )
        } else intArrayOf(0, 0, 0, 0)
    }

    private fun renderTestTypeAnalysis(testTypeStats: List<TestTypeStat>) {
        binding.containerTestTypes.removeAllViews()

        if (testTypeStats.isEmpty()) {
            val tv = TextView(this).apply {
                text = "暂无各项目数据"
                setTextColor(resources.getColor(R.color.text_hint, null))
                textSize = 13f
                setPadding(0, 16, 0, 16)
            }
            binding.containerTestTypes.addView(tv)
            return
        }

        for (stat in testTypeStats) {
            val card = androidx.cardview.widget.CardView(this).apply {
                layoutParams = LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    LinearLayout.LayoutParams.WRAP_CONTENT
                ).apply { bottomMargin = 12 }
                setCardBackgroundColor(resources.getColor(R.color.white, null))
                radius = 14f * resources.displayMetrics.density
                cardElevation = 0f
            }

            val content = LinearLayout(this).apply {
                orientation = LinearLayout.VERTICAL
                setPadding(24, 20, 24, 20)
            }

            // Header row
            val header = LinearLayout(this).apply {
                orientation = LinearLayout.HORIZONTAL
                gravity = android.view.Gravity.CENTER_VERTICAL
            }

            val label = stat.label.ifEmpty {
                Constants.TEST_TYPE_LABELS[stat.testType] ?: stat.testType
            }

            val tvName = TextView(this).apply {
                text = label
                setTextColor(resources.getColor(R.color.text_primary, null))
                textSize = 14f
                setTypeface(null, android.graphics.Typeface.BOLD)
                layoutParams = LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f)
            }

            val tvScore = TextView(this).apply {
                text = String.format("%.1f分", stat.avgScore)
                setTextColor(getScoreColor(stat.avgScore))
                textSize = 18f
                setTypeface(null, android.graphics.Typeface.BOLD)
            }

            header.addView(tvName)
            header.addView(tvScore)
            content.addView(header)

            // Detail row
            val detail = LinearLayout(this).apply {
                orientation = LinearLayout.HORIZONTAL
                setPadding(0, 8, 0, 0)
            }

            val tvCount = TextView(this).apply {
                text = "测试人数：${stat.count}"
                setTextColor(resources.getColor(R.color.text_secondary, null))
                textSize = 12f
                layoutParams = LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f)
            }

            val tvRange = TextView(this).apply {
                text = "最高${stat.maxScore}分 / 最低${stat.minScore}分"
                setTextColor(resources.getColor(R.color.text_hint, null))
                textSize = 12f
            }

            detail.addView(tvCount)
            detail.addView(tvRange)
            content.addView(detail)

            // Score bar
            val barBg = LinearLayout(this).apply {
                layoutParams = LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    (4 * resources.displayMetrics.density).toInt()
                ).apply { topMargin = 12 }
                setBackgroundColor(resources.getColor(R.color.bg_divider, null))
            }

            val barFill = View(this).apply {
                layoutParams = LinearLayout.LayoutParams(
                    0,
                    LinearLayout.LayoutParams.MATCH_PARENT
                )
                setBackgroundColor(getScoreColor(stat.avgScore))
            }

            // Calculate fill percentage (score out of 100)
            val fillWidth = (stat.avgScore / 100.0 * 100).toInt().coerceIn(0, 100)
            barFill.layoutParams = LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT, fillWidth.toFloat())
            barBg.addView(barFill)
            content.addView(barBg)

            // Level assessment
            val level = when {
                stat.avgScore >= 90 -> "优秀 - 表现突出，继续保持"
                stat.avgScore >= 80 -> "良好 - 水平较高，仍有提升空间"
                stat.avgScore >= 60 -> "及格 - 需要加强训练"
                else -> "不及格 - 需重点关注和辅导"
            }

            val tvLevel = TextView(this).apply {
                text = level
                setTextColor(getScoreColor(stat.avgScore))
                textSize = 12f
                setPadding(0, 8, 0, 0)
            }
            content.addView(tvLevel)

            card.addView(content)
            binding.containerTestTypes.addView(card)
        }
    }

    private fun getScoreColor(score: Double): Int {
        return when {
            score >= 90 -> resources.getColor(R.color.success_green, null)
            score >= 80 -> resources.getColor(R.color.brand_blue, null)
            score >= 60 -> resources.getColor(R.color.warning_orange, null)
            else -> resources.getColor(R.color.danger_red, null)
        }
    }

    private fun renderRecommendations(stats: ClassStats, dist: IntArray) {
        val sb = StringBuilder()
        var index = 1

        // Overall assessment
        when {
            stats.avgScore >= 85 -> sb.appendLine("${index}. 班级整体表现优秀，平均分${String.format("%.1f", stats.avgScore)}分，请继续保持良好训练状态。")
            stats.avgScore >= 70 -> sb.appendLine("${index}. 班级整体表现良好，平均分${String.format("%.1f", stats.avgScore)}分，建议针对薄弱项目加强训练。")
            stats.avgScore >= 60 -> sb.appendLine("${index}. 班级整体处于及格水平，平均分${String.format("%.1f", stats.avgScore)}分，需要系统性提升。")
            else -> sb.appendLine("${index}. 班级整体成绩偏低，平均分${String.format("%.1f", stats.avgScore)}分，建议制定专项提升计划。")
        }
        index++

        // Pass rate advice
        val passRate = (stats.passRate * 100).toInt()
        if (passRate < 90) {
            sb.appendLine("${index}. 及格率为${passRate}%，低于90%目标，建议对不及格学生(${dist[3]}%)进行一对一辅导。")
            index++
        }

        // Excellent rate advice
        val excellentRate = (stats.excellentRate * 100).toInt()
        if (excellentRate < 30) {
            sb.appendLine("${index}. 优秀率为${excellentRate}%，建议通过增加训练强度提升优秀率。")
            index++
        }

        // Weak test types
        val weakTypes = stats.testTypeStats.filter { it.avgScore < 70 }
        if (weakTypes.isNotEmpty()) {
            val names = weakTypes.joinToString("、") {
                it.label.ifEmpty { Constants.TEST_TYPE_LABELS[it.testType] ?: it.testType }
            }
            sb.appendLine("${index}. 薄弱项目：${names}，建议增加专项训练频次。")
            index++
        }

        // Strong test types
        val strongTypes = stats.testTypeStats.filter { it.avgScore >= 85 }
        if (strongTypes.isNotEmpty()) {
            val names = strongTypes.joinToString("、") {
                it.label.ifEmpty { Constants.TEST_TYPE_LABELS[it.testType] ?: it.testType }
            }
            sb.appendLine("${index}. 优势项目：${names}，可以作为榜样带动其他项目。")
            index++
        }

        // Score gap advice
        val maxMinGap = stats.testTypeStats.maxOfOrNull { it.maxScore - it.minScore } ?: 0
        if (maxMinGap > 40) {
            sb.appendLine("${index}. 各项目最高分与最低分差距较大(${maxMinGap}分)，建议关注体能较弱的学生，实施分层教学。")
            index++
        }

        // Testing coverage
        if (stats.testedStudents < stats.totalStudents) {
            val untested = stats.totalStudents - stats.testedStudents
            sb.appendLine("${index}. 还有${untested}名学生未完成测试，请及时安排补测。")
        }

        binding.tvRecommendations.text = sb.toString()
    }
}
