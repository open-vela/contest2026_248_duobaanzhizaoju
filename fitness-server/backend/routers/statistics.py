"""统计分析路由"""
from datetime import datetime, timedelta
from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select, func, and_, case

from database import get_db
from models import FitnessRecord, Student, Class, TeacherClass
from auth import get_current_user
from scoring import TEST_TYPE_LABELS

router = APIRouter(prefix="/api/v1/statistics", tags=["统计分析"])


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


@router.get("/overview")
async def overview(
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    """总览统计（教师只看自己班级数据）"""
    teacher_class_ids = await _get_teacher_class_ids(user, db)
    teacher_student_ids = await _get_teacher_student_ids(user, db)

    # 学生总数
    if teacher_class_ids is not None:
        if not teacher_class_ids:
            return {
                "total_students": 0, "total_records": 0, "today_records": 0,
                "avg_score": 0, "pass_rate": 0, "excellent_rate": 0,
            }
        total_students = await db.scalar(
            select(func.count(Student.id)).where(Student.class_id.in_(teacher_class_ids))
        )
    else:
        total_students = await db.scalar(select(func.count(Student.id)))

    # 记录总数
    if teacher_student_ids is not None:
        total_records = await db.scalar(
            select(func.count(FitnessRecord.id)).where(FitnessRecord.student_id.in_(teacher_student_ids))
        )
    else:
        total_records = await db.scalar(select(func.count(FitnessRecord.id)))

    # 今日新增
    today = datetime.now().replace(hour=0, minute=0, second=0, microsecond=0)
    if teacher_student_ids is not None:
        today_records = await db.scalar(
            select(func.count(FitnessRecord.id)).where(
                FitnessRecord.test_date >= today,
                FitnessRecord.student_id.in_(teacher_student_ids)
            )
        )
    else:
        today_records = await db.scalar(
            select(func.count(FitnessRecord.id)).where(FitnessRecord.test_date >= today)
        )

    # 平均分
    if teacher_student_ids is not None:
        avg_score = await db.scalar(
            select(func.avg(FitnessRecord.score)).where(FitnessRecord.student_id.in_(teacher_student_ids))
        )
    else:
        avg_score = await db.scalar(select(func.avg(FitnessRecord.score)))

    # 及格率
    if teacher_student_ids is not None:
        pass_count = await db.scalar(
            select(func.count(FitnessRecord.id)).where(
                FitnessRecord.score >= 60,
                FitnessRecord.student_id.in_(teacher_student_ids)
            )
        )
    else:
        pass_count = await db.scalar(
            select(func.count(FitnessRecord.id)).where(FitnessRecord.score >= 60)
        )
    pass_rate = (pass_count / total_records * 100) if total_records else 0

    # 优秀率
    if teacher_student_ids is not None:
        excellent_count = await db.scalar(
            select(func.count(FitnessRecord.id)).where(
                FitnessRecord.score >= 90,
                FitnessRecord.student_id.in_(teacher_student_ids)
            )
        )
    else:
        excellent_count = await db.scalar(
            select(func.count(FitnessRecord.id)).where(FitnessRecord.score >= 90)
        )
    excellent_rate = (excellent_count / total_records * 100) if total_records else 0

    return {
        "total_students": total_students or 0,
        "total_records": total_records or 0,
        "today_records": today_records or 0,
        "avg_score": round(avg_score, 1) if avg_score else 0,
        "pass_rate": round(pass_rate, 1),
        "excellent_rate": round(excellent_rate, 1),
    }


@router.get("/class/{class_id}")
async def class_statistics(
    class_id: int,
    test_type: str | None = None,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    """班级统计（教师只能查看自己绑定的班级）"""
    teacher_class_ids = await _get_teacher_class_ids(user, db)
    if teacher_class_ids is not None and class_id not in teacher_class_ids:
        raise HTTPException(status_code=403, detail="只能查看自己绑定的班级数据")

    # 班级学生数
    total_students = await db.scalar(
        select(func.count(Student.id)).where(Student.class_id == class_id)
    )

    # 查询成绩
    query = (
        select(FitnessRecord)
        .join(Student, FitnessRecord.student_id == Student.student_id)
        .where(Student.class_id == class_id)
    )
    if test_type:
        query = query.where(FitnessRecord.test_type == test_type)

    result = await db.execute(query)
    records = result.scalars().all()

    tested_students = len(set(r.student_id for r in records))
    scores = [r.score for r in records if r.score is not None]
    avg_score = sum(scores) / len(scores) if scores else 0
    pass_count = sum(1 for s in scores if s >= 60)
    excellent_count = sum(1 for s in scores if s >= 90)

    # 按项目分组统计
    type_stats = {}
    for r in records:
        if r.test_type not in type_stats:
            type_stats[r.test_type] = {
                "test_type": r.test_type,
                "label": TEST_TYPE_LABELS.get(r.test_type, r.test_type),
                "count": 0,
                "scores": [],
            }
        type_stats[r.test_type]["count"] += 1
        if r.score is not None:
            type_stats[r.test_type]["scores"].append(r.score)

    type_list = []
    for ts in type_stats.values():
        s = ts["scores"]
        ts["avg_score"] = round(sum(s) / len(s), 1) if s else 0
        ts["max_score"] = max(s) if s else 0
        ts["min_score"] = min(s) if s else 0
        del ts["scores"]
        type_list.append(ts)

    return {
        "total_students": total_students or 0,
        "tested_students": tested_students,
        "test_rate": round(tested_students / total_students * 100, 1) if total_students else 0,
        "avg_score": round(avg_score, 1),
        "pass_rate": round(pass_count / len(scores) * 100, 1) if scores else 0,
        "excellent_rate": round(excellent_count / len(scores) * 100, 1) if scores else 0,
        "test_type_stats": type_list,
    }


@router.get("/trend/{student_id}")
async def student_trend(
    student_id: str,
    test_type: str | None = None,
    limit: int = Query(20, ge=1, le=100),
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    """学生成绩趋势（教师只能查看自己班级的学生）"""
    teacher_student_ids = await _get_teacher_student_ids(user, db)
    if teacher_student_ids is not None and student_id not in teacher_student_ids:
        raise HTTPException(status_code=403, detail="只能查看自己班级学生的数据")

    query = select(FitnessRecord).where(FitnessRecord.student_id == student_id)
    if test_type:
        query = query.where(FitnessRecord.test_type == test_type)
    query = query.order_by(FitnessRecord.test_date.desc()).limit(limit)
    result = await db.execute(query)
    records = result.scalars().all()

    return [
        {
            "test_date": r.test_date.isoformat(),
            "test_type": r.test_type,
            "label": TEST_TYPE_LABELS.get(r.test_type, r.test_type),
            "score": r.score,
            "count": r.count,
            "time_sec": r.time_sec,
            "distance_m": r.distance_m,
            "grade": r.grade,
        }
        for r in reversed(records)
    ]


@router.get("/leaderboard")
async def leaderboard(
    test_type: str | None = None,
    limit: int = Query(20, ge=1, le=100),
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    """排行榜（教师只能看到自己班级的学生）"""
    teacher_student_ids = await _get_teacher_student_ids(user, db)

    query = (
        select(
            FitnessRecord.student_id,
            Student.name,
            Student.grade,
            func.max(FitnessRecord.score).label("best_score"),
            func.max(FitnessRecord.test_date).label("test_date"),
        )
        .join(Student, FitnessRecord.student_id == Student.student_id)
        .where(FitnessRecord.score.isnot(None))
    )

    if teacher_student_ids is not None:
        if not teacher_student_ids:
            return []
        query = query.where(FitnessRecord.student_id.in_(teacher_student_ids))

    if test_type:
        query = query.where(FitnessRecord.test_type == test_type)
    query = query.group_by(FitnessRecord.student_id, Student.name, Student.grade)
    query = query.order_by(func.max(FitnessRecord.score).desc()).limit(limit)

    result = await db.execute(query)
    rows = result.all()

    return [
        {
            "rank": i + 1,
            "student_id": row.student_id,
            "name": row.name,
            "grade": row.grade,
            "best_score": row.best_score,
            "test_date": row.test_date.isoformat() if row.test_date else None,
        }
        for i, row in enumerate(rows)
    ]
