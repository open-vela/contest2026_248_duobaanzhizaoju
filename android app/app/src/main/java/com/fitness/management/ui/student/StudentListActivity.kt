package com.fitness.management.ui.student

import android.content.Intent
import android.os.Bundle
import android.text.Editable
import android.text.TextWatcher
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.swiperefreshlayout.widget.SwipeRefreshLayout
import com.fitness.management.data.model.StudentListItem
import com.fitness.management.data.repository.FitnessRepository
import com.fitness.management.databinding.ActivityStudentListBinding
import com.fitness.management.ui.statistics.ClassStatisticsActivity
import com.fitness.management.ui.teacher.TeacherHomeActivity
import com.fitness.management.utils.Extensions.toast
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class StudentListActivity : AppCompatActivity() {

    private lateinit var binding: ActivityStudentListBinding
    private val repository = FitnessRepository()
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)
    private var loadJob: Job? = null

    private lateinit var adapter: StudentAdapter
    private var allStudents: List<StudentListItem> = emptyList()
    private var studentScores: Map<String, Int> = emptyMap()
    private var searchQuery: String = ""

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityStudentListBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setupRecyclerView()
        setupSearch()
        setupSwipeRefresh()
        setupBottomNav()
        setupBackButton()
    }

    override fun onResume() {
        super.onResume()
        loadStudents()
    }

    override fun onDestroy() {
        super.onDestroy()
        loadJob?.cancel()
    }

    private fun setupRecyclerView() {
        adapter = StudentAdapter(emptyList(), emptyMap()) { student ->
            val intent = Intent(this, StudentDetailActivity::class.java)
            intent.putExtra("student_id", student.studentId)
            startActivity(intent)
        }
        binding.rvStudents.layoutManager = LinearLayoutManager(this)
        binding.rvStudents.adapter = adapter
    }

    private fun setupSearch() {
        binding.etSearch.addTextChangedListener(object : TextWatcher {
            override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
            override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {}
            override fun afterTextChanged(s: Editable?) {
                searchQuery = s?.toString()?.trim() ?: ""
                applyFilter()
            }
        })
    }

    private fun setupSwipeRefresh() {
        binding.swipeRefresh.setOnRefreshListener {
            loadStudents()
        }
    }

    private fun setupBottomNav() {
        binding.navHome.setOnClickListener {
            navigateToHome()
        }
        // Current tab: Students (highlighted by layout)
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

    private fun setupBackButton() {
        binding.btnBack.setOnClickListener {
            navigateToHome()
        }
    }

    @Suppress("DEPRECATION")
    override fun onBackPressed() {
        navigateToHome()
    }

    private fun navigateToHome() {
        val role = com.fitness.management.utils.SessionManager.role
        val intent = if (role == com.fitness.management.utils.Constants.ROLE_ADMIN) {
            Intent(this, com.fitness.management.ui.admin.AdminHomeActivity::class.java)
        } else {
            Intent(this, TeacherHomeActivity::class.java)
        }
        intent.flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
        startActivity(intent)
        finish()
    }

    private fun loadStudents() {
        loadJob?.cancel()
        loadJob = scope.launch {
            binding.swipeRefresh.isRefreshing = true
            try {
                val result = withContext(Dispatchers.IO) {
                    repository.getMyStudents()
                }
                result.fold(
                    onSuccess = { students ->
                        allStudents = students
                        loadStudentScores(students)
                    },
                    onFailure = { error ->
                        binding.swipeRefresh.isRefreshing = false
                        toast("加载失败: ${error.message}")
                    }
                )
            } catch (e: Exception) {
                binding.swipeRefresh.isRefreshing = false
                toast("加载失败: ${e.message}")
            }
        }
    }

    private suspend fun loadStudentScores(students: List<StudentListItem>) {
        try {
            val scores = mutableMapOf<String, Int>()
            val deferred = students.map { student ->
                scope.async(Dispatchers.IO) {
                    val result = repository.getStudentRecords(student.studentId)
                    result.getOrNull()?.let { records ->
                        val latestScores = records
                            .groupBy { it.testType }
                            .mapValues { (_, typeRecords) ->
                                typeRecords.maxByOrNull { it.testDate }
                            }
                            .mapNotNull { it.value }
                            .mapNotNull { it.score }
                        if (latestScores.isNotEmpty()) {
                            val avg = latestScores.average().toInt()
                            scores[student.studentId] = avg
                        }
                    }
                }
            }
            deferred.awaitAll()
            studentScores = scores
            applyFilter()
        } catch (e: Exception) {
            studentScores = emptyMap()
            applyFilter()
        } finally {
            binding.swipeRefresh.isRefreshing = false
        }
    }

    private fun applyFilter() {
        val filtered = if (searchQuery.isEmpty()) {
            allStudents
        } else {
            allStudents.filter { student ->
                student.name.contains(searchQuery, ignoreCase = true) ||
                        student.studentId.contains(searchQuery, ignoreCase = true)
            }
        }
        adapter.updateData(filtered, studentScores)
    }
}
