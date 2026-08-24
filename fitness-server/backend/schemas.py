"""Pydantic 请求/响应模型"""
from datetime import datetime, date
from pydantic import BaseModel, Field


# ===== 认证 =====
class Token(BaseModel):
    access_token: str
    token_type: str = "bearer"
    role: str
    real_name: str


class LoginRequest(BaseModel):
    username: str
    password: str


# ===== 学生 =====
class StudentCreate(BaseModel):
    student_id: str
    name: str
    gender: str = "M"
    birth_date: date | None = None
    grade: str
    class_id: int | None = None
    school_id: int | None = None
    height_cm: float | None = None
    weight_kg: float | None = None


class StudentOut(BaseModel):
    id: int
    student_id: str
    name: str
    gender: str
    birth_date: date | None
    grade: str
    class_id: int | None
    height_cm: float | None
    weight_kg: float | None

    class Config:
        from_attributes = True


# ===== 设备 =====
class DeviceRegister(BaseModel):
    device_id: str
    device_name: str | None = None
    student_id: str | None = None


class DeviceOut(BaseModel):
    id: int
    device_id: str
    student_id: str | None
    device_name: str | None
    battery_level: int | None
    last_online: datetime | None
    is_active: bool

    class Config:
        from_attributes = True


# ===== 体测数据 =====
class FitnessRecordCreate(BaseModel):
    student_id: str
    device_id: str | None = None
    test_type: str
    test_date: datetime
    duration_sec: int | None = None
    count: int | None = None
    distance_m: float | None = None
    time_sec: float | None = None
    avg_hr: int | None = None
    max_hr: int | None = None
    hr_zones: dict | None = None
    raw_data: str | None = None
    note: str | None = None


class DeviceUpload(BaseModel):
    """设备批量上传"""
    device_id: str
    records: list[FitnessRecordCreate]


class FitnessRecordOut(BaseModel):
    id: int
    student_id: str
    device_id: str | None
    test_type: str
    test_date: datetime
    duration_sec: int | None
    count: int | None
    distance_m: float | None
    time_sec: float | None
    avg_hr: int | None
    max_hr: int | None
    hr_zones: dict | None
    score: int | None
    grade: str | None
    note: str | None
    uploaded_at: datetime

    class Config:
        from_attributes = True


# ===== 统计 =====
class ClassStatistics(BaseModel):
    total_students: int
    tested_students: int
    avg_score: float
    pass_rate: float
    excellent_rate: float
    test_type_stats: list[dict]


class TrendItem(BaseModel):
    test_date: datetime
    test_type: str
    score: int | None
    count: int | None
    time_sec: float | None


# ===== 教师绑定 =====
class TeacherBindRequest(BaseModel):
    teacher_id: int
    class_id: int


# ===== 通用 =====
class ApiResponse(BaseModel):
    success: bool = True
    message: str = ""
    data: dict | list | None = None


class PaginatedResponse(BaseModel):
    total: int
    page: int
    page_size: int
    items: list


# ===== AI 报告 =====
class AIConfigCreate(BaseModel):
    provider: str = Field(..., description="AI 服务商: openai/deepseek/zhipu/qwen/doubao/kimi/mimo")
    model_name: str = Field(..., description="模型名称，如 gpt-4o / deepseek-chat / glm-4-plus")
    api_key: str = Field(..., description="API Key")
    api_base: str | None = Field(None, description="自定义 API 地址（留空使用默认）")


class AIConfigOut(BaseModel):
    id: int
    provider: str
    model_name: str
    api_key_masked: str = Field(description="掩码后的 API Key")
    api_base: str | None
    is_active: bool
    created_at: datetime

    class Config:
        from_attributes = True


class AIReportOut(BaseModel):
    id: int
    student_id: str
    student_name: str | None = None
    week_start: date
    week_end: date
    report_summary: str | None
    provider: str
    model_name: str
    created_at: datetime

    class Config:
        from_attributes = True


class AIReportDetail(BaseModel):
    id: int
    student_id: str
    student_name: str | None = None
    week_start: date
    week_end: date
    report_html: str
    report_summary: str | None
    provider: str
    model_name: str
    created_at: datetime
