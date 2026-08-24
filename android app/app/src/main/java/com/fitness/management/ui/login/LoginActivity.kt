package com.fitness.management.ui.login

import android.annotation.SuppressLint
import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.text.TextUtils
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.fitness.management.data.api.RetrofitClient
import com.fitness.management.data.repository.FitnessRepository
import com.fitness.management.R
import com.fitness.management.databinding.ActivityLoginBinding
import com.fitness.management.utils.Constants
import com.fitness.management.utils.SessionManager
import kotlinx.coroutines.launch

class LoginActivity : AppCompatActivity() {

    private lateinit var binding: ActivityLoginBinding
    private val repository = FitnessRepository()
    private var selectedRole: String = Constants.ROLE_TEACHER

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Auto-login if session exists
        if (SessionManager.isLoggedIn()) {
            val intent = Intent(this, com.fitness.management.ui.main.MainActivity::class.java)
            startActivity(intent)
            finish()
            return
        }

        binding = ActivityLoginBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setupRoleTabs()
        setupClickListeners()

        // Pre-fill server URL from session if exists
        val savedUrl = SessionManager.baseUrl
        if (savedUrl != Constants.DEFAULT_BASE_URL) {
            binding.etServerUrl.setText(savedUrl)
        }
    }

    private fun setupRoleTabs() {
        selectRoleTab(true)

        binding.tabTeacher.setOnClickListener {
            selectedRole = Constants.ROLE_TEACHER
            selectRoleTab(true)
        }

        binding.tabAdmin.setOnClickListener {
            selectedRole = Constants.ROLE_ADMIN
            selectRoleTab(false)
        }
    }

    private fun selectRoleTab(isTeacher: Boolean) {
        // Teacher tab
        val teacherBg = binding.tabTeacher.getChildAt(0)
        val teacherText = binding.tabTeacher.getChildAt(1) as? android.widget.TextView
        // Admin tab
        val adminBg = binding.tabAdmin.getChildAt(0)
        val adminText = binding.tabAdmin.getChildAt(1) as? android.widget.TextView

        if (isTeacher) {
            teacherBg?.setBackgroundResource(R.drawable.bg_tab_selector)
            teacherText?.setTextColor(Color.WHITE)
            adminBg?.setBackgroundResource(R.drawable.bg_tab_selector_inactive)
            adminText?.setTextColor(getColor(R.color.text_secondary))
        } else {
            adminBg?.setBackgroundResource(R.drawable.bg_tab_selector)
            adminText?.setTextColor(Color.WHITE)
            teacherBg?.setBackgroundResource(R.drawable.bg_tab_selector_inactive)
            teacherText?.setTextColor(getColor(R.color.text_secondary))
        }
    }

    private fun setupClickListeners() {
        binding.btnLogin.setOnClickListener {
            performLogin()
        }

        binding.tvForgotPassword.setOnClickListener {
            Toast.makeText(this, "请联系管理员重置密码", Toast.LENGTH_SHORT).show()
        }
    }

    private fun performLogin() {
        val username = binding.etUsername.text?.toString()?.trim() ?: ""
        val password = binding.etPassword.text?.toString()?.trim() ?: ""
        val serverUrl = binding.etServerUrl.text?.toString()?.trim() ?: ""

        // Validate inputs
        if (TextUtils.isEmpty(username)) {
            binding.tilUsername.error = "请输入用户名"
            return
        }
        binding.tilUsername.error = null

        if (TextUtils.isEmpty(password)) {
            binding.tilPassword.error = "请输入密码"
            return
        }
        binding.tilPassword.error = null

        // Save server URL and rebuild client if changed
        if (serverUrl.isNotEmpty() && serverUrl != SessionManager.baseUrl) {
            SessionManager.setBaseUrl(serverUrl)
            RetrofitClient.init(serverUrl)
        } else if (SessionManager.baseUrl != serverUrl && serverUrl.isNotEmpty()) {
            // Ensure RetrofitClient is initialized
            RetrofitClient.init(serverUrl)
        }

        // Show loading
        showLoading(true)

        lifecycleScope.launch {
            try {
                val result = repository.login(username, password)
                result.fold(
                    onSuccess = { loginResponse ->
                        // Save login session
                        SessionManager.saveLogin(
                            token = loginResponse.accessToken,
                            role = loginResponse.role,
                            realName = loginResponse.realName,
                            username = username
                        )

                        // Navigate to main activity
                        val intent = Intent(this@LoginActivity, com.fitness.management.ui.main.MainActivity::class.java)
                        startActivity(intent)
                        finish()
                    },
                    onFailure = { error ->
                        showLoading(false)
                        Toast.makeText(
                            this@LoginActivity,
                            error.message ?: "登录失败，请检查用户名和密码",
                            Toast.LENGTH_SHORT
                        ).show()
                    }
                )
            } catch (e: Exception) {
                showLoading(false)
                Toast.makeText(
                    this@LoginActivity,
                    "网络连接失败，请检查网络设置",
                    Toast.LENGTH_SHORT
                ).show()
            }
        }
    }

    private fun showLoading(isLoading: Boolean) {
        binding.progressLogin.visibility = if (isLoading) View.VISIBLE else View.GONE
        binding.btnLogin.isEnabled = !isLoading
        binding.btnLogin.text = if (isLoading) "登录中..." else "登 录"
    }

    companion object {
        private const val TAG = "LoginActivity"
    }
}
