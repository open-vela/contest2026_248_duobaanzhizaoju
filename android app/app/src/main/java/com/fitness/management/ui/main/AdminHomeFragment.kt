package com.fitness.management.ui.main

import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import com.fitness.management.data.repository.FitnessRepository
import com.fitness.management.databinding.FragmentAdminHomeBinding
import com.fitness.management.ui.admin.ClassManagementActivity
import com.fitness.management.ui.admin.DeviceManagementActivity
import com.fitness.management.ui.admin.ReportExportActivity
import com.fitness.management.ui.admin.TeacherManagementActivity
import com.fitness.management.utils.Extensions.toast
import com.fitness.management.utils.SessionManager
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.async
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.text.SimpleDateFormat
import java.util.Calendar
import java.util.Locale

class AdminHomeFragment : Fragment() {

    private var _binding: FragmentAdminHomeBinding? = null
    private val binding get() = _binding!!
    private val repository = FitnessRepository()
    private var loadJob: Job? = null

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        _binding = FragmentAdminHomeBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        setupGreeting()
        setupClickListeners()
    }

    override fun onResume() {
        super.onResume()
        loadData()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        loadJob?.cancel()
        _binding = null
    }

    private fun setupGreeting() {
        val realName = SessionManager.realName ?: "管理员"
        binding.tvGreeting.text = "你好，${realName}"

        val calendar = Calendar.getInstance()
        val dateFormat = SimpleDateFormat("yyyy年M月d日", Locale.CHINESE)
        val dayOfWeek = when (calendar.get(Calendar.DAY_OF_WEEK)) {
            Calendar.SUNDAY -> "周日"
            Calendar.MONDAY -> "周一"
            Calendar.TUESDAY -> "周二"
            Calendar.WEDNESDAY -> "周三"
            Calendar.THURSDAY -> "周四"
            Calendar.FRIDAY -> "周五"
            Calendar.SATURDAY -> "周六"
            else -> ""
        }
        binding.tvDate.text = "${dateFormat.format(calendar.time)} $dayOfWeek"

        val displayName = realName.take(1)
        binding.avatar.text = displayName
    }

    private fun setupClickListeners() {
        // Management function cards
        binding.cardClassManagement.setOnClickListener {
            startActivity(Intent(requireActivity(), ClassManagementActivity::class.java))
        }

        binding.cardDeviceManagement.setOnClickListener {
            startActivity(Intent(requireActivity(), DeviceManagementActivity::class.java))
        }

        binding.cardReportExport.setOnClickListener {
            startActivity(Intent(requireActivity(), ReportExportActivity::class.java))
        }

        binding.cardTeacherManagement.setOnClickListener {
            startActivity(Intent(requireActivity(), TeacherManagementActivity::class.java))
        }
    }

    private fun loadData() {
        loadJob?.cancel()
        loadJob = CoroutineScope(Dispatchers.IO).launch {
            try {
                // Load school overview
                val overviewDeferred = async { repository.getOverview() }

                // Load class count
                val classesDeferred = async { repository.getAllClasses() }

                // Load device count
                val devicesDeferred = async { repository.listDevices(activeOnly = true) }

                // Load teacher count
                val teachersDeferred = async { repository.getAllTeachers() }

                // Handle overview
                val overviewResult = overviewDeferred.await()
                if (overviewResult.isSuccess) {
                    val overview = overviewResult.getOrThrow()
                    withContext(Dispatchers.Main) {
                        _binding?.let { binding ->
                            binding.tvTotalStudents.text = String.format("%,d", overview.totalStudents)
                            binding.tvTestedStudents.text = String.format("%,d", overview.totalRecords)
                            binding.tvCompletionRate.text = String.format("%.1f%%", overview.passRate)
                            binding.progressCompletion.progress = overview.passRate.toInt()
                        }
                    }
                } else {
                    withContext(Dispatchers.Main) {
                        requireContext().toast("获取概览数据失败")
                    }
                }

                // Handle class count
                val classesResult = classesDeferred.await()
                if (classesResult.isSuccess) {
                    val classes = classesResult.getOrThrow()
                    withContext(Dispatchers.Main) {
                        _binding?.let { binding ->
                            binding.tvClassCount.text = "${classes.size}个班级"
                        }
                    }
                }

                // Handle device count
                val devicesResult = devicesDeferred.await()
                if (devicesResult.isSuccess) {
                    val devices = devicesResult.getOrThrow()
                    withContext(Dispatchers.Main) {
                        _binding?.let { binding ->
                            binding.tvDeviceCount.text = "${devices.size}台在线"
                        }
                    }
                }

                // Handle teacher count
                val teachersResult = teachersDeferred.await()
                if (teachersResult.isSuccess) {
                    val teachers = teachersResult.getOrThrow()
                    withContext(Dispatchers.Main) {
                        _binding?.let { binding ->
                            binding.tvTeacherCount.text = "${teachers.size}位教师"
                        }
                    }
                }

                // Load class rankings (top 3)
                loadClassRankings()

            } catch (e: Exception) {
                withContext(Dispatchers.Main) {
                    requireContext().toast("加载失败: ${e.message}")
                }
            }
        }
    }

    private suspend fun loadClassRankings() {
        try {
            val classesResult = repository.getAllClasses()
            if (classesResult.isFailure) return

            val classes = classesResult.getOrThrow()
            val statsWithClass = mutableListOf<Triple<Double, String, String>>()

            for (classInfo in classes) {
                val statsResult = repository.getClassStatistics(classInfo.id)
                if (statsResult.isSuccess) {
                    val stats = statsResult.getOrThrow()
                    val teacherName = classInfo.headTeacher ?: ""
                    val displayName = "${classInfo.grade}${classInfo.className}"
                    statsWithClass.add(Triple(stats.avgScore, displayName, teacherName))
                }
            }

            statsWithClass.sortByDescending { it.first }
            val top3 = statsWithClass.take(3)

            withContext(Dispatchers.Main) {
                _binding?.let { binding ->
                    if (top3.size >= 1) {
                        val (score1, name1, teacher1) = top3[0]
                        binding.tvRank1Name.text = name1
                        binding.tvRank1Teacher.text = if (teacher1.isNotEmpty()) "班主任：$teacher1" else ""
                        binding.tvRank1Score.text = String.format("%.1f", score1)
                    }
                    if (top3.size >= 2) {
                        val (score2, name2, teacher2) = top3[1]
                        binding.tvRank2Name.text = name2
                        binding.tvRank2Teacher.text = if (teacher2.isNotEmpty()) "班主任：$teacher2" else ""
                        binding.tvRank2Score.text = String.format("%.1f", score2)
                    }
                    if (top3.size >= 3) {
                        val (score3, name3, teacher3) = top3[2]
                        binding.tvRank3Name.text = name3
                        binding.tvRank3Teacher.text = if (teacher3.isNotEmpty()) "班主任：$teacher3" else ""
                        binding.tvRank3Score.text = String.format("%.1f", score3)
                    }
                }
            }
        } catch (e: Exception) {
            // Ranking load failed silently
        }
    }
}
