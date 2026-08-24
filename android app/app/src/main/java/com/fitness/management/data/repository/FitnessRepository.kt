package com.fitness.management.data.repository

import com.fitness.management.data.api.ApiService
import com.fitness.management.data.api.RetrofitClient
import com.fitness.management.data.model.*

class FitnessRepository {

    private val api: ApiService get() = RetrofitClient.api

    // ===== Auth =====
    suspend fun login(username: String, password: String): Result<LoginResponse> {
        return try {
            val resp = api.login(LoginRequest(username, password))
            if (resp.isSuccessful && resp.body() != null) {
                Result.success(resp.body()!!)
            } else {
                Result.failure(Exception("用户名或密码错误"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun getMe(): Result<UserInfo> {
        return try {
            val resp = api.getMe()
            if (resp.isSuccessful && resp.body() != null) {
                Result.success(resp.body()!!)
            } else {
                Result.failure(Exception("获取用户信息失败"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    // ===== Students =====
    suspend fun listStudents(grade: String? = null, classId: Int? = null, keyword: String? = null): Result<List<Student>> {
        return try {
            val resp = api.listStudents(grade, classId, keyword)
            if (resp.isSuccessful) Result.success(resp.body() ?: emptyList())
            else Result.failure(Exception("获取学生列表失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun getMyStudents(classId: Int? = null): Result<List<StudentListItem>> {
        return try {
            val resp = api.getMyStudents(classId)
            if (resp.isSuccessful) Result.success(resp.body() ?: emptyList())
            else Result.failure(Exception("获取学生列表失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    // ===== Fitness Records =====
    suspend fun listRecords(studentId: String? = null, testType: String? = null): Result<List<FitnessRecord>> {
        return try {
            val resp = api.listRecords(studentId, testType)
            if (resp.isSuccessful) Result.success(resp.body() ?: emptyList())
            else Result.failure(Exception("获取体测记录失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun getStudentRecords(studentId: String, testType: String? = null): Result<List<FitnessRecord>> {
        return try {
            val resp = api.getStudentRecords(studentId, testType)
            if (resp.isSuccessful) Result.success(resp.body() ?: emptyList())
            else Result.failure(Exception("获取体测记录失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun manualRecord(req: ManualRecordRequest): Result<FitnessRecord> {
        return try {
            val resp = api.manualRecord(req)
            if (resp.isSuccessful && resp.body() != null) Result.success(resp.body()!!)
            else Result.failure(Exception("录入失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    // ===== Statistics =====
    suspend fun getOverview(): Result<OverviewStats> {
        return try {
            val resp = api.getOverview()
            if (resp.isSuccessful && resp.body() != null) Result.success(resp.body()!!)
            else Result.failure(Exception("获取概览数据失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun getClassStatistics(classId: Int): Result<ClassStats> {
        return try {
            val resp = api.getClassStatistics(classId)
            if (resp.isSuccessful && resp.body() != null) Result.success(resp.body()!!)
            else Result.failure(Exception("获取班级统计失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun getStudentTrend(studentId: String, testType: String? = null): Result<List<TrendItem>> {
        return try {
            val resp = api.getStudentTrend(studentId, testType)
            if (resp.isSuccessful) Result.success(resp.body() ?: emptyList())
            else Result.failure(Exception("获取趋势数据失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun getLeaderboard(testType: String? = null): Result<List<LeaderboardItem>> {
        return try {
            val resp = api.getLeaderboard(testType)
            if (resp.isSuccessful) Result.success(resp.body() ?: emptyList())
            else Result.failure(Exception("获取排行榜失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    // ===== Classes =====
    suspend fun getMyClasses(): Result<List<ClassInfo>> {
        return try {
            val resp = api.getMyClasses()
            if (resp.isSuccessful) Result.success(resp.body() ?: emptyList())
            else Result.failure(Exception("获取班级列表失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun getAllClasses(): Result<List<ClassInfo>> {
        return try {
            val resp = api.getAllClasses()
            if (resp.isSuccessful) Result.success(resp.body() ?: emptyList())
            else Result.failure(Exception("获取班级列表失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    // ===== Devices =====
    suspend fun listDevices(activeOnly: Boolean = false): Result<List<Device>> {
        return try {
            val resp = api.listDevices(activeOnly)
            if (resp.isSuccessful) Result.success(resp.body() ?: emptyList())
            else Result.failure(Exception("获取设备列表失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun registerDevice(req: DeviceRegisterRequest): Result<Device> {
        return try {
            val resp = api.registerDevice(req)
            if (resp.isSuccessful && resp.body() != null) Result.success(resp.body()!!)
            else Result.failure(Exception("设备注册失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun unbindDevice(deviceId: String): Result<ApiResponse> {
        return try {
            val resp = api.unbindDevice(deviceId)
            if (resp.isSuccessful && resp.body() != null) Result.success(resp.body()!!)
            else Result.failure(Exception("解绑失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    // ===== Teacher Management (Admin) =====
    suspend fun getAllTeachers(): Result<List<TeacherInfo>> {
        return try {
            val resp = api.getAllTeachers()
            if (resp.isSuccessful) Result.success(resp.body() ?: emptyList())
            else Result.failure(Exception("获取教师列表失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun getAllBindings(): Result<List<BindingInfo>> {
        return try {
            val resp = api.getAllBindings()
            if (resp.isSuccessful) Result.success(resp.body() ?: emptyList())
            else Result.failure(Exception("获取绑定关系失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun adminBind(teacherId: Int, classId: Int): Result<ApiResponse> {
        return try {
            val resp = api.adminBind(TeacherBindRequest(teacherId, classId))
            if (resp.isSuccessful && resp.body() != null) Result.success(resp.body()!!)
            else Result.failure(Exception("绑定失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun adminUnbind(teacherId: Int, classId: Int): Result<ApiResponse> {
        return try {
            val resp = api.adminUnbind(TeacherBindRequest(teacherId, classId))
            if (resp.isSuccessful && resp.body() != null) Result.success(resp.body()!!)
            else Result.failure(Exception("解绑失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    // ===== AI Report =====
    suspend fun getWatchReports(studentId: String, limit: Int = 5): Result<List<AIReportItem>> {
        return try {
            val resp = api.getWatchReports(studentId, limit)
            if (resp.isSuccessful && resp.body() != null) Result.success(resp.body()!!)
            else Result.failure(Exception("获取报告列表失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun getWatchLatestReport(studentId: String): Result<AIReportStatus> {
        return try {
            val resp = api.getWatchLatestReport(studentId)
            if (resp.isSuccessful && resp.body() != null) Result.success(resp.body()!!)
            else Result.failure(Exception("获取最新报告失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }

    suspend fun getWatchReportDetail(reportId: Int): Result<AIReportDetail> {
        return try {
            val resp = api.getWatchReportDetail(reportId)
            if (resp.isSuccessful && resp.body() != null) Result.success(resp.body()!!)
            else Result.failure(Exception("获取报告详情失败"))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
}
