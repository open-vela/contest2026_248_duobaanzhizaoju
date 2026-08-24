"""体测数据路由"""
from datetime import datetime, timezone
from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select, func, and_

from database import get_db
from models import FitnessRecord, Student, TeacherClass
from schemas import FitnessRecordCreate, FitnessRecordOut, DeviceUpload, ApiResponse
from scoring import calculate_score
from auth import get_current_user

router = APIRouter(prefix="/api/v1/fitness", tags=["体测数据"])


async def _get_teacher_class_ids(user, db) -> list[int] | None:
    """获取教师的绑定班级ID列表，admin返回None表示不限"""
    if user.role == "admin":
        return None
    result = await db.execute(
        select(TeacherClass.class_id).where(TeacherClass.teacher_id == user.id)
    )
    return [row[0] for row in result.all()]


async def _get_teacher_student_ids(user, db) -> list[str] | None:
    """获取教师绑定班级的学生ID列表，admin返回None表示不限"""
    teacher_class_ids = await _get_teacher_class_ids(user, db)
    if teacher_class_ids is None:
        return None
    result = await db.execute(
        select(Student.student_id).where(Student.class_id.in_(teacher_class_ids))
    )
    return [row[0] for row in result.all()]


@router.post("/upload", response_model=ApiResponse)
async def upload_record(
    data: FitnessRecordCreate,
    db: AsyncSession = Depends(get_db),
):
    """设备端单条数据上传（无需认证，用device_id标识）"""
    # 验证学生存在
    stu = await db.execute(select(Student).where(Student.student_id == data.student_id))
    if not stu.scalar_one_or_none():
        raise HTTPException(status_code=400, detail=f"学生不存在: {data.student_id}")

    # 查学生性别用于评分
    stu_result = await db.execute(select(Student).where(Student.student_id == data.student_id))
    student = stu_result.scalar_one()

    # 自动评分
    score = None
    grade = None
    value = None
    if data.test_type in ("jump_rope", "sit_ups", "pull_up") and data.count is not None:
        value = float(data.count)
    elif data.test_type in ("50m_run", "800m_run", "1000m_run") and data.time_sec is not None:
        value = data.time_sec
    elif data.test_type == "long_jump" and data.distance_m is not None:
        value = data.distance_m * 100  # 转cm
    elif data.test_type == "sit_reach" and data.distance_m is not None:
        value = data.distance_m

    if value is not None:
        score, grade = calculate_score(data.test_type, student.gender, value)

    record = FitnessRecord(
        **data.model_dump(),
        score=score,
        grade=grade,
    )
    db.add(record)
    await db.flush()
    return ApiResponse(success=True, message="上传成功", data={"id": record.id, "score": score, "grade": grade})


@router.post("/upload/batch", response_model=ApiResponse)
async def upload_batch(
    data: DeviceUpload,
    db: AsyncSession = Depends(get_db),
):
    """设备批量上传"""
    success = 0
    failed = 0
    for rec in data.records:
        try:
            stu = await db.execute(select(Student).where(Student.student_id == rec.student_id))
            student = stu.scalar_one_or_none()
            if not student:
                failed += 1
                continue

            score = None
            grade = None
            value = None
            if rec.test_type in ("jump_rope", "sit_ups", "pull_up") and rec.count is not None:
                value = float(rec.count)
            elif rec.test_type in ("50m_run", "800m_run", "1000m_run") and rec.time_sec is not None:
                value = rec.time_sec
            elif rec.test_type == "long_jump" and rec.distance_m is not None:
                value = rec.distance_m * 100
            elif rec.test_type == "sit_reach" and rec.distance_m is not None:
                value = rec.distance_m

            if value is not None:
                score, grade = calculate_score(rec.test_type, student.gender, value)

            record = FitnessRecord(**rec.model_dump(), score=score, grade=grade)
            db.add(record)
            success += 1
        except Exception:
            failed += 1

    await db.flush()
    return ApiResponse(success=True, message=f"批量上传完成: 成功{success}条, 失败{failed}条",
                       data={"success": success, "failed": failed})


@router.get("/records", response_model=list[FitnessRecordOut])
async def list_records(
    student_id: str | None = None,
    test_type: str | None = None,
    start_date: datetime | None = None,
    end_date: datetime | None = None,
    page: int = Query(1, ge=1),
    page_size: int = Query(50, ge=1, le=200),
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    teacher_student_ids = await _get_teacher_student_ids(user, db)
    query = select(FitnessRecord)

    # 教师只能看到自己绑定班级学生的记录
    if teacher_student_ids is not None:
        if not teacher_student_ids:
            return []
        query = query.where(FitnessRecord.student_id.in_(teacher_student_ids))

    if student_id:
        # 教师如果请求了不在自己范围内的学生，返回空
        if teacher_student_ids is not None and student_id not in teacher_student_ids:
            return []
        query = query.where(FitnessRecord.student_id == student_id)
    if test_type:
        query = query.where(FitnessRecord.test_type == test_type)
    if start_date:
        query = query.where(FitnessRecord.test_date >= start_date)
    if end_date:
        query = query.where(FitnessRecord.test_date <= end_date)
    query = query.order_by(FitnessRecord.test_date.desc())
    query = query.offset((page - 1) * page_size).limit(page_size)
    result = await db.execute(query)
    return result.scalars().all()


@router.get("/records/{student_id}", response_model=list[FitnessRecordOut])
async def get_student_records(
    student_id: str,
    test_type: str | None = None,
    limit: int = Query(100, ge=1, le=500),
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    teacher_student_ids = await _get_teacher_student_ids(user, db)

    # 教师只能查看自己班级的学生记录
    if teacher_student_ids is not None and student_id not in teacher_student_ids:
        raise HTTPException(status_code=403, detail="无权查看该学生的数据")

    query = select(FitnessRecord).where(FitnessRecord.student_id == student_id)
    if test_type:
        query = query.where(FitnessRecord.test_type == test_type)
    query = query.order_by(FitnessRecord.test_date.desc()).limit(limit)
    result = await db.execute(query)
    return result.scalars().all()


@router.post("/records/manual", response_model=FitnessRecordOut)
async def manual_record(
    data: FitnessRecordCreate,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    """老师手动录入成绩"""
    stu = await db.execute(select(Student).where(Student.student_id == data.student_id))
    student = stu.scalar_one_or_none()
    if not student:
        raise HTTPException(status_code=400, detail="学生不存在")

    # 教师只能录入自己班级学生的成绩
    teacher_class_ids = await _get_teacher_class_ids(user, db)
    if teacher_class_ids is not None and student.class_id not in teacher_class_ids:
        raise HTTPException(status_code=403, detail="只能录入自己班级学生的成绩")

    score = None
    grade = None
    value = None
    if data.test_type in ("jump_rope", "sit_ups", "pull_up") and data.count is not None:
        value = float(data.count)
    elif data.test_type in ("50m_run", "800m_run", "1000m_run") and data.time_sec is not None:
        value = data.time_sec
    elif data.test_type == "long_jump" and data.distance_m is not None:
        value = data.distance_m * 100
    elif data.test_type == "sit_reach" and data.distance_m is not None:
        value = data.distance_m

    if value is not None:
        score, grade = calculate_score(data.test_type, student.gender, value)

    record = FitnessRecord(**data.model_dump(), score=score, grade=grade)
    db.add(record)
    await db.flush()
    await db.refresh(record)
    return record
