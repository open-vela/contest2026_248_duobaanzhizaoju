package com.fitness.management.ui.admin

import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.fitness.management.R
import com.fitness.management.data.model.Device
import com.fitness.management.data.repository.FitnessRepository
import com.fitness.management.databinding.ActivityDeviceManagementBinding
import com.fitness.management.utils.Extensions.toast
import kotlinx.coroutines.launch

class DeviceManagementActivity : AppCompatActivity() {

    private lateinit var binding: ActivityDeviceManagementBinding
    private val repository = FitnessRepository()
    private val deviceAdapter = DeviceAdapter()
    private var allDevices = listOf<Device>()
    private var currentFilter = "all"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityDeviceManagementBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.btnBack.setOnClickListener { finish() }

        binding.rvDevices.apply {
            layoutManager = LinearLayoutManager(this@DeviceManagementActivity)
            adapter = deviceAdapter
        }

        setupFilterChips()
        loadDevices()
    }

    private fun setupFilterChips() {
        binding.chipAll.setOnClickListener {
            currentFilter = "all"
            updateFilterUI()
            filterDevices()
        }
        binding.chipOnline.setOnClickListener {
            currentFilter = "online"
            updateFilterUI()
            filterDevices()
        }
        binding.chipOffline.setOnClickListener {
            currentFilter = "offline"
            updateFilterUI()
            filterDevices()
        }
    }

    private fun updateFilterUI() {
        val chips = listOf(binding.chipAll, binding.chipOnline, binding.chipOffline)
        val selectedChip = when (currentFilter) {
            "all" -> binding.chipAll
            "online" -> binding.chipOnline
            else -> binding.chipOffline
        }
        chips.forEach { chip ->
            if (chip == selectedChip) {
                chip.setBackgroundResource(R.drawable.bg_tab_active)
                chip.setTextColor(Color.WHITE)
            } else {
                chip.setBackgroundResource(R.drawable.bg_tab_selector_inactive)
                chip.setTextColor(ContextCompat.getColor(this, R.color.text_secondary))
            }
        }
    }

    private fun loadDevices() {
        binding.progressLoading.visibility = View.VISIBLE
        binding.layoutEmpty.visibility = View.GONE

        lifecycleScope.launch {
            try {
                val result = repository.listDevices(activeOnly = false)
                result.onSuccess { devices ->
                    binding.progressLoading.visibility = View.GONE
                    allDevices = devices
                    filterDevices()
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

    private fun filterDevices() {
        val filtered = when (currentFilter) {
            "online" -> allDevices.filter { it.isActive }
            "offline" -> allDevices.filter { !it.isActive }
            else -> allDevices
        }
        binding.tvDeviceTotal.text = "共${filtered.size}台设备"
        if (filtered.isEmpty()) {
            binding.layoutEmpty.visibility = View.VISIBLE
            binding.rvDevices.visibility = View.GONE
        } else {
            binding.layoutEmpty.visibility = View.GONE
            binding.rvDevices.visibility = View.VISIBLE
            deviceAdapter.submitList(filtered)
        }
    }

    inner class DeviceAdapter : RecyclerView.Adapter<DeviceAdapter.ViewHolder>() {
        private var items = listOf<Device>()

        fun submitList(list: List<Device>) {
            items = list
            notifyDataSetChanged()
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
            val view = LayoutInflater.from(parent.context)
                .inflate(R.layout.item_device_row, parent, false)
            return ViewHolder(view)
        }

        override fun onBindViewHolder(holder: ViewHolder, position: Int) {
            holder.bind(items[position])
        }

        override fun getItemCount() = items.size

        inner class ViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
            private val viewStatusDot: View = itemView.findViewById(R.id.viewStatusDot)
            private val tvDeviceId: TextView = itemView.findViewById(R.id.tvDeviceId)
            private val tvDeviceName: TextView = itemView.findViewById(R.id.tvDeviceName)
            private val tvBoundStudent: TextView = itemView.findViewById(R.id.tvBoundStudent)
            private val tvBattery: TextView = itemView.findViewById(R.id.tvBattery)
            private val tvOnlineStatus: TextView = itemView.findViewById(R.id.tvOnlineStatus)

            fun bind(item: Device) {
                tvDeviceId.text = item.deviceId
                tvDeviceName.text = item.deviceName ?: "未命名设备"
                tvBoundStudent.text = if (item.studentId != null) "绑定：${item.studentId}" else "未绑定学生"

                val battery = item.batteryLevel
                if (battery != null) {
                    tvBattery.text = "${battery}%"
                    tvBattery.setTextColor(
                        when {
                            battery > 50 -> ContextCompat.getColor(itemView.context, R.color.success_green)
                            battery > 20 -> ContextCompat.getColor(itemView.context, R.color.warning_orange)
                            else -> ContextCompat.getColor(itemView.context, R.color.danger_red)
                        }
                    )
                } else {
                    tvBattery.text = "--"
                    tvBattery.setTextColor(ContextCompat.getColor(itemView.context, R.color.text_hint))
                }

                if (item.isActive) {
                    tvOnlineStatus.text = "在线"
                    tvOnlineStatus.setTextColor(ContextCompat.getColor(itemView.context, R.color.success_green))
                    val dot = viewStatusDot.background as? GradientDrawable
                    dot?.setColor(ContextCompat.getColor(itemView.context, R.color.success_green))
                } else {
                    tvOnlineStatus.text = "离线"
                    tvOnlineStatus.setTextColor(ContextCompat.getColor(itemView.context, R.color.text_hint))
                    val dot = viewStatusDot.background as? GradientDrawable
                    dot?.setColor(ContextCompat.getColor(itemView.context, R.color.text_hint))
                }
            }
        }
    }
}
