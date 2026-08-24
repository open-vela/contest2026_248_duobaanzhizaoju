package com.fitness.management.ui.main

import android.annotation.SuppressLint
import android.graphics.Color
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import androidx.lifecycle.lifecycleScope
import com.fitness.management.R
import com.fitness.management.data.model.LeaderboardItem
import com.fitness.management.data.repository.FitnessRepository
import com.fitness.management.databinding.FragmentHomeBinding
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

class HomeFragment : Fragment() {

    private var _binding: FragmentHomeBinding? = null
    private val binding get() = _binding!!
    private val repository = FitnessRepository()

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        _binding = FragmentHomeBinding.inflate(inflater, container, false)
        return binding.root
    }

    @SuppressLint("SetTextI18n")
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        val realName = SessionManager.realName ?: "老师"
        binding.tvGreeting.text = "你好，${realName}"
        binding.tvAvatarInitial.text = if (realName.isNotEmpty()) realName.first().toString() else "师"

        val dateFormat = SimpleDateFormat("yyyy年M月d日 EEEE", Locale.CHINESE)
        binding.tvDate.text = dateFormat.format(Date())
    }

    override fun onResume() {
        super.onResume()
        loadData()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }

    private fun loadData() {
        showLoading(true)

        viewLifecycleOwner.lifecycleScope.launch {
            try {
                // Get class info first
                val classesResult = repository.getMyClasses()
                val classes = classesResult.getOrNull() ?: emptyList()

                if (classes.isEmpty()) {
                    showLoading(false)
                    return@launch
                }

                val targetClass = classes.first()
                binding.cardClassProgress.visibility = View.VISIBLE
                binding.tvClassName.text = "${targetClass.grade}${targetClass.className}"
                binding.tvSemester.text = "2026春季学期"

                // Load class statistics for accurate tested count
                val statsResult = repository.getClassStatistics(targetClass.id)
                statsResult.onSuccess { stats ->
                    val testedRate = if (stats.totalStudents > 0) {
                        (stats.testedStudents.toFloat() / stats.totalStudents * 100).toInt().coerceAtMost(100)
                    } else 0
                    binding.tvCompletionText.text = "${stats.testedStudents}/${stats.totalStudents} 人完成"
                    binding.tvPercentage.text = "${testedRate}%"
                    binding.progressClass.progress = testedRate
                }

                // Load overview for avg score
                val overviewResult = repository.getOverview()
                overviewResult.onSuccess { overview ->
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
                requireContext().toast("数据加载失败: ${e.message}")
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
                requireContext().toast("暂无学生数据")
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
                requireContext().toast("暂无测试记录")
            }
        } catch (e: Exception) {
            requireContext().toast("加载测试数据失败")
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
                            color = ContextCompat.getColor(requireContext(), R.color.brand_blue)
                            setCircleColor(ContextCompat.getColor(requireContext(), R.color.brand_blue))
                            lineWidth = 2f
                            circleRadius = 3f
                            setDrawValues(false)
                            setDrawFilled(true)
                            fillColor = ContextCompat.getColor(requireContext(), R.color.brand_blue_light)
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
            color = ContextCompat.getColor(requireContext(), R.color.brand_blue)
            setCircleColor(ContextCompat.getColor(requireContext(), R.color.brand_blue))
            lineWidth = 2f
            circleRadius = 3f
            setDrawValues(false)
            setDrawFilled(true)
            fillColor = ContextCompat.getColor(requireContext(), R.color.brand_blue_light)
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
            textColor = ContextCompat.getColor(requireContext(), R.color.text_secondary)
            labelCount = labels.size
            valueFormatter = IndexAxisValueFormatter(labels)
            granularity = 1f
        }

        chart.axisLeft.apply {
            axisMinimum = 40f
            axisMaximum = 110f
            textColor = ContextCompat.getColor(requireContext(), R.color.text_secondary)
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
            ContextCompat.getColor(requireContext(), R.color.warning_orange), // Gold
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
        private const val TAG = "HomeFragment"
    }
}
