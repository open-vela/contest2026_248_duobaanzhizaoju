package com.fitness.management.ui.student

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.fitness.management.R
import com.fitness.management.data.model.StudentListItem

class StudentAdapter(
    private var students: List<StudentListItem>,
    private var scores: Map<String, Int>,
    private val onItemClick: (StudentListItem) -> Unit
) : RecyclerView.Adapter<StudentAdapter.ViewHolder>() {

    interface OnItemClickListener {
        fun onItemClick(student: StudentListItem)
    }

    inner class ViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        val tvAvatar: TextView = itemView.findViewById(R.id.tv_avatar)
        val tvName: TextView = itemView.findViewById(R.id.tv_name)
        val tvStudentId: TextView = itemView.findViewById(R.id.tv_student_id)
        val tvGradeClass: TextView = itemView.findViewById(R.id.tv_grade_class)
        val tvScore: TextView = itemView.findViewById(R.id.tv_score)

        fun bind(student: StudentListItem) {
            tvAvatar.text = if (student.name.isNotEmpty()) student.name.first().toString() else "?"
            tvName.text = student.name
            tvStudentId.text = student.studentId
            tvGradeClass.text = student.grade

            val score = scores[student.studentId]
            if (score != null) {
                tvScore.text = score.toString()
                tvScore.visibility = View.VISIBLE
                val bgRes = when {
                    score >= 90 -> R.drawable.bg_score_green
                    score >= 80 -> R.drawable.bg_score_blue
                    score >= 60 -> R.drawable.bg_score_orange
                    else -> R.drawable.bg_score_red
                }
                tvScore.setBackgroundResource(bgRes)
            } else {
                tvScore.text = "未测"
                tvScore.setBackgroundResource(R.drawable.bg_score_gray)
                tvScore.visibility = View.VISIBLE
            }

            itemView.setOnClickListener { onItemClick(student) }
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_student, parent, false)
        return ViewHolder(view)
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        holder.bind(students[position])
    }

    override fun getItemCount(): Int = students.size

    fun updateData(newStudents: List<StudentListItem>, newScores: Map<String, Int>) {
        students = newStudents
        scores = newScores
        notifyDataSetChanged()
    }
}
