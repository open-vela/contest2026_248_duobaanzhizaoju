package com.fitness.management.utils

object Constants {
    // API
    const val DEFAULT_BASE_URL = "http://101.35.231.154:9000"
    const val API_PREFIX = "/api/v1"

    // Session keys
    const val PREF_NAME = "fitness_session"
    const val KEY_TOKEN = "access_token"
    const val KEY_ROLE = "role"
    const val KEY_REAL_NAME = "real_name"
    const val KEY_USERNAME = "username"
    const val KEY_BASE_URL = "base_url"

    // Roles
    const val ROLE_ADMIN = "admin"
    const val ROLE_TEACHER = "teacher"

    // Test types
    val TEST_TYPE_LABELS = mapOf(
        "jump_rope" to "跳绳",
        "50m_run" to "50米跑",
        "800m_run" to "800米跑",
        "1000m_run" to "1000米跑",
        "long_jump" to "立定跳远",
        "sit_ups" to "仰卧起坐",
        "pull_up" to "引体向上",
        "sit_reach" to "坐位体前屈"
    )

    val GRADE_LABELS = mapOf(
        "excellent" to "优秀",
        "good" to "良好",
        "pass" to "及格",
        "fail" to "不及格"
    )

    val GRADE_COLORS = mapOf(
        "excellent" to 0xFF10B981.toInt(),
        "good" to 0xFF2E9E5A.toInt(),
        "pass" to 0xFFF59E0B.toInt(),
        "fail" to 0xFFEF4444.toInt()
    )

    // Colors
    const val COLOR_BRAND_BLUE = 0xFF2E9E5A.toInt()
    const val COLOR_SUCCESS_GREEN = 0xFF10B981.toInt()
    const val COLOR_WARNING_ORANGE = 0xFFF59E0B.toInt()
    const val COLOR_DANGER_RED = 0xFFEF4444.toInt()
    const val COLOR_PURPLE = 0xFF8B5CF6.toInt()
    const val COLOR_PINK = 0xFFEC4899.toInt()
    const val COLOR_BLUE_MALE = 0xFF3B82F6.toInt()
    const val COLOR_TEXT_PRIMARY = 0xFF1A2E1A.toInt()
    const val COLOR_TEXT_SECONDARY = 0xFF6B7B6B.toInt()
    const val COLOR_BG_MAIN = 0xFFF5FAF5.toInt()
    const val COLOR_WHITE = 0xFFFFFFFF.toInt()
}
