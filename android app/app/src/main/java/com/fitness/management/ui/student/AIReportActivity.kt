package com.fitness.management.ui.student

import android.graphics.Color
import android.os.Bundle
import android.view.View
import android.webkit.WebView
import android.webkit.WebViewClient
import androidx.appcompat.app.AppCompatActivity
import com.fitness.management.data.model.AIReportDetail
import com.fitness.management.data.model.AIReportStatus
import com.fitness.management.data.repository.FitnessRepository
import com.fitness.management.databinding.ActivityAiReportBinding
import com.fitness.management.utils.Extensions.toast
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class AIReportActivity : AppCompatActivity() {

    private lateinit var binding: ActivityAiReportBinding
    private val repository = FitnessRepository()
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)
    private var loadJob: Job? = null

    private var studentId: String = ""
    private var studentName: String = ""

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityAiReportBinding.inflate(layoutInflater)
        setContentView(binding.root)

        studentId = intent.getStringExtra("student_id") ?: ""
        studentName = intent.getStringExtra("student_name") ?: ""

        setupWebView()
        setupListeners()
        loadReport()
    }

    override fun onDestroy() {
        super.onDestroy()
        loadJob?.cancel()
        binding.webviewReport.destroy()
    }

    private fun setupWebView() {
        binding.webviewReport.apply {
            webViewClient = WebViewClient()
            settings.javaScriptEnabled = false
            settings.loadWithOverviewMode = true
            settings.useWideViewPort = true
            settings.builtInZoomControls = true
            settings.displayZoomControls = false
            setBackgroundColor(Color.parseColor("#F5F5F5"))
        }
    }

    private fun setupListeners() {
        binding.btnBack.setOnClickListener { finish() }
        binding.btnRetry.setOnClickListener { loadReport() }
    }

    private fun loadReport() {
        loadJob?.cancel()
        showLoading()

        loadJob = scope.launch {
            try {
                val result = withContext(Dispatchers.IO) {
                    repository.getWatchLatestReport(studentId)
                }

                result.fold(
                    onSuccess = { status ->
                        if (!status.hasReport) {
                            showEmpty()
                            return@launch
                        }

                        // Get the full report detail
                        val reportId = status.id ?: 0
                        if (reportId == 0) {
                            showEmpty()
                            return@launch
                        }

                        val detailResult = withContext(Dispatchers.IO) {
                            repository.getWatchReportDetail(reportId)
                        }

                        detailResult.fold(
                            onSuccess = { detail ->
                                showReport(detail.reportHtml ?: "", status, detail)
                            },
                            onFailure = { error ->
                                showError("加载报告详情失败: ${error.message}")
                            }
                        )
                    },
                    onFailure = { error ->
                        showError("无法获取报告: ${error.message}")
                    }
                )

            } catch (e: Exception) {
                showError("加载报告失败: ${e.message}")
            }
        }
    }

    private fun showLoading() {
        binding.progressBar.visibility = View.VISIBLE
        binding.webviewReport.visibility = View.GONE
        binding.layoutEmpty.visibility = View.GONE
        binding.layoutError.visibility = View.GONE
    }

    private fun showEmpty() {
        binding.progressBar.visibility = View.GONE
        binding.webviewReport.visibility = View.GONE
        binding.layoutError.visibility = View.GONE
        binding.layoutEmpty.visibility = View.VISIBLE
    }

    private fun showError(msg: String) {
        binding.progressBar.visibility = View.GONE
        binding.webviewReport.visibility = View.GONE
        binding.layoutEmpty.visibility = View.GONE
        binding.layoutError.visibility = View.VISIBLE
        binding.tvErrorMsg.text = msg
    }

    private fun showReport(html: String, status: AIReportStatus, detail: AIReportDetail?) {
        binding.progressBar.visibility = View.GONE
        binding.layoutEmpty.visibility = View.GONE
        binding.layoutError.visibility = View.GONE
        binding.webviewReport.visibility = View.VISIBLE

        val infoParts = mutableListOf<String>()
        status.weekStart?.let { infoParts.add(it) }
        detail?.provider?.let { infoParts.add(it) }
        if (infoParts.isNotEmpty()) {
            binding.tvReportInfo.text = infoParts.joinToString(" · ")
            binding.tvReportInfo.visibility = View.VISIBLE
        }

        val styledHtml = buildString {
            append("""<!DOCTYPE html><html><head>
                <meta charset="utf-8">
                <meta name="viewport" content="width=device-width,initial-scale=1.0,maximum-scale=3.0">
                <style>
                    body { font-family: -apple-system,'Microsoft YaHei',sans-serif; font-size:16px;
                           line-height:1.8; color:#333; padding:16px; margin:0;
                           background-color:#f5f5f5; }
                    h3 { color:#2E9E5A; margin-top:20px; margin-bottom:8px; font-size:18px; }
                    ul { padding-left:20px; }
                    li { margin-bottom:6px; }
                    p { margin:8px 0; }
                    strong { color:#1a1a1a; }
                    span { color:#555; }
                </style></head><body>${html}</body></html>""")
        }
        binding.webviewReport.loadDataWithBaseURL(null, styledHtml, "text/html", "UTF-8", null)
    }
}
