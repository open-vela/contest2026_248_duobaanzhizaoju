package com.fitness.management.utils

import android.content.Context
import android.content.SharedPreferences

object SessionManager {

    private lateinit var prefs: SharedPreferences

    fun init(context: Context) {
        prefs = context.getSharedPreferences(Constants.PREF_NAME, Context.MODE_PRIVATE)
    }

    fun saveLogin(token: String, role: String, realName: String, username: String) {
        prefs.edit().apply {
            putString(Constants.KEY_TOKEN, token)
            putString(Constants.KEY_ROLE, role)
            putString(Constants.KEY_REAL_NAME, realName)
            putString(Constants.KEY_USERNAME, username)
            apply()
        }
    }

    val token: String? get() = prefs.getString(Constants.KEY_TOKEN, null)

    val role: String? get() = prefs.getString(Constants.KEY_ROLE, null)

    val realName: String? get() = prefs.getString(Constants.KEY_REAL_NAME, null)

    val username: String? get() = prefs.getString(Constants.KEY_USERNAME, null)

    fun isLoggedIn(): Boolean = token != null

    fun isAdmin(): Boolean = role == Constants.ROLE_ADMIN

    fun logout() {
        prefs.edit().clear().apply()
    }

    val baseUrl: String
        get() = prefs.getString(Constants.KEY_BASE_URL, Constants.DEFAULT_BASE_URL) ?: Constants.DEFAULT_BASE_URL

    fun setBaseUrl(url: String) {
        prefs.edit().putString(Constants.KEY_BASE_URL, url).apply()
    }
}
