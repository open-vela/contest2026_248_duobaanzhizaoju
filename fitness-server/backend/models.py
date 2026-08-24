"""SQLAlchemy 数据模型"""
from datetime import datetime, date
from sqlalchemy import (
    String, Integer, Float, Boolean, Text, DateTime, Date, ForeignKey,
    JSON, func
)
from sqlalchemy.types import JSON as JSONType
from sqlalchemy.orm import Mapped, mapped_column, relationship

from database import Base


class School(Base):
    __tablename__ = "schools"

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    name: Mapped[str] = mapped_column(String(100), nullable=False)
    address: Mapped[str | None] = mapped_column(String(200))

    classes: Mapped[list["Class"]] = relationship(back_populates="school")
    students: Mapped[list["Student"]] = relationship(back_populates="school")


class Class(Base):
    __tablename__ = "classes"

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    school_id: Mapped[int] = mapped_column(ForeignKey("schools.id"))
    grade: Mapped[str] = mapped_column(String(10))  # 初一/初二/初三
    class_name: Mapped[str] = mapped_column(String(10))  # 1班/2班
    head_teacher: Mapped[str | None] = mapped_column(String(50))

    school: Mapped["School"] = relationship(back_populates="classes")
    students: Mapped[list["Student"]] = relationship(back_populates="class_info")


class Student(Base):
    __tablename__ = "students"

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    student_id: Mapped[str] = mapped_column(String(20), unique=True, nullable=False, index=True)
    name: Mapped[str] = mapped_column(String(50), nullable=False)
    gender: Mapped[str] = mapped_column(String(1))  # M/F
    birth_date: Mapped[date | None] = mapped_column(Date)
    grade: Mapped[str] = mapped_column(String(10))
    class_id: Mapped[int | None] = mapped_column(ForeignKey("classes.id"))
    school_id: Mapped[int | None] = mapped_column(ForeignKey("schools.id"))
    height_cm: Mapped[float | None] = mapped_column(Float)
    weight_kg: Mapped[float | None] = mapped_column(Float)
    created_at: Mapped[datetime] = mapped_column(DateTime, server_default=func.now())

    school: Mapped["School | None"] = relationship(back_populates="students")
    class_info: Mapped["Class | None"] = relationship(back_populates="students")
    records: Mapped[list["FitnessRecord"]] = relationship(back_populates="student")
    devices: Mapped[list["Device"]] = relationship(back_populates="student")


class Device(Base):
    __tablename__ = "devices"

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    device_id: Mapped[str] = mapped_column(String(50), unique=True, nullable=False, index=True)
    student_id: Mapped[str | None] = mapped_column(ForeignKey("students.student_id"))
    device_name: Mapped[str | None] = mapped_column(String(100))
    firmware_version: Mapped[str | None] = mapped_column(String(20))
    battery_level: Mapped[int | None] = mapped_column(Integer)
    last_online: Mapped[datetime | None] = mapped_column(DateTime)
    is_active: Mapped[bool] = mapped_column(Boolean, default=True)
    registered_at: Mapped[datetime] = mapped_column(DateTime, server_default=func.now())

    student: Mapped["Student | None"] = relationship(back_populates="devices")


class FitnessRecord(Base):
    __tablename__ = "fitness_records"

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    student_id: Mapped[str] = mapped_column(ForeignKey("students.student_id"), index=True)
    device_id: Mapped[str | None] = mapped_column(String(50))
    test_type: Mapped[str] = mapped_column(String(30), nullable=False, index=True)
    test_date: Mapped[datetime] = mapped_column(DateTime, nullable=False, index=True)
    duration_sec: Mapped[int | None] = mapped_column(Integer)
    count: Mapped[int | None] = mapped_column(Integer)
    distance_m: Mapped[float | None] = mapped_column(Float)
    time_sec: Mapped[float | None] = mapped_column(Float)
    avg_hr: Mapped[int | None] = mapped_column(Integer)
    max_hr: Mapped[int | None] = mapped_column(Integer)
    hr_zones: Mapped[dict | None] = mapped_column(JSONType)
    score: Mapped[int | None] = mapped_column(Integer)
    grade: Mapped[str | None] = mapped_column(String(10))
    raw_data: Mapped[str | None] = mapped_column(Text)
    note: Mapped[str | None] = mapped_column(String(200))
    uploaded_at: Mapped[datetime] = mapped_column(DateTime, server_default=func.now())

    student: Mapped["Student"] = relationship(back_populates="records")


class User(Base):
    """系统用户（老师/管理员）"""
    __tablename__ = "users"

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    username: Mapped[str] = mapped_column(String(50), unique=True, nullable=False)
    password_hash: Mapped[str] = mapped_column(String(200), nullable=False)
    real_name: Mapped[str] = mapped_column(String(50))
    role: Mapped[str] = mapped_column(String(20), default="teacher")  # admin/teacher
    phone: Mapped[str | None] = mapped_column(String(20))
    is_active: Mapped[bool] = mapped_column(Boolean, default=True)
    created_at: Mapped[datetime] = mapped_column(DateTime, server_default=func.now())


class TeacherClass(Base):
    """教师-班级多对多绑定"""
    __tablename__ = "teacher_class"

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    teacher_id: Mapped[int] = mapped_column(ForeignKey("users.id"), index=True)
    class_id: Mapped[int] = mapped_column(ForeignKey("classes.id"), index=True)
    created_at: Mapped[datetime] = mapped_column(DateTime, server_default=func.now())


class ParentStudent(Base):
    """家长-学生关联"""
    __tablename__ = "parent_student"

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    parent_phone: Mapped[str] = mapped_column(String(20), index=True)
    student_id: Mapped[str] = mapped_column(ForeignKey("students.student_id"))
    relationship: Mapped[str] = mapped_column(String(20))  # 父亲/母亲/其他
    created_at: Mapped[datetime] = mapped_column(DateTime, server_default=func.now())


class AIConfig(Base):
    """AI 大模型配置（全局唯一）"""
    __tablename__ = "ai_config"

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    provider: Mapped[str] = mapped_column(String(30), nullable=False)  # openai/deepseek/zhipu/qwen/doubao/kimi
    model_name: Mapped[str] = mapped_column(String(50), nullable=False)  # gpt-4o / deepseek-chat / glm-4 ...
    api_key: Mapped[str] = mapped_column(String(200), nullable=False)
    api_base: Mapped[str | None] = mapped_column(String(200))  # 自定义 endpoint
    is_active: Mapped[bool] = mapped_column(Boolean, default=True)
    created_at: Mapped[datetime] = mapped_column(DateTime, server_default=func.now())
    updated_at: Mapped[datetime] = mapped_column(DateTime, server_default=func.now(), onupdate=func.now())


class AIReport(Base):
    """AI 生成的个人周报"""
    __tablename__ = "ai_reports"

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    student_id: Mapped[str] = mapped_column(ForeignKey("students.student_id"), index=True)
    week_start: Mapped[date] = mapped_column(Date, nullable=False)  # 本周起始日(周一)
    week_end: Mapped[date] = mapped_column(Date, nullable=False)    # 本周结束日(周日)
    report_html: Mapped[str] = mapped_column(Text, nullable=False)  # 报告 HTML 内容
    report_summary: Mapped[str | None] = mapped_column(String(500))  # 摘要(手表端快速展示)
    provider: Mapped[str] = mapped_column(String(30))  # 使用的 AI 服务商
    model_name: Mapped[str] = mapped_column(String(50))  # 使用的模型
    created_at: Mapped[datetime] = mapped_column(DateTime, server_default=func.now())

    student: Mapped["Student"] = relationship()


# 收集所有模型供 init_db 使用
all_models = [School, Class, Student, Device, FitnessRecord, User, ParentStudent, TeacherClass, AIConfig, AIReport]
