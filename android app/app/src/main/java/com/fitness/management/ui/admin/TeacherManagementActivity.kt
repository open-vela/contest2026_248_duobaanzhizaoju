package com.fitness.management.ui.admin

import android.graphics.Color
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ArrayAdapter
import android.widget.LinearLayout
import android.widget.TextView
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.fitness.management.R
import com.fitness.management.data.model.BindingInfo
import com.fitness.management.data.model.ClassInfo
import com.fitness.management.data.model.TeacherInfo
import com.fitness.management.data.repository.FitnessRepository
import com.fitness.management.databinding.ActivityTeacherManagementBinding
import com.fitness.management.utils.Extensions.toast
import kotlinx.coroutines.async
import kotlinx.coroutines.launch

class TeacherManagementActivity : AppCompatActivity() {

    private lateinit var binding: ActivityTeacherManagementBinding
    private val repository = FitnessRepository()
    private val teacherAdapter = TeacherAdapter()
    private var allClasses = listOf<ClassInfo>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityTeacherManagementBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.btnBack.setOnClickListener { finish() }

        binding.rvTeachers.apply {
            layoutManager = LinearLayoutManager(this@TeacherManagementActivity)
            adapter = teacherAdapter
        }

        loadTeachers()
    }

    private fun loadTeachers() {
        binding.progressLoading.visibility = View.VISIBLE
        binding.layoutEmpty.visibility = View.GONE

        lifecycleScope.launch {
            try {
                val teachersDeferred = async { repository.getAllTeachers() }
                val bindingsDeferred = async { repository.getAllBindings() }
                val classesDeferred = async { repository.getAllClasses() }

                val teachersResult = teachersDeferred.await()
                val bindingsResult = bindingsDeferred.await()
                val classesResult = classesDeferred.await()

                binding.progressLoading.visibility = View.GONE

                val teachers = teachersResult.getOrNull() ?: emptyList()
                val bindings = bindingsResult.getOrNull() ?: emptyList()
                allClasses = classesResult.getOrNull() ?: emptyList()

                if (teachers.isEmpty()) {
                    binding.layoutEmpty.visibility = View.VISIBLE
                    binding.rvTeachers.visibility = View.GONE
                } else {
                    binding.tvTeacherTotal.text = "共${teachers.size}位教师"
                    teacherAdapter.submitList(teachers, bindings)
                }
            } catch (e: Exception) {
                binding.progressLoading.visibility = View.GONE
                toast("网络错误: ${e.message}")
            }
        }
    }

    private fun showEditDialog(teacher: TeacherInfo, currentBindings: List<BindingInfo>) {
        val dialogView = LayoutInflater.from(this).inflate(R.layout.dialog_teacher_edit, null)
        val tvTeacherName = dialogView.findViewById<TextView>(R.id.tvTeacherName)
        val containerBoundClasses = dialogView.findViewById<LinearLayout>(R.id.containerBoundClasses)
        val tvNoBindings = dialogView.findViewById<TextView>(R.id.tvNoBindings)
        val spinnerClass = dialogView.findViewById<android.widget.Spinner>(R.id.spinnerClass)
        val btnBind = dialogView.findViewById<com.google.android.material.button.MaterialButton>(R.id.btnBind)
        val progressLoading = dialogView.findViewById<View>(R.id.progressLoading)

        tvTeacherName.text = teacher.realName

        // Show current bindings
        containerBoundClasses.removeAllViews()
        if (currentBindings.isEmpty()) {
            tvNoBindings.visibility = View.VISIBLE
            containerBoundClasses.visibility = View.GONE
        } else {
            tvNoBindings.visibility = View.GONE
            containerBoundClasses.visibility = View.VISIBLE
            for (bindingInfo in currentBindings) {
                val row = LinearLayout(this).apply {
                    orientation = LinearLayout.HORIZONTAL
                    gravity = android.view.Gravity.CENTER_VERTICAL
                    setPadding(0, 8, 0, 8)
                }

                val tvClassName = TextView(this).apply {
                    text = bindingInfo.className
                    textSize = 14f
                    setTextColor(resources.getColor(R.color.text_primary, null))
                    layoutParams = LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f)
                }

                val btnUnbind = TextView(this).apply {
                    text = "解绑"
                    textSize = 12f
                    setTextColor(resources.getColor(R.color.danger_red, null))
                    setPadding(16, 8, 16, 8)
                    setBackgroundResource(R.drawable.bg_tab_selector_inactive)
                }

                btnUnbind.setOnClickListener {
                    AlertDialog.Builder(this@TeacherManagementActivity)
                        .setTitle("解绑确认")
                        .setMessage("确定要解绑 ${teacher.realName} 与 ${bindingInfo.className} 吗？")
                        .setPositiveButton("确定") { _, _ ->
                            performUnbind(teacher, bindingInfo, dialogView)
                        }
                        .setNegativeButton("取消", null)
                        .show()
                }

                row.addView(tvClassName)
                row.addView(btnUnbind)
                containerBoundClasses.addView(row)
            }
        }

        // Setup class spinner
        val boundClassIds = currentBindings.map { it.classId }.toSet()
        val availableClasses = allClasses.filter { it.id !in boundClassIds }
        val classNames = mutableListOf("-- 请选择班级 --")
        classNames.addAll(availableClasses.map { "${it.grade}${it.className}" })
        val spinnerAdapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, classNames)
        spinnerAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        spinnerClass.adapter = spinnerAdapter

        val dialog = AlertDialog.Builder(this)
            .setView(dialogView)
            .setNegativeButton("关闭", null)
            .create()

        btnBind.setOnClickListener {
            val selectedPos = spinnerClass.selectedItemPosition
            if (selectedPos <= 0) {
                toast("请选择要绑定的班级")
                return@setOnClickListener
            }
            val selectedClass = availableClasses[selectedPos - 1]
            performBind(teacher, selectedClass, dialogView, dialog)
        }

        dialog.show()
    }

    private fun performBind(teacher: TeacherInfo, classInfo: ClassInfo, dialogView: View, dialog: AlertDialog) {
        val progressLoading = dialogView.findViewById<View>(R.id.progressLoading)
        progressLoading.visibility = View.VISIBLE

        lifecycleScope.launch {
            try {
                val result = repository.adminBind(teacher.id, classInfo.id)
                progressLoading.visibility = View.GONE
                result.onSuccess {
                    toast("绑定成功")
                    dialog.dismiss()
                    loadTeachers()
                }
                result.onFailure { error ->
                    toast("绑定失败: ${error.message}")
                }
            } catch (e: Exception) {
                progressLoading.visibility = View.GONE
                toast("网络错误: ${e.message}")
            }
        }
    }

    private fun performUnbind(teacher: TeacherInfo, bindingInfo: BindingInfo, dialogView: View) {
        val progressLoading = dialogView.findViewById<View>(R.id.progressLoading)
        progressLoading.visibility = View.VISIBLE

        lifecycleScope.launch {
            try {
                val result = repository.adminUnbind(teacher.id, bindingInfo.classId)
                progressLoading.visibility = View.GONE
                result.onSuccess {
                    toast("解绑成功")
                    // Refresh the dialog
                    val newBindings = teacherAdapter.getBindingsForTeacher(teacher.id).filter { it.classId != bindingInfo.classId }
                    showEditDialog(teacher, newBindings)
                    loadTeachers()
                }
                result.onFailure { error ->
                    toast("解绑失败: ${error.message}")
                }
            } catch (e: Exception) {
                progressLoading.visibility = View.GONE
                toast("网络错误: ${e.message}")
            }
        }
    }

    inner class TeacherAdapter : RecyclerView.Adapter<TeacherAdapter.ViewHolder>() {
        private var teachers = listOf<TeacherInfo>()
        private var bindings = listOf<BindingInfo>()

        fun submitList(teacherList: List<TeacherInfo>, bindingList: List<BindingInfo>) {
            teachers = teacherList
            bindings = bindingList
            notifyDataSetChanged()
        }

        fun getBindingsForTeacher(teacherId: Int): List<BindingInfo> {
            return bindings.filter { it.teacherId == teacherId }
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
            val view = LayoutInflater.from(parent.context)
                .inflate(R.layout.item_teacher_row, parent, false)
            return ViewHolder(view)
        }

        override fun onBindViewHolder(holder: ViewHolder, position: Int) {
            holder.bind(teachers[position])
        }

        override fun getItemCount() = teachers.size

        inner class ViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
            private val tvAvatar: TextView = itemView.findViewById(R.id.tvAvatar)
            private val tvTeacherName: TextView = itemView.findViewById(R.id.tvTeacherName)
            private val tvUsername: TextView = itemView.findViewById(R.id.tvUsername)
            private val tvBoundClasses: TextView = itemView.findViewById(R.id.tvBoundClasses)
            private val btnEdit: TextView = itemView.findViewById(R.id.btnEdit)

            fun bind(item: TeacherInfo) {
                tvAvatar.text = item.realName.firstOrNull()?.toString() ?: "师"
                tvTeacherName.text = item.realName
                tvUsername.text = "账号：${item.username}"

                val teacherBindings = bindings.filter { it.teacherId == item.id }
                if (teacherBindings.isNotEmpty()) {
                    tvBoundClasses.text = "管理班级：${teacherBindings.joinToString("、") { it.className }}"
                    tvBoundClasses.setTextColor(itemView.context.getColor(R.color.text_hint))
                } else {
                    tvBoundClasses.text = "未分配班级"
                    tvBoundClasses.setTextColor(itemView.context.getColor(R.color.warning_orange))
                }

                btnEdit.setOnClickListener {
                    showEditDialog(item, teacherBindings)
                }
            }
        }
    }
}
