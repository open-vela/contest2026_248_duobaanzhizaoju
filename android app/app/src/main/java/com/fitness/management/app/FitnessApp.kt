package com.fitness.management.app

import android.app.Application
import com.fitness.management.data.api.RetrofitClient
import com.fitness.management.utils.SessionManager

class FitnessApp : Application() {
    override fun onCreate() {
        super.onCreate()
        SessionManager.init(this)
        RetrofitClient.init(SessionManager.baseUrl)
    }
}
