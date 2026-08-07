package com.fitness.management.data.model

import com.google.gson.annotations.SerializedName

// ===== Auth =====
data class LoginRequest(
    val username: String,
    val password: String
)

data class LoginResponse(
    @SerializedName("access_token") val accessToken: String,
    @SerializedName("token_type") val tokenType: String = "bearer",
    val role: String,
    @SerializedName("real_name") val realName: String
)

data class UserInfo(
    val id: Int,
    val username: String,
    @SerializedName("real_name") val realName: String,
    val role: String
)

// ===== Student =====
data class Student(
    val id: Int,
    @SerializedName("student_id") val studentId: String,
    val name: String,
    val gender: String,
    @SerializedName("birth_date") val birthDate: String? = null,
    val grade: String,
    @SerializedName("class_id") val classId: Int? = null,
    @SerializedName("height_cm") val heightCm: Float? = null,
    @SerializedName("weight_kg") val weightKg: Float? = null
)

data class StudentListItem(
    @SerializedName("student_id") val studentId: String,
    val name: String,
    val gender: String,
    val grade: String,
    @SerializedName("class_id") val classId: Int? = null,
    @SerializedName("height_cm") val heightCm: Float? = null,
    @SerializedName("weight_kg") val weightKg: Float? = null
)

// ===== Fitness Record =====
data class FitnessRecord(
    val id: Int,
    @SerializedName("student_id") val studentId: String,
    @SerializedName("device_id") val deviceId: String? = null,
    @SerializedName("test_type") val testType: String,
    @SerializedName("test_date") val testDate: String,
    @SerializedName("duration_sec") val durationSec: Int? = null,
    val count: Int? = null,
    @SerializedName("distance_m") val distanceM: Float? = null,
    @SerializedName("time_sec") val timeSec: Float? = null,
    @SerializedName("avg_hr") val avgHr: Int? = null,
    @SerializedName("max_hr") val maxHr: Int? = null,
    @SerializedName("hr_zones") val hrZones: Map<String, Int>? = null,
    val score: Int? = null,
    val grade: String? = null,
    val note: String? = null,
    @SerializedName("uploaded_at") val uploadedAt: String
)

data class ManualRecordRequest(
    @SerializedName("student_id") val studentId: String,
    @SerializedName("device_id") val deviceId: String? = null,
    @SerializedName("test_type") val testType: String,
    @SerializedName("test_date") val testDate: String,
    @SerializedName("duration_sec") val durationSec: Int? = null,
    val count: Int? = null,
    @SerializedName("distance_m") val distanceM: Float? = null,
    @SerializedName("time_sec") val timeSec: Float? = null,
    @SerializedName("avg_hr") val avgHr: Int? = null,
    @SerializedName("max_hr") val maxHr: Int? = null,
    val note: String? = null
)

// ===== Statistics =====
data class OverviewStats(
    @SerializedName("total_students") val totalStudents: Int,
    @SerializedName("total_records") val totalRecords: Int,
    @SerializedName("today_records") val todayRecords: Int,
    @SerializedName("avg_score") val avgScore: Double,
    @SerializedName("pass_rate") val passRate: Double,
    @SerializedName("excellent_rate") val excellentRate: Double
)

data class ClassStats(
    @SerializedName("total_students") val totalStudents: Int,
    @SerializedName("tested_students") val testedStudents: Int,
    @SerializedName("test_rate") val testRate: Double,
    @SerializedName("avg_score") val avgScore: Double,
    @SerializedName("pass_rate") val passRate: Double,
    @SerializedName("excellent_rate") val excellentRate: Double,
    @SerializedName("test_type_stats") val testTypeStats: List<TestTypeStat>
)

data class TestTypeStat(
    @SerializedName("test_type") val testType: String,
    val label: String,
    val count: Int,
    @SerializedName("avg_score") val avgScore: Double,
    @SerializedName("max_score") val maxScore: Int,
    @SerializedName("min_score") val minScore: Int
)

data class TrendItem(
    @SerializedName("test_date") val testDate: String,
    @SerializedName("test_type") val testType: String,
    val label: String,
    val score: Int? = null,
    val count: Int? = null,
    @SerializedName("time_sec") val timeSec: Float? = null,
    @SerializedName("distance_m") val distanceM: Float? = null,
    val grade: String? = null
)

data class LeaderboardItem(
    val rank: Int,
    @SerializedName("student_id") val studentId: String,
    val name: String,
    val grade: String,
    @SerializedName("best_score") val bestScore: Int,
    @SerializedName("test_date") val testDate: String?
)

// ===== Class =====
data class ClassInfo(
    val id: Int,
    val grade: String,
    @SerializedName("class_name") val className: String,
    @SerializedName("head_teacher") val headTeacher: String?
)

// ===== Device =====
data class Device(
    val id: Int,
    @SerializedName("device_id") val deviceId: String,
    @SerializedName("student_id") val studentId: String?,
    @SerializedName("device_name") val deviceName: String?,
    @SerializedName("battery_level") val batteryLevel: Int?,
    @SerializedName("last_online") val lastOnline: String?,
    @SerializedName("is_active") val isActive: Boolean
)

data class DeviceRegisterRequest(
    @SerializedName("device_id") val deviceId: String,
    @SerializedName("device_name") val deviceName: String? = null,
    @SerializedName("student_id") val studentId: String? = null
)

// ===== Teacher =====
data class TeacherInfo(
    val id: Int,
    val username: String,
    @SerializedName("real_name") val realName: String,
    val phone: String?
)

data class BindingInfo(
    val id: Int,
    @SerializedName("teacher_id") val teacherId: Int,
    @SerializedName("teacher_name") val teacherName: String,
    @SerializedName("teacher_username") val teacherUsername: String,
    @SerializedName("class_id") val classId: Int,
    @SerializedName("class_name") val className: String
)

data class TeacherBindRequest(
    @SerializedName("teacher_id") val teacherId: Int,
    @SerializedName("class_id") val classId: Int
)

// ===== AI Report =====
data class AIReportItem(
    val id: Int,
    @SerializedName("week_start") val weekStart: String,
    @SerializedName("week_end") val weekEnd: String,
    val summary: String?,
    @SerializedName("created_at") val createdAt: String?
)

data class AIReportDetail(
    val id: Int,
    @SerializedName("student_id") val studentId: String,
    @SerializedName("week_start") val weekStart: String,
    @SerializedName("week_end") val weekEnd: String,
    @SerializedName("report_html") val reportHtml: String?,
    val summary: String?,
    val provider: String?,
    @SerializedName("model_name") val modelName: String?,
    @SerializedName("created_at") val createdAt: String?
)

data class AIReportStatus(
    @SerializedName("has_report") val hasReport: Boolean,
    val message: String?,
    val id: Int? = null,
    @SerializedName("week_start") val weekStart: String? = null,
    @SerializedName("week_end") val weekEnd: String? = null,
    val summary: String? = null,
    @SerializedName("created_at") val createdAt: String? = null
)

// ===== Generic =====
data class ApiResponse(
    val success: Boolean = true,
    val message: String = "",
    val data: Any? = null
)
