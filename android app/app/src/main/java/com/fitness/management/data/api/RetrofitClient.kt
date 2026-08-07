package com.fitness.management.data.api

import com.fitness.management.utils.SessionManager
import okhttp3.OkHttpClient
import okhttp3.logging.HttpLoggingInterceptor
import retrofit2.Retrofit
import retrofit2.converter.gson.GsonConverterFactory
import java.util.concurrent.TimeUnit

object RetrofitClient {

    private var retrofit: Retrofit? = null
    private var apiService: ApiService? = null
    private var currentBaseUrl: String? = null

    fun init(baseUrl: String) {
        currentBaseUrl = baseUrl

        val loggingInterceptor = HttpLoggingInterceptor().apply {
            level = HttpLoggingInterceptor.Level.BODY
        }

        val client = OkHttpClient.Builder()
            .addInterceptor { chain ->
                val original = chain.request()
                val token = SessionManager.token
                val request = original.newBuilder().apply {
                    if (token != null) {
                        header("Authorization", "Bearer $token")
                    }
                    header("Content-Type", "application/json")
                }.build()
                chain.proceed(request)
            }
            .addInterceptor(loggingInterceptor)
            .connectTimeout(30, TimeUnit.SECONDS)
            .readTimeout(30, TimeUnit.SECONDS)
            .writeTimeout(30, TimeUnit.SECONDS)
            .build()

        retrofit = Retrofit.Builder()
            .baseUrl(if (baseUrl.endsWith("/")) baseUrl else "$baseUrl/")
            .client(client)
            .addConverterFactory(GsonConverterFactory.create())
            .build()

        apiService = retrofit!!.create(ApiService::class.java)
    }

    val api: ApiService
        get() {
            if (apiService == null) {
                init(SessionManager.baseUrl ?: "http://101.35.231.154:9000")
            }
            return apiService!!
        }

    fun rebuild() {
        currentBaseUrl?.let { init(it) }
    }
}
