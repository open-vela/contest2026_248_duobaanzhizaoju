package com.fitness.management.data.api

import com.fitness.management.data.model.*
import retrofit2.Response
import retrofit2.http.*

interface ApiService {

    // ===== Auth =====
    @POST("/api/v1/auth/login")
    suspend fun login(@Body req: LoginRequest): Response<LoginResponse>

    @GET("/api/v1/auth/me")
    suspend fun getMe(): Response<UserInfo>

    // ===== Students =====
    @GET("/api/v1/students")
    suspend fun listStudents(
        @Query("grade") grade: String? = null,
        @Query("class_id") classId: Int? = null,
        @Query("keyword") keyword: String? = null,
        @Query("page") page: Int = 1,
        @Query("page_size") pageSize: Int = 50
    ): Response<List<Student>>

    // ===== Fitness Records =====
    @GET("/api/v1/fitness/records")
    suspend fun listRecords(
        @Query("student_id") studentId: String? = null,
        @Query("test_type") testType: String? = null,
        @Query("page") page: Int = 1,
        @Query("page_size") pageSize: Int = 50
    ): Response<List<FitnessRecord>>

    @GET("/api/v1/fitness/records/{student_id}")
    suspend fun getStudentRecords(
        @Path("student_id") studentId: String,
        @Query("test_type") testType: String? = null,
        @Query("limit") limit: Int = 100
    ): Response<List<FitnessRecord>>

    @POST("/api/v1/fitness/records/manual")
    suspend fun manualRecord(@Body req: ManualRecordRequest): Response<FitnessRecord>

    // ===== Statistics =====
    @GET("/api/v1/statistics/overview")
    suspend fun getOverview(): Response<OverviewStats>

    @GET("/api/v1/statistics/class/{class_id}")
    suspend fun getClassStatistics(
        @Path("class_id") classId: Int,
        @Query("test_type") testType: String? = null
    ): Response<ClassStats>

    @GET("/api/v1/statistics/trend/{student_id}")
    suspend fun getStudentTrend(
        @Path("student_id") studentId: String,
        @Query("test_type") testType: String? = null,
        @Query("limit") limit: Int = 20
    ): Response<List<TrendItem>>

    @GET("/api/v1/statistics/leaderboard")
    suspend fun getLeaderboard(
        @Query("test_type") testType: String? = null,
        @Query("limit") limit: Int = 20
    ): Response<List<LeaderboardItem>>

    // ===== Devices =====
    @GET("/api/v1/devices")
    suspend fun listDevices(@Query("active_only") activeOnly: Boolean = false): Response<List<Device>>

    @POST("/api/v1/devices/register")
    suspend fun registerDevice(@Body req: DeviceRegisterRequest): Response<Device>

    @PUT("/api/v1/devices/{device_id}/unbind")
    suspend fun unbindDevice(@Path("device_id") deviceId: String): Response<ApiResponse>

    // ===== Teacher-Class =====
    @GET("/api/v1/teacher-class/my-classes")
    suspend fun getMyClasses(): Response<List<ClassInfo>>

    @GET("/api/v1/teacher-class/students")
    suspend fun getMyStudents(@Query("class_id") classId: Int? = null): Response<List<StudentListItem>>

    @GET("/api/v1/teacher-class/all-classes")
    suspend fun getAllClasses(): Response<List<ClassInfo>>

    @GET("/api/v1/teacher-class/all-teachers")
    suspend fun getAllTeachers(): Response<List<TeacherInfo>>

    @GET("/api/v1/teacher-class/bindings")
    suspend fun getAllBindings(): Response<List<BindingInfo>>

    @POST("/api/v1/teacher-class/admin-bind")
    suspend fun adminBind(@Body req: TeacherBindRequest): Response<ApiResponse>

    @HTTP(method = "DELETE", path = "/api/v1/teacher-class/admin-unbind", hasBody = true)
    suspend fun adminUnbind(@Body req: TeacherBindRequest): Response<ApiResponse>

    // ===== AI Report =====
    @GET("/api/v1/ai/watch/reports")
    suspend fun getWatchReports(
        @Query("student_id") studentId: String,
        @Query("limit") limit: Int = 5
    ): Response<List<AIReportItem>>

    @GET("/api/v1/ai/watch/report/latest")
    suspend fun getWatchLatestReport(
        @Query("student_id") studentId: String
    ): Response<AIReportStatus>

    @GET("/api/v1/ai/watch/report/{report_id}")
    suspend fun getWatchReportDetail(
        @Path("report_id") reportId: Int
    ): Response<AIReportDetail>
}
