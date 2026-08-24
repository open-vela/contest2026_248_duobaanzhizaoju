package com.fitness.management.ui.teacher

import android.annotation.SuppressLint
import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.view.View
import android.widget.ImageView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import com.fitness.management.R
import com.fitness.management.data.model.LeaderboardItem
import com.fitness.management.data.repository.FitnessRepository
import com.fitness.management.databinding.ActivityTeacherHomeBinding
import com.fitness.management.utils.Constants
import com.fitness.management.utils.Extensions.toast
import com.fitness.management.utils.SessionManager
import com.github.mikephil.charting.charts.LineChart
import com.github.mikephil.charting.components.XAxis
import com.github.mikephil.charting.data.Entry
import com.github.mikephil.charting.data.LineData
import com.github.mikephil.charting.data.LineDataSet
import com.github.mikephil.charting.formatter.IndexAxisValueFormatter
import kotlinx.coroutines.launch
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class TeacherHomeActivity : AppCompatActivity() {

    private lateinit var binding: ActivityTeacherHomeBinding
    private val repository = FitnessRepository()

    @SuppressLint("SetTextI18n")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityTeacherHomeBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val realName = SessionManager.realName ?: "老师"
        binding.tvGreeting.text = "你好，${realName}"
        binding.tvAvatarInitial.text = if (realName.isNotEmpty()) realName.first().toString() else "师"

        val dateFormat = SimpleDateFormat("yyyy年M月d日 EEEE", Locale.CHINESE)
        binding.tvDate.text = dateFormat.format(Date())

        setupBottomNav()
    }

    override fun onResume() {
        super.onResume()
        loadData()
    }

    private fun loadData() {
        showLoading(true)

        lifecycleScope.launch {
            try {
                // Load overview stats
                val overviewResult = repository.getOverview()
                overviewResult.onSuccess { overview ->
                    binding.cardClassProgress.visibility = View.VISIBLE

                    // Get class info
                    val classesResult = repository.getMyClasses()
                    classesResult.onSuccess { classes ->
                        if (classes.isNotEmpty()) {
                            val cls = classes.first()
                            binding.tvClassName.text = "${cls.grade}${cls.className}"
                            binding.tvSemester.text = "2026春季学期"
                        }
                    }

                    // Get total student count
                    val studentsResult = repository.getMyStudents()
                    studentsResult.onSuccess { students ->
                        val testedRate = if (overview.totalStudents > 0) {
                            (overview.totalRecords.toFloat() / overview.totalStudents * 100).toInt()
                        } else 0
                        binding.tvCompletionText.text = "${overview.totalRecords}/${overview.totalStudents} 人完成"
                        binding.tvPercentage.text = "${testedRate}%"
                        binding.progressClass.progress = testedRate
                    }

                    // Setup trend chart
                    setupTrendChart(overview.avgScore)
                }

                // Load pending test items
                loadPendingTestItems()

                // Load leaderboard
                val leaderboardResult = repository.getLeaderboard()
                leaderboardResult.onSuccess { leaderboard ->
                    if (leaderboard.isNotEmpty()) {
                        binding.cardStars.visibility = View.VISIBLE
                        populateStars(leaderboard.take(3))
                    } else {
                        binding.cardStars.visibility = View.VISIBLE
                        binding.tvEmptyStars.visibility = View.VISIBLE
                    }
                }

                showLoading(false)

            } catch (e: Exception) {
                showLoading(false)
                toast("数据加载失败: ${e.message}")
            }
        }
    }

    private suspend fun loadPendingTestItems() {
        binding.cardPendingTests.visibility = View.VISIBLE
        binding.containerTestItems.removeAllViews()

        try {
            val studentsResult = repository.getMyStudents()
            val recordsResult = repository.listRecords()

            val students = studentsResult.getOrNull() ?: emptyList()
            val records = recordsResult.getOrNull() ?: emptyList()

            val totalStudents = students.size
            if (totalStudents == 0) {
                toast("暂无学生数据")
                return
            }

            // Count records per test type
            val typeCounts = mutableMapOf<String, Int>()
            for (record in records) {
                typeCounts[record.testType] = (typeCounts[record.testType] ?: 0) + 1
            }

            val dotColors = listOf(
                R.drawable.ic_dot_blue,
                R.drawable.ic_dot_green,
                R.drawable.ic_dot_orange,
                R.drawable.ic_dot_purple
            )

            var colorIndex = 0
            for ((testType, count) in typeCounts) {
                val label = Constants.TEST_TYPE_LABELS[testType] ?: testType
                val completion = "$count/$totalStudents 完成"
                val dotRes = dotColors[colorIndex % dotColors.size]
                colorIndex++

                val row = layoutInflater.inflate(R.layout.item_test_row, binding.containerTestItems, false)
                row.findViewById<ImageView>(R.id.iv_dot).setImageResource(dotRes)
                row.findViewById<TextView>(R.id.tv_test_name).text = label
                row.findViewById<TextView>(R.id.tv_test_completion).text = completion
                binding.containerTestItems.addView(row)
            }

            if (typeCounts.isEmpty()) {
                toast("暂无测试记录")
            }
        } catch (e: Exception) {
            toast("加载测试数据失败")
        }
    }

    private suspend fun setupTrendChart(avgScore: Double) {
        binding.cardTrend.visibility = View.VISIBLE

        val chart = binding.lineChart
        val entries = mutableListOf<Entry>()
        val labels = mutableListOf<String>()

        // Try to load real trend data from first class
        try {
            val classesResult = repository.getMyClasses()
            val classes = classesResult.getOrNull() ?: emptyList()
            if (classes.isNotEmpty()) {
                val studentsResult = repository.getMyStudents(classId = classes.first().id)
                val students = studentsResult.getOrNull() ?: emptyList()
                if (students.isNotEmpty()) {
                    val trendResult = repository.getStudentTrend(students.first().studentId)
                    val trendItems = trendResult.getOrNull() ?: emptyList()
                    if (trendItems.isNotEmpty()) {
                        for (i in trendItems.indices) {
                            val item = trendItems[i]
                            entries.add(Entry(i.toFloat(), (item.score ?: 0).toFloat()))
                            labels.add(item.testDate.takeLast(5))
                        }
                        val dataSet = LineDataSet(entries, "成绩趋势").apply {
                            color = ContextCompat.getColor(this@TeacherHomeActivity, R.color.brand_blue)
                            setCircleColor(ContextCompat.getColor(this@TeacherHomeActivity, R.color.brand_blue))
                            lineWidth = 2f
                            circleRadius = 3f
                            setDrawValues(false)
                            setDrawFilled(true)
                            fillColor = ContextCompat.getColor(this@TeacherHomeActivity, R.color.brand_blue_light)
                            mode = LineDataSet.Mode.CUBIC_BEZIER
                        }
                        chart.data = LineData(dataSet)
                        setupChartAxes(chart, labels)
                        return
                    }
                }
            }
        } catch (_: Exception) { }

        // Fallback: use avgScore to generate demo data
        val baseScore = avgScore.toFloat()
        val demoValues = floatArrayOf(
            baseScore - 8f, baseScore - 5f, baseScore - 3f,
            baseScore - 1f, baseScore + 1f, baseScore + 3f, baseScore + 5f
        )
        val demoLabels = arrayOf("第1周", "第2周", "第3周", "第4周", "第5周", "第6周", "第7周")

        for (i in demoValues.indices) {
            entries.add(Entry(i.toFloat(), demoValues[i].coerceAtLeast(50f)))
            labels.add(demoLabels[i])
        }

        val dataSet = LineDataSet(entries, "班级平均分").apply {
            color = ContextCompat.getColor(this@TeacherHomeActivity, R.color.brand_blue)
            setCircleColor(ContextCompat.getColor(this@TeacherHomeActivity, R.color.brand_blue))
            lineWidth = 2f
            circleRadius = 3f
            setDrawValues(false)
            setDrawFilled(true)
            fillColor = ContextCompat.getColor(this@TeacherHomeActivity, R.color.brand_blue_light)
            mode = LineDataSet.Mode.CUBIC_BEZIER
        }

        chart.data = LineData(dataSet)
        setupChartAxes(chart, labels)
    }

    private fun setupChartAxes(chart: LineChart, labels: List<String>) {
        chart.xAxis.apply {
            position = XAxis.XAxisPosition.BOTTOM
            setDrawGridLines(false)
            textSize = 10f
            textColor = ContextCompat.getColor(this@TeacherHomeActivity, R.color.text_secondary)
            labelCount = labels.size
            valueFormatter = IndexAxisValueFormatter(labels)
            granularity = 1f
        }

        chart.axisLeft.apply {
            axisMinimum = 40f
            axisMaximum = 110f
            textColor = ContextCompat.getColor(this@TeacherHomeActivity, R.color.text_secondary)
            textSize = 10f
            setDrawGridLines(true)
            gridColor = Color.parseColor("#F3F4F6")
        }
        chart.axisRight.isEnabled = false

        chart.apply {
            description.isEnabled = false
            legend.isEnabled = false
            setScaleEnabled(false)
            setPinchZoom(false)
            isDoubleTapToZoomEnabled = false
            animateX(500)
            invalidate()
        }
    }

    private fun populateStars(leaderboard: List<LeaderboardItem>) {
        binding.containerStars.removeAllViews()
        binding.tvEmptyStars.visibility = View.GONE

        val rankColors = listOf(
            ContextCompat.getColor(this, R.color.warning_orange), // Gold
            Color.parseColor("#9CA3AF"), // Silver
            Color.parseColor("#D97706")  // Bronze
        )

        for (item in leaderboard) {
            val row = layoutInflater.inflate(R.layout.item_star_row, binding.containerStars, false)

            // Rank badge
            row.findViewById<TextView>(R.id.tv_rank).apply {
                text = "${item.rank}"
                if (item.rank <= rankColors.size) {
                    setTextColor(Color.WHITE)
                    background.setTint(rankColors[item.rank - 1])
                }
            }

            row.findViewById<TextView>(R.id.tv_star_name).text = item.name
            row.findViewById<TextView>(R.id.tv_star_achievement).text = "${item.bestScore}分"

            binding.containerStars.addView(row)
        }
    }

    @Suppress("DEPRECATION")
    override fun onBackPressed() {
        finishAffinity()
    }

    private fun setupBottomNav() {
        binding.tabHome.setOnClickListener { /* Already on home */ }

        binding.tabStudents.setOnClickListener {
            val intent = Intent(this, com.fitness.management.ui.student.StudentListActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
            startActivity(intent)
        }

        binding.tabStatistics.setOnClickListener {
            val intent = Intent(this, com.fitness.management.ui.statistics.ClassStatisticsActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
            startActivity(intent)
        }

        binding.tabProfile.setOnClickListener {
            val intent = Intent(this, com.fitness.management.ui.profile.ProfileActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
            startActivity(intent)
        }
    }

    private fun showLoading(isLoading: Boolean) {
        binding.progressOverall.visibility = if (isLoading) View.VISIBLE else View.GONE
        if (!isLoading) {
            if (binding.cardClassProgress.visibility == View.GONE) {
                binding.cardClassProgress.visibility = View.VISIBLE
            }
            if (binding.cardPendingTests.visibility == View.GONE) {
                binding.cardPendingTests.visibility = View.VISIBLE
            }
        }
    }

    companion object {
        private const val TAG = "TeacherHomeActivity"
    }
}
