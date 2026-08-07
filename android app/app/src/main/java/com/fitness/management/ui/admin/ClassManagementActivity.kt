package com.fitness.management.ui.admin

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.fitness.management.R
import com.fitness.management.data.model.ClassInfo
import com.fitness.management.data.repository.FitnessRepository
import com.fitness.management.databinding.ActivityClassManagementBinding
import com.fitness.management.utils.Extensions.toast
import kotlinx.coroutines.launch

class ClassManagementActivity : AppCompatActivity() {

    private lateinit var binding: ActivityClassManagementBinding
    private val repository = FitnessRepository()
    private val classAdapter = ClassAdapter()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityClassManagementBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.btnBack.setOnClickListener { finish() }

        binding.rvClasses.apply {
            layoutManager = LinearLayoutManager(this@ClassManagementActivity)
            adapter = classAdapter
        }

        loadClasses()
    }

    private fun loadClasses() {
        binding.progressLoading.visibility = View.VISIBLE
        binding.layoutEmpty.visibility = View.GONE

        lifecycleScope.launch {
            try {
                val result = repository.getAllClasses()
                result.onSuccess { classes ->
                    binding.progressLoading.visibility = View.GONE
                    if (classes.isEmpty()) {
                        binding.layoutEmpty.visibility = View.VISIBLE
                        binding.rvClasses.visibility = View.GONE
                    } else {
                        binding.tvClassTotal.text = "共${classes.size}个班级"
                        classAdapter.submitList(classes)
                    }
                }
                result.onFailure { error ->
                    binding.progressLoading.visibility = View.GONE
                    toast("加载失败: ${error.message}")
                }
            } catch (e: Exception) {
                binding.progressLoading.visibility = View.GONE
                toast("网络错误: ${e.message}")
            }
        }
    }

    inner class ClassAdapter : RecyclerView.Adapter<ClassAdapter.ViewHolder>() {
        private var items = listOf<ClassInfo>()

        fun submitList(list: List<ClassInfo>) {
            items = list
            notifyDataSetChanged()
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
            val view = LayoutInflater.from(parent.context)
                .inflate(R.layout.item_class_row, parent, false)
            return ViewHolder(view)
        }

        override fun onBindViewHolder(holder: ViewHolder, position: Int) {
            holder.bind(items[position])
        }

        override fun getItemCount() = items.size

        inner class ViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
            private val tvClassName: TextView = itemView.findViewById(R.id.tvClassName)
            private val tvClassGrade: TextView = itemView.findViewById(R.id.tvClassGrade)
            private val tvTeacher: TextView = itemView.findViewById(R.id.tvTeacher)

            fun bind(item: ClassInfo) {
                tvClassName.text = "${item.grade}${item.className}"
                tvClassGrade.text = "年级：${item.grade}"
                tvTeacher.text = item.headTeacher ?: "未指定班主任"
            }
        }
    }
}
