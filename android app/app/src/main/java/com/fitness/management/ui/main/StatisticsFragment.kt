package com.fitness.management.ui.main

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.ArrayAdapter
import androidx.fragment.app.Fragment
import com.fitness.management.data.model.ClassInfo
import com.fitness.management.data.model.ClassStats
import com.fitness.management.data.repository.FitnessRepository
import com.fitness.management.databinding.FragmentStatisticsBinding
import com.fitness.management.utils.Constants
import com.fitness.management.utils.Extensions.toast
import com.github.mikephil.charting.components.XAxis
import com.github.mikephil.charting.data.BarData
import com.github.mikephil.charting.data.BarDataSet
import com.github.mikephil.charting.data.BarEntry
import com.github.mikephil.charting.formatter.IndexAxisValueFormatter
import com.github.mikephil.charting.formatter.ValueFormatter
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class StatisticsFragment : Fragment() {

    private var _binding: FragmentStatisticsBinding? = null
    private val binding get() = _binding!!
    private val repository = FitnessRepository()
    private var loadJob: Job? = null
    private var genderJob: Job? = null

    private var classes = listOf<ClassInfo>()
    private var selectedClassIndex = 0

    private val barColors = intArrayOf(
        Constants.COLOR_BRAND_BLUE,
        Constants.COLOR_SUCCESS_GREEN,
        Constants.COLOR_WARNING_ORANGE,
        Constants.COLOR_PURPLE,
        Constants.COLOR_DANGER_RED
    )

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        _binding = FragmentStatisticsBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onResume() {
        super.onResume()
        if (classes.isEmpty()) {
            loadClasses()
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        loadJob?.cancel()
        genderJob?.cancel()
        _binding = null
    }

    private fun loadClasses() {
        CoroutineScope(Dispatchers.IO).launch {
            try {
                val result = repository.getMyClasses()
                if (result.isFailure) {
                    withContext(Dispatchers.Main) {
                        requireContext().toast("获取班级列表失败")
                    }
                    return@launch
                }

                classes = result.getOrThrow()
                if (classes.isEmpty()) {
                    withContext(Dispatchers.Main) {
                        requireContext().toast("暂无班级数据")
                    }
                    return@launch
                }

                withContext(Dispatchers.Main) {
                    _binding?.let { binding ->
                        setupClassSpinner()
                    }
                }
            } catch (e: Exception) {
                withContext(Dispatchers.Main) {
                    requireContext().toast("加载失败: ${e.message}")
                }
            }
        }
    }

    private fun setupClassSpinner() {
        val classNames = classes.map { "${it.grade}${it.className}" }
        val adapter = ArrayAdapter(requireContext(), android.R.layout.simple_spinner_item, classNames)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        binding.spinnerClass.adapter = adapter

        binding.spinnerClass.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                if (position != selectedClassIndex) {
                    selectedClassIndex = position
                    loadClassStatistics(classes[position].id)
                }
            }
            override fun onNothingSelected(parent: AdapterView<*>?) {}
        }

        // Load first class stats
        selectedClassIndex = 0
        loadClassStatistics(classes[0].id)
    }

    private fun loadClassStatistics(classId: Int) {
        loadJob?.cancel()
        loadJob = CoroutineScope(Dispatchers.IO).launch {
            try {
                val statsResult = repository.getClassStatistics(classId)
                if (statsResult.isFailure) {
                    withContext(Dispatchers.Main) {
                        requireContext().toast("获取班级统计失败")
                    }
                    return@launch
                }

                val stats = statsResult.getOrThrow()
                withContext(Dispatchers.Main) {
                    _binding?.let { binding ->
                        updateKpiCards(stats)
                        updateBarChart(stats)
                        updateScoreDistribution(stats)
                        updateGenderComparison(classId)
                    }
                }
            } catch (e: Exception) {
                withContext(Dispatchers.Main) {
                    requireContext().toast("加载失败: ${e.message}")
                }
            }
        }
    }

    private fun updateKpiCards(stats: ClassStats) {
        binding.tvAvgScore.text = String.format("%.1f", stats.avgScore)
        binding.tvExcellentRate.text = String.format("%.0f%%", stats.excellentRate)
        val excellentCount = ((stats.excellentRate / 100.0) * stats.totalStudents).toInt()
        binding.tvExcellentCount.text = "${excellentCount}人优秀"
        binding.tvPassRate.text = String.format("%.0f%%", stats.passRate)
        val passCount = ((stats.passRate / 100.0) * stats.totalStudents).toInt()
        binding.tvPassCount.text = "${passCount}人及格"
    }

    private fun updateBarChart(stats: ClassStats) {
        val barChart = binding.barChart
        val testTypeStats = stats.testTypeStats

        if (testTypeStats.isEmpty()) {
            barChart.visibility = View.GONE
            return
        }
        barChart.visibility = View.VISIBLE

        val entries = testTypeStats.mapIndexed { index, stat ->
            BarEntry(index.toFloat(), stat.avgScore.toFloat())
        }

        val labels = testTypeStats.map { stat ->
            stat.label.ifEmpty {
                Constants.TEST_TYPE_LABELS[stat.testType] ?: stat.testType
            }
        }

        val dataSet = BarDataSet(entries, "").apply {
            colors = entries.indices.map { barColors[it % barColors.size] }
            valueTextSize = 10f
            valueTextColor = Constants.COLOR_TEXT_SECONDARY
            valueFormatter = object : ValueFormatter() {
                override fun getBarLabel(entry: BarEntry?): String {
                    return entry?.y?.let { String.format("%.1f", it) } ?: ""
                }
            }
            setDrawValues(true)
        }

        barChart.apply {
            data = BarData(dataSet)
            description.isEnabled = false
            legend.isEnabled = false
            setFitBars(true)
            animateY(800)

            xAxis.apply {
                position = XAxis.XAxisPosition.BOTTOM
                valueFormatter = IndexAxisValueFormatter(labels)
                granularity = 1f
                setDrawGridLines(false)
                textColor = Constants.COLOR_TEXT_SECONDARY
                textSize = 9f
                labelRotationAngle = -35f
                setAvoidFirstLastClipping(true)
                labelCount = labels.size
            }

            axisLeft.apply {
                axisMinimum = 0f
                axisMaximum = 100f
                setDrawGridLines(false)
                textColor = Constants.COLOR_TEXT_SECONDARY
                textSize = 10f
            }

            axisRight.isEnabled = false
            setScaleEnabled(false)
            setPinchZoom(false)
            isDoubleTapToZoomEnabled = false
            invalidate()
        }
    }

    private fun updateScoreDistribution(stats: ClassStats) {
        val totalStudents = stats.totalStudents
        if (totalStudents == 0) return

        val testTypeStats = stats.testTypeStats
        var excellentCount = 0
        var goodCount = 0
        var passCount = 0
        var failCount = 0

        for (stat in testTypeStats) {
            val score = stat.avgScore.toInt()
            when {
                score >= 90 -> excellentCount++
                score in 80..89 -> goodCount++
                score in 60..79 -> passCount++
                else -> failCount++
            }
        }

        if (testTypeStats.isEmpty()) {
            excellentCount = ((stats.excellentRate / 100.0) * totalStudents).toInt()
            val passRateCount = ((stats.passRate / 100.0) * totalStudents).toInt()
            failCount = totalStudents - passRateCount
            passCount = passRateCount - excellentCount
            goodCount = 0
        }

        val total = excellentCount + goodCount + passCount + failCount
        if (total == 0) return

        val excellentPct = (excellentCount * 100 / total)
        val goodPct = (goodCount * 100 / total)
        val passPct = (passCount * 100 / total)
        val failPct = 100 - excellentPct - goodPct - passPct

        binding.progressExcellent.progress = excellentPct
        binding.tvExcellentPercent.text = "${excellentPct}%"
        binding.progressGood.progress = goodPct
        binding.tvGoodPercent.text = "${goodPct}%"
        binding.progressPass.progress = passPct
        binding.tvPassPercent.text = "${passPct}%"
        binding.progressFail.progress = failPct
        binding.tvFailPercent.text = "${failPct}%"
    }

    private fun updateGenderComparison(classId: Int) {
        genderJob?.cancel()
        genderJob = CoroutineScope(Dispatchers.IO).launch {
            try {
                val studentsResult = repository.getMyStudents(classId)
                if (studentsResult.isFailure) return@launch

                val students = studentsResult.getOrThrow()
                val maleStudents = students.filter { it.gender == "M" }
                val femaleStudents = students.filter { it.gender == "F" }

                var maleAvgScore = 0.0
                var maleExcellentRate = 0.0
                var maleCount = 0
                var femaleAvgScore = 0.0
                var femaleExcellentRate = 0.0
                var femaleCount = 0

                for (student in maleStudents) {
                    val recordsResult = repository.getStudentRecords(student.studentId)
                    if (recordsResult.isSuccess) {
                        val records = recordsResult.getOrThrow()
                        if (records.isNotEmpty()) {
                            val avg = records.mapNotNull { it.score }.average()
                            val excellent = records.count { (it.score ?: 0) >= 90 }
                            maleAvgScore += avg
                            maleExcellentRate += (excellent.toDouble() / records.size) * 100
                            maleCount++
                        }
                    }
                }

                for (student in femaleStudents) {
                    val recordsResult = repository.getStudentRecords(student.studentId)
                    if (recordsResult.isSuccess) {
                        val records = recordsResult.getOrThrow()
                        if (records.isNotEmpty()) {
                            val avg = records.mapNotNull { it.score }.average()
                            val excellent = records.count { (it.score ?: 0) >= 90 }
                            femaleAvgScore += avg
                            femaleExcellentRate += (excellent.toDouble() / records.size) * 100
                            femaleCount++
                        }
                    }
                }

                if (maleCount > 0) {
                    maleAvgScore /= maleCount
                    maleExcellentRate /= maleCount
                }
                if (femaleCount > 0) {
                    femaleAvgScore /= femaleCount
                    femaleExcellentRate /= femaleCount
                }

                withContext(Dispatchers.Main) {
                    _binding?.let { binding ->
                        binding.tvMaleScore.text = String.format("%.1f分", maleAvgScore)
                        binding.tvMaleInfo.text = "${maleStudents.size}人 / 优秀率${String.format("%.0f%%", maleExcellentRate)}"
                        binding.tvFemaleScore.text = String.format("%.1f分", femaleAvgScore)
                        binding.tvFemaleInfo.text = "${femaleStudents.size}人 / 优秀率${String.format("%.0f%%", femaleExcellentRate)}"
                    }
                }
            } catch (e: Exception) {
                withContext(Dispatchers.Main) {
                    requireContext().toast("加载性别对比失败: ${e.message}")
                }
            }
        }
    }
}
