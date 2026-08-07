package com.fitness.management.ui.main

import android.content.Intent
import android.os.Bundle
import android.text.Editable
import android.text.TextWatcher
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.ArrayAdapter
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.LinearLayoutManager
import com.fitness.management.data.model.ClassInfo
import com.fitness.management.data.model.StudentListItem
import com.fitness.management.data.repository.FitnessRepository
import com.fitness.management.databinding.FragmentStudentsBinding
import com.fitness.management.ui.student.StudentAdapter
import com.fitness.management.ui.student.StudentDetailActivity
import com.fitness.management.utils.Extensions.toast
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class StudentsFragment : Fragment() {

    private var _binding: FragmentStudentsBinding? = null
    private val binding get() = _binding!!
    private val repository = FitnessRepository()
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)
    private var loadJob: Job? = null

    private lateinit var adapter: StudentAdapter
    private var allStudents: List<StudentListItem> = emptyList()
    private var studentScores: Map<String, Int> = emptyMap()
    private var searchQuery: String = ""

    private var classes = listOf<ClassInfo>()
    private var selectedGrade: String? = null
    private var selectedClassId: Int? = null
    private var grades = listOf<String>()

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        _binding = FragmentStudentsBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        setupRecyclerView()
        setupSearch()
        setupSwipeRefresh()
    }

    override fun onResume() {
        super.onResume()
        loadStudents()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        loadJob?.cancel()
        _binding = null
    }

    private fun setupRecyclerView() {
        adapter = StudentAdapter(emptyList(), emptyMap()) { student ->
            val intent = Intent(requireActivity(), StudentDetailActivity::class.java)
            intent.putExtra("student_id", student.studentId)
            startActivity(intent)
        }
        binding.rvStudents.layoutManager = LinearLayoutManager(requireContext())
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

    private fun loadStudents() {
        loadJob?.cancel()
        loadJob = scope.launch {
            binding.swipeRefresh.isRefreshing = true
            try {
                val studentsDeferred = async(Dispatchers.IO) { repository.getMyStudents() }
                val classesDeferred = async(Dispatchers.IO) { repository.getMyClasses() }

                val studentsResult = studentsDeferred.await()
                val classesResult = classesDeferred.await()

                studentsResult.fold(
                    onSuccess = { students ->
                        allStudents = students
                        // Extract unique grades
                        grades = students.map { it.grade }.distinct().sorted()
                        setupGradeSpinner()

                        // Load classes for class spinner
                        classes = classesResult.getOrNull() ?: emptyList()
                        setupClassSpinner()

                        loadStudentScores(students)
                    },
                    onFailure = { error ->
                        binding.swipeRefresh.isRefreshing = false
                        requireContext().toast("加载失败: ${error.message}")
                    }
                )
            } catch (e: Exception) {
                binding.swipeRefresh.isRefreshing = false
                requireContext().toast("加载失败: ${e.message}")
            }
        }
    }

    private fun setupGradeSpinner() {
        val items = mutableListOf("全部年级")
        items.addAll(grades)
        val adapter = ArrayAdapter(requireContext(), android.R.layout.simple_spinner_item, items)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        binding.spinnerGrade.adapter = adapter

        binding.spinnerGrade.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                selectedGrade = if (position > 0) grades[position - 1] else null
                selectedClassId = null
                updateClassSpinner()
                applyFilter()
            }
            override fun onNothingSelected(parent: AdapterView<*>?) {
                selectedGrade = null
            }
        }
    }

    private fun setupClassSpinner() {
        updateClassSpinner()

        binding.spinnerClass.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                val filteredClasses = getFilteredClasses()
                selectedClassId = if (position > 0 && position <= filteredClasses.size) {
                    filteredClasses[position - 1].id
                } else null
                applyFilter()
            }
            override fun onNothingSelected(parent: AdapterView<*>?) {
                selectedClassId = null
            }
        }
    }

    private fun getFilteredClasses(): List<ClassInfo> {
        return if (selectedGrade != null) {
            classes.filter { it.grade == selectedGrade }
        } else {
            classes
        }
    }

    private fun updateClassSpinner() {
        val filteredClasses = getFilteredClasses()
        val items = mutableListOf("全部班级")
        items.addAll(filteredClasses.map { "${it.grade}${it.className}" })
        val adapter = ArrayAdapter(requireContext(), android.R.layout.simple_spinner_item, items)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        binding.spinnerClass.adapter = adapter
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
        var filtered = allStudents

        // Filter by grade
        if (selectedGrade != null) {
            filtered = filtered.filter { it.grade == selectedGrade }
        }

        // Filter by class
        if (selectedClassId != null) {
            filtered = filtered.filter { it.classId == selectedClassId }
        }

        // Filter by search query
        if (searchQuery.isNotEmpty()) {
            filtered = filtered.filter { student ->
                student.name.contains(searchQuery, ignoreCase = true) ||
                        student.studentId.contains(searchQuery, ignoreCase = true)
            }
        }

        adapter.updateData(filtered, studentScores)
    }
}
