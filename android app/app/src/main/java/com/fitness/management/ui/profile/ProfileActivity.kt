package com.fitness.management.ui.profile

import android.content.Intent
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.fitness.management.databinding.ActivityProfileBinding
import com.fitness.management.ui.login.LoginActivity
import com.fitness.management.ui.statistics.ClassStatisticsActivity
import com.fitness.management.ui.student.StudentListActivity
import com.fitness.management.ui.teacher.TeacherHomeActivity
import com.fitness.management.utils.Constants
import com.fitness.management.utils.SessionManager

class ProfileActivity : AppCompatActivity() {

    private lateinit var binding: ActivityProfileBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityProfileBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setupUserInfo()
        setupBottomNav()
        setupLogout()
    }

    private fun setupUserInfo() {
        val realName = SessionManager.realName ?: "用户"
        val username = SessionManager.username ?: "-"
        val role = SessionManager.role ?: "teacher"
        val baseUrl = SessionManager.baseUrl

        binding.tvAvatar.text = realName.firstOrNull()?.toString() ?: "用"
        binding.tvRealName.text = realName
        binding.tvUsername.text = username
        binding.tvRole.text = "角色：${if (role == Constants.ROLE_ADMIN) "管理员" else "教师"}"
        binding.tvRoleDetail.text = if (role == Constants.ROLE_ADMIN) "管理员" else "教师"
        binding.tvServer.text = baseUrl

        try {
            val pInfo = packageManager.getPackageInfo(packageName, 0)
            binding.tvVersion.text = pInfo.versionName
        } catch (_: Exception) {
            binding.tvVersion.text = "1.0.0"
        }
    }

    private fun setupBottomNav() {
        binding.navHome.setOnClickListener {
            navigateToHome()
        }
        binding.navStudent.setOnClickListener {
            startActivity(Intent(this, StudentListActivity::class.java))
            finish()
        }
        binding.navStatistics.setOnClickListener {
            startActivity(Intent(this, ClassStatisticsActivity::class.java))
            finish()
        }
        // Current tab: Profile — do nothing
    }

    private fun navigateToHome() {
        val role = SessionManager.role
        val intent = if (role == Constants.ROLE_ADMIN) {
            Intent(this, com.fitness.management.ui.admin.AdminHomeActivity::class.java)
        } else {
            Intent(this, TeacherHomeActivity::class.java)
        }
        intent.flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
        startActivity(intent)
        finish()
    }

    private fun setupLogout() {
        binding.btnLogout.setOnClickListener {
            SessionManager.logout()
            val intent = Intent(this, LoginActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
            startActivity(intent)
            finish()
        }
    }

    @Suppress("DEPRECATION")
    override fun onBackPressed() {
        navigateToHome()
    }
}
