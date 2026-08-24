package com.fitness.management.ui.student

import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.LinearLayout
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.fitness.management.R
import com.fitness.management.data.model.FitnessRecord
import com.fitness.management.data.model.TrendItem
import com.fitness.management.data.repository.FitnessRepository
import com.fitness.management.databinding.ActivityStudentDetailBinding
import com.fitness.management.ui.teacher.TeacherHomeActivity
import com.fitness.management.utils.Constants
import com.fitness.management.utils.Extensions.toast
import com.github.mikephil.charting.charts.LineChart
import com.github.mikephil.charting.components.XAxis
import com.github.mikephil.charting.data.Entry
import com.github.mikephil.charting.data.LineData
import com.github.mikephil.charting.data.LineDataSet
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class StudentDetailActivity : AppCompatActivity() {

    private lateinit var binding: ActivityStudentDetailBinding
    private val repository = FitnessRepository()
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)
    private var loadJob: Job? = null

    private var studentId: String = ""

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityStudentDetailBinding.inflate(layoutInflater)
        setContentView(binding.root)

        studentId = intent.getStringExtra("student_id") ?: ""
        if (studentId.isEmpty()) {
            toast("学号不能为空")
            finish()
            return
        }

        setupBackButton()
        setupChart()
        setupAIReportButton()
    }

    override fun onResume() {
        super.onResume()
        loadData()
    }

    override fun onDestroy() {
        super.onDestroy()
        loadJob?.cancel()
    }

    private fun setupAIReportButton() {
        binding.cardAiReport.setOnClickListener {
            val intent = Intent(this, AIReportActivity::class.java)
            intent.putExtra("student_id", studentId)
            intent.putExtra("student_name", intent.getStringExtra("student_name") ?: "")
            startActivity(intent)
        }
    }

    private fun setupBackButton() {
        binding.btnBack.setOnClickListener {
            finish()
        }
    }

    private fun setupChart() {
        binding.chartTrend.description.isEnabled = false
        binding.chartTrend.setTouchEnabled(false)
        binding.chartTrend.setPinchZoom(false)
        binding.chartTrend.legend.isEnabled = false
        binding.chartTrend.xAxis.position = XAxis.XAxisPosition.BOTTOM
        binding.chartTrend.xAxis.setDrawGridLines(false)
        binding.chartTrend.axisRight.isEnabled = false
        binding.chartTrend.axisLeft.setDrawGridLines(true)
        binding.chartTrend.axisLeft.gridColor = Color.parseColor("#F3F4F6")
        binding.chartTrend.animateX(500)
    }

    private fun loadData() {
        loadJob?.cancel()
        loadJob = scope.launch {
            try {
                val records = withContext(Dispatchers.IO) {
                    repository.getStudentRecords(studentId)
                }

                records.fold(
                    onSuccess = { recordList ->
                        if (recordList.isEmpty()) {
                            toast("暂无体测记录")
                            return@launch
                        }
                        updateScoreSection(recordList)
                        updateScoreList(recordList)
                        generateAdvice(recordList)
                    },
                    onFailure = { error ->
                        toast("加载失败: ${error.message}")
                    }
                )

                // Load trend data
                val trend = withContext(Dispatchers.IO) {
                    repository.getStudentTrend(studentId, "jump_rope")
                }
                trend.fold(
                    onSuccess = { trendItems ->
                        updateTrendChart(trendItems)
                    },
                    onFailure = { /* Chart empty state handled */ }
                )

            } catch (e: Exception) {
                toast("加载失败: ${e.message}")
            }
        }
    }

    private data class TypeScoreInfo(
        val testType: String,
        val label: String,
        val value: String,
        val score: Int?
    )

    private fun updateScoreSection(records: List<FitnessRecord>) {
        val latestByType = records
            .filter { it.score != null }
            .groupBy { it.testType }
            .mapValues { (_, typeRecords) ->
                typeRecords.maxByOrNull { it.testDate }!!
            }
            .values.toList()

        if (latestByType.isEmpty()) {
            binding.tvBigScore.text = "--"
            binding.tvGradeLabel.text = "暂无数据"
            return
        }

        val avgScore = latestByType.mapNotNull { it.score }.average().toInt()
        binding.tvBigScore.text = avgScore.toString()

        val grade = when {
            avgScore >= 90 -> "优秀"
            avgScore >= 80 -> "良好"
            avgScore >= 60 -> "及格"
            else -> "不及格"
        }
        binding.tvGradeLabel.text = grade

        val gradeRes = when {
            avgScore >= 90 -> R.drawable.bg_grade_green
            avgScore >= 80 -> R.drawable.bg_grade_blue
            avgScore >= 60 -> R.drawable.bg_grade_orange
            else -> R.drawable.bg_grade_red
        }
        binding.tvGradeLabel.setBackgroundResource(gradeRes)
    }

    private fun updateScoreList(records: List<FitnessRecord>) {
        binding.llScoreItems.removeAllViews()

        val latestByType = records
            .groupBy { it.testType }
            .mapValues { (_, typeRecords) ->
                typeRecords.maxByOrNull { it.testDate }!!
            }
            .values
            .sortedBy { it.testType }

        val scoreInfos = latestByType.map { record ->
            val label = Constants.TEST_TYPE_LABELS[record.testType] ?: record.testType
            val value = getRecordValue(record)
            TypeScoreInfo(record.testType, label, value, record.score)
        }

        for ((index, info) in scoreInfos.withIndex()) {
            val itemView = createScoreItemView(info.label, info.value, info.score)
            binding.llScoreItems.addView(itemView)

            // Add divider except for last item
            if (index < scoreInfos.size - 1) {
                val divider = View(this).apply {
                    layoutParams = LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT, 1
                    )
                    setBackgroundColor(Color.parseColor("#F3F4F6"))
                }
                binding.llScoreItems.addView(divider)
            }
        }
    }

    private fun getRecordValue(record: FitnessRecord): String {
        return when (record.testType) {
            "jump_rope" -> "${record.count ?: 0}次"
            "50m_run", "100m_run" -> "%.1fs".format(record.timeSec ?: 0f)
            "800m_run", "1000m_run" -> {
                val sec = record.timeSec ?: 0f
                val minutes = (sec / 60).toInt()
                val remainingSec = (sec % 60).toInt()
                "${minutes}'${remainingSec}\""
            }
            "long_jump" -> "%.2fm".format(record.distanceM ?: 0f)
            "sit_ups" -> "${record.count ?: 0}次"
            "pull_up" -> "${record.count ?: 0}次"
            "sit_reach" -> "%.1fcm".format(record.distanceM ?: 0f)
            else -> "${record.count ?: record.distanceM ?: record.timeSec ?: "-"}"
        }
    }

    private fun createScoreItemView(projectName: String, projectValue: String, score: Int?): View {
        val container = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            setPadding(0, 12, 0, 12)
        }

        val tvProject = TextView(this).apply {
            text = projectName
            setTextColor(ContextCompat.getColor(context, R.color.text_primary))
            textSize = 14f
            layoutParams = LinearLayout.LayoutParams(
                0,
                ViewGroup.LayoutParams.WRAP_CONTENT,
                1f
            )
        }
        container.addView(tvProject)

        val tvValue = TextView(this).apply {
            text = projectValue
            setTextColor(ContextCompat.getColor(context, R.color.text_secondary))
            textSize = 13f
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { marginEnd = 16 }
        }
        container.addView(tvValue)

        if (score != null) {
            val tvScoreBadge = TextView(this).apply {
                text = score.toString()
                setTextColor(ContextCompat.getColor(context, R.color.white))
                textSize = 14f
                gravity = Gravity.CENTER
                val scoreRes = when {
                    score >= 90 -> R.drawable.bg_score_green
                    score >= 80 -> R.drawable.bg_score_blue
                    score >= 60 -> R.drawable.bg_score_orange
                    else -> R.drawable.bg_score_red
                }
                setBackgroundResource(scoreRes)
                minWidth = 40
                setPadding(10, 4, 10, 4)
            }
            container.addView(tvScoreBadge)
        }

        return container
    }

    private fun updateTrendChart(trendItems: List<TrendItem>) {
        if (trendItems.isEmpty()) {
            binding.chartTrend.clear()
            binding.chartTrend.setNoDataText("暂无趋势数据")
            return
        }

        val entries = trendItems.mapIndexed { index, item ->
            Entry(index.toFloat(), (item.score ?: 0).toFloat())
        }

        val dataSet = LineDataSet(entries, "成绩").apply {
            color = Color.parseColor("#2E9E5A")
            valueTextColor = Color.parseColor("#6B7280")
            valueTextSize = 10f
            lineWidth = 2f
            setCircleColor(Color.parseColor("#2E9E5A"))
            circleRadius = 4f
            setDrawFilled(true)
            setFillColor(Color.parseColor("#10014DB2"))
            mode = LineDataSet.Mode.CUBIC_BEZIER
        }

        val lineData = LineData(dataSet)
        binding.chartTrend.data = lineData
        binding.chartTrend.invalidate()
    }

    private fun generateAdvice(records: List<FitnessRecord>) {
        val latestByType = records
            .filter { it.score != null }
            .groupBy { it.testType }
            .mapValues { (_, typeRecords) ->
                typeRecords.maxByOrNull { it.testDate }!!
            }
            .values.toList()

        if (latestByType.isEmpty()) {
            binding.tvAdvice.text = "暂无运动数据，建议完成一次体测后再查看建议。"
            return
        }

        val scored = latestByType.sortedBy { it.score }
        val lowest = scored.firstOrNull()
        val highest = scored.lastOrNull()

        val weakestLabel = lowest?.let { Constants.TEST_TYPE_LABELS[it.testType] ?: it.testType } ?: ""
        val strongestLabel = highest?.let { Constants.TEST_TYPE_LABELS[it.testType] ?: it.testType } ?: ""

        val advice = buildString {
            append("根据您的体测成绩分析：\n\n")
            lowest?.let { l ->
                val label = Constants.TEST_TYPE_LABELS[l.testType] ?: l.testType
                append("\u2022 您的${label}成绩相对较弱（${l.score}分），建议加强该项目的专项训练，")
                append(getAdviceForType(l.testType, low = true))
                append("\n\n")
            }
            highest?.let { h ->
                val label = Constants.TEST_TYPE_LABELS[h.testType] ?: h.testType
                append("\u2022 您的${label}表现优秀（${h.score}分），请继续保持！")
                append("\n\n")
            }
            append("建议每周至少进行3-4次体育锻炼，均衡发展各项体能指标。注意运动前后的热身与拉伸，预防运动损伤。")
        }
        binding.tvAdvice.text = advice
    }

    private fun getAdviceForType(testType: String, low: Boolean): String {
        return when (testType) {
            "jump_rope" -> if (low) "建议每天练习跳绳500-1000次，提高协调性和耐力。" else ""
            "50m_run", "100m_run" -> if (low) "建议进行短距离冲刺训练，提高爆发力。" else ""
            "800m_run", "1000m_run" -> if (low) "建议每周进行2-3次慢跑训练，增强心肺耐力。" else ""
            "long_jump" -> if (low) "建议进行腿部力量训练和弹跳练习。" else ""
            "sit_ups" -> if (low) "建议每天进行3组15-20个仰卧起坐练习。" else ""
            "pull_up" -> if (low) "建议进行辅助引体向上和手臂力量训练。" else ""
            "sit_reach" -> if (low) "建议每天进行坐位体前屈拉伸，提高身体柔韧性。" else ""
            else -> if (low) "建议加强该项目的针对性训练。" else ""
        }
    }
}
