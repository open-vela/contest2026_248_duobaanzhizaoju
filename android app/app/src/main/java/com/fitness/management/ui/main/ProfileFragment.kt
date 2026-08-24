package com.fitness.management.ui.main

import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import com.fitness.management.databinding.FragmentProfileBinding
import com.fitness.management.ui.login.LoginActivity
import com.fitness.management.utils.Constants
import com.fitness.management.utils.SessionManager

class ProfileFragment : Fragment() {

    private var _binding: FragmentProfileBinding? = null
    private val binding get() = _binding!!

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        _binding = FragmentProfileBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        setupUserInfo()
        setupLogout()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
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
            val pInfo = requireContext().packageManager.getPackageInfo(requireContext().packageName, 0)
            binding.tvVersion.text = pInfo.versionName
        } catch (_: Exception) {
            binding.tvVersion.text = "1.0.0"
        }
    }

    private fun setupLogout() {
        binding.btnLogout.setOnClickListener {
            SessionManager.logout()
            val intent = Intent(requireActivity(), LoginActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
            startActivity(intent)
            requireActivity().finish()
        }
    }
}
