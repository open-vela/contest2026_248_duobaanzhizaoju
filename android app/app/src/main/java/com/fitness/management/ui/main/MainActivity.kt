package com.fitness.management.ui.main

import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import com.fitness.management.R
import com.fitness.management.databinding.ActivityMainBinding
import com.fitness.management.utils.Constants
import com.fitness.management.utils.SessionManager

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    private var homeFragment: Fragment? = null
    private var studentsFragment: Fragment? = null
    private var statisticsFragment: Fragment? = null
    private var profileFragment: Fragment? = null

    private var currentTab = TAB_HOME

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setupFragments()
        setupBottomNav()
        showTab(TAB_HOME)
    }

    private fun setupFragments() {
        val role = SessionManager.role
        homeFragment = if (role == Constants.ROLE_ADMIN) {
            AdminHomeFragment()
        } else {
            HomeFragment()
        }
        studentsFragment = StudentsFragment()
        statisticsFragment = StatisticsFragment()
        profileFragment = ProfileFragment()

        val transaction = supportFragmentManager.beginTransaction()
        homeFragment?.let { transaction.add(R.id.fragmentContainer, it, TAG_HOME).hide(it) }
        studentsFragment?.let { transaction.add(R.id.fragmentContainer, it, TAG_STUDENTS).hide(it) }
        statisticsFragment?.let { transaction.add(R.id.fragmentContainer, it, TAG_STATISTICS).hide(it) }
        profileFragment?.let { transaction.add(R.id.fragmentContainer, it, TAG_PROFILE).hide(it) }
        transaction.commitAllowingStateLoss()
    }

    private fun setupBottomNav() {
        binding.tabHome.setOnClickListener { showTab(TAB_HOME) }
        binding.tabStudents.setOnClickListener { showTab(TAB_STUDENTS) }
        binding.tabStatistics.setOnClickListener { showTab(TAB_STATISTICS) }
        binding.tabProfile.setOnClickListener { showTab(TAB_PROFILE) }
    }

    private fun showTab(tabIndex: Int) {
        currentTab = tabIndex
        val transaction = supportFragmentManager.beginTransaction()

        when (tabIndex) {
            TAB_HOME -> {
                homeFragment?.let { transaction.show(it) }
                studentsFragment?.let { transaction.hide(it) }
                statisticsFragment?.let { transaction.hide(it) }
                profileFragment?.let { transaction.hide(it) }
            }
            TAB_STUDENTS -> {
                homeFragment?.let { transaction.hide(it) }
                studentsFragment?.let { transaction.show(it) }
                statisticsFragment?.let { transaction.hide(it) }
                profileFragment?.let { transaction.hide(it) }
            }
            TAB_STATISTICS -> {
                homeFragment?.let { transaction.hide(it) }
                studentsFragment?.let { transaction.hide(it) }
                statisticsFragment?.let { transaction.show(it) }
                profileFragment?.let { transaction.hide(it) }
            }
            TAB_PROFILE -> {
                homeFragment?.let { transaction.hide(it) }
                studentsFragment?.let { transaction.hide(it) }
                statisticsFragment?.let { transaction.hide(it) }
                profileFragment?.let { transaction.show(it) }
            }
        }

        transaction.commitAllowingStateLoss()
        updateTabHighlight(tabIndex)
    }

    private fun updateTabHighlight(tabIndex: Int) {
        val tabs = listOf(
            binding.tabHome,
            binding.tabStudents,
            binding.tabStatistics,
            binding.tabProfile
        )

        for (i in tabs.indices) {
            val tab = tabs[i]
            if (i == tabIndex) {
                tab.setTextColor(ContextCompat.getColor(this, R.color.brand_blue))
                tab.textSize = 12f
                tab.setTypeface(null, android.graphics.Typeface.BOLD)
                tab.setBackgroundResource(R.drawable.nav_active_bg)
            } else {
                tab.setTextColor(ContextCompat.getColor(this, R.color.text_hint))
                tab.textSize = 12f
                tab.setTypeface(null, android.graphics.Typeface.NORMAL)
                tab.background = null
            }
        }
    }

    @Suppress("DEPRECATION")
    override fun onBackPressed() {
        if (currentTab == TAB_HOME) {
            finishAffinity()
        } else {
            showTab(TAB_HOME)
        }
    }

    companion object {
        private const val TAB_HOME = 0
        private const val TAB_STUDENTS = 1
        private const val TAB_STATISTICS = 2
        private const val TAB_PROFILE = 3

        private const val TAG_HOME = "home_fragment"
        private const val TAG_STUDENTS = "students_fragment"
        private const val TAG_STATISTICS = "statistics_fragment"
        private const val TAG_PROFILE = "profile_fragment"
    }
}
