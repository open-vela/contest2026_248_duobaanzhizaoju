package com.fitness.management.utils

import android.content.Context
import android.widget.Toast
import com.google.gson.Gson
import com.google.gson.JsonObject

object Extensions {
    fun Context.toast(msg: String) {
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
    }

    fun Context.toastLong(msg: String) {
        Toast.makeText(this, msg, Toast.LENGTH_LONG).show()
    }

    fun extractErrorMessage(errorBody: String?): String {
        if (errorBody.isNullOrBlank()) return "请求失败"
        return try {
            val json = Gson().fromJson(errorBody, JsonObject::class.java)
            json.get("detail")?.asString ?: "请求失败"
        } catch (e: Exception) {
            "请求失败"
        }
    }
}
