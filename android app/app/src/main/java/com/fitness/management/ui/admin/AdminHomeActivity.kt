package com.fitness.management.ui.admin

import android.content.Intent
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.fitness.management.data.repository.FitnessRepository
import com.fitness.management.databinding.ActivityAdminHomeBinding
import com.fitness.management.ui.statistics.ClassStatisticsActivity
import com.fitness.management.ui.student.StudentListActivity
import com.fitness.management.utils.Constants
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

class AdminHomeActivity : AppCompatActivity() {

    private lateinit var binding: ActivityAdminHomeBinding
    private val repository = FitnessRepository()
    private var loadJob: Job? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityAdminHomeBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setupGreeting()
        setupClickListeners()
    }

    override fun onResume() {
        super.onResume()
        loadData()
    }

    override fun onDestroy() {
        super.onDestroy()
        loadJob?.cancel()
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
            startActivity(Intent(this, ClassManagementActivity::class.java))
        }

        binding.cardDeviceManagement.setOnClickListener {
            startActivity(Intent(this, DeviceManagementActivity::class.java))
        }

        binding.cardReportExport.setOnClickListener {
            startActivity(Intent(this, ReportExportActivity::class.java))
        }

        binding.cardTeacherManagement.setOnClickListener {
            startActivity(Intent(this, TeacherManagementActivity::class.java))
        }

        // Bottom navigation
        binding.navStudent.setOnClickListener {
            val intent = Intent(this, StudentListActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
            startActivity(intent)
        }

        binding.navStatistics.setOnClickListener {
            val intent = Intent(this, ClassStatisticsActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
            startActivity(intent)
        }

        binding.navProfile.setOnClickListener {
            val intent = Intent(this, com.fitness.management.ui.profile.ProfileActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
            startActivity(intent)
        }
    }

    @Suppress("DEPRECATION")
    override fun onBackPressed() {
        finishAffinity()
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
                        binding.tvTotalStudents.text = String.format("%,d", overview.totalStudents)
                        binding.tvTestedStudents.text = String.format("%,d", overview.totalRecords)
                        binding.tvCompletionRate.text = String.format("%.1f%%", overview.passRate)
                        binding.progressCompletion.progress = overview.passRate.toInt()
                    }
                } else {
                    withContext(Dispatchers.Main) {
                        toast("获取概览数据失败")
                    }
                }

                // Handle class count
                val classesResult = classesDeferred.await()
                if (classesResult.isSuccess) {
                    val classes = classesResult.getOrThrow()
                    withContext(Dispatchers.Main) {
                        binding.tvClassCount.text = "${classes.size}个班级"
                    }
                }

                // Handle device count
                val devicesResult = devicesDeferred.await()
                if (devicesResult.isSuccess) {
                    val devices = devicesResult.getOrThrow()
                    withContext(Dispatchers.Main) {
                        binding.tvDeviceCount.text = "${devices.size}台在线"
                    }
                }

                // Handle teacher count
                val teachersResult = teachersDeferred.await()
                if (teachersResult.isSuccess) {
                    val teachers = teachersResult.getOrThrow()
                    withContext(Dispatchers.Main) {
                        binding.tvTeacherCount.text = "${teachers.size}位教师"
                    }
                }

                // Load class rankings (top 3)
                loadClassRankings()

            } catch (e: Exception) {
                withContext(Dispatchers.Main) {
                    toast("加载失败: ${e.message}")
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
        } catch (e: Exception) {
            // Ranking load failed silently
        }
    }
}
