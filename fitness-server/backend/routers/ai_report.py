"""AI 报告路由 — 配置管理 + 报告生成 + 报告查看"""
from datetime import date, timedelta
from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select, func

from database import get_db
from models import AIConfig, AIReport, Student, TeacherClass
from schemas import (
    AIConfigCreate, AIConfigOut, AIReportOut, AIReportDetail, ApiResponse
)
from auth import get_current_user, require_admin
from ai_engine import (
    get_active_config, mask_key, generate_weekly_report,
    get_current_week_range
)

router = APIRouter(prefix="/api/v1/ai", tags=["AI 报告"])


# ==================== AI 配置管理（管理员） ====================

@router.get("/config", response_model=list[AIConfigOut])
async def list_ai_configs(
    db: AsyncSession = Depends(get_db),
    _=Depends(require_admin),
):
    """获取所有 AI 配置列表"""
    result = await db.execute(select(AIConfig).order_by(AIConfig.id.desc()))
    configs = result.scalars().all()
    return [
        AIConfigOut(
            id=c.id,
            provider=c.provider,
            model_name=c.model_name,
            api_key_masked=mask_key(c.api_key),
            api_base=c.api_base,
            is_active=c.is_active,
            created_at=c.created_at,
        )
        for c in configs
    ]


@router.post("/config", response_model=AIConfigOut)
async def create_ai_config(
    data: AIConfigCreate,
    db: AsyncSession = Depends(get_db),
    _=Depends(require_admin),
):
    """创建 AI 配置（同时设为激活，其他配置自动停用）"""
    # 停用其他配置
    existing = await db.execute(select(AIConfig).where(AIConfig.is_active == True))
    for cfg in existing.scalars().all():
        cfg.is_active = False

    config = AIConfig(
        provider=data.provider,
        model_name=data.model_name,
        api_key=data.api_key,
        api_base=data.api_base,
        is_active=True,
    )
    db.add(config)
    await db.flush()
    await db.refresh(config)

    return AIConfigOut(
        id=config.id,
        provider=config.provider,
        model_name=config.model_name,
        api_key_masked=mask_key(config.api_key),
        api_base=config.api_base,
        is_active=config.is_active,
        created_at=config.created_at,
    )


@router.put("/config/{config_id}/activate")
async def activate_ai_config(
    config_id: int,
    db: AsyncSession = Depends(get_db),
    _=Depends(require_admin),
):
    """激活指定配置"""
    result = await db.execute(select(AIConfig).where(AIConfig.id == config_id))
    config = result.scalar_one_or_none()
    if not config:
        raise HTTPException(status_code=404, detail="配置不存在")

    # 停用其他
    all_configs = await db.execute(select(AIConfig).where(AIConfig.is_active == True))
    for cfg in all_configs.scalars().all():
        cfg.is_active = False

    config.is_active = True
    await db.flush()
    return ApiResponse(message="已激活")


@router.delete("/config/{config_id}")
async def delete_ai_config(
    config_id: int,
    db: AsyncSession = Depends(get_db),
    _=Depends(require_admin),
):
    """删除 AI 配置"""
    result = await db.execute(select(AIConfig).where(AIConfig.id == config_id))
    config = result.scalar_one_or_none()
    if not config:
        raise HTTPException(status_code=404, detail="配置不存在")
    await db.delete(config)
    await db.flush()
    return ApiResponse(message="已删除")


# ==================== 报告生成（管理员触发） ====================

@router.post("/reports/generate/{student_id}", response_model=AIReportOut)
async def generate_report_for_student(
    student_id: str,
    week_start: date | None = None,
    week_end: date | None = None,
    db: AsyncSession = Depends(get_db),
    _=Depends(require_admin),
):
    """为指定学生生成周报（手动触发）"""
    # 查学生
    result = await db.execute(select(Student).where(Student.student_id == student_id))
    student = result.scalar_one_or_none()
    if not student:
        raise HTTPException(status_code=404, detail="学生不存在")

    # 默认本周
    if not week_start or not week_end:
        week_start, week_end = get_current_week_range()

    try:
        report = await generate_weekly_report(db, student, week_start, week_end)
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"AI 生成失败: {str(e)}")

    return AIReportOut(
        id=report.id,
        student_id=report.student_id,
        student_name=student.name,
        week_start=report.week_start,
        week_end=report.week_end,
        report_summary=report.report_summary,
        provider=report.provider,
        model_name=report.model_name,
        created_at=report.created_at,
    )


@router.post("/reports/generate-batch", response_model=ApiResponse)
async def generate_reports_batch(
    week_start: date | None = None,
    week_end: date | None = None,
    db: AsyncSession = Depends(get_db),
    _=Depends(require_admin),
):
    """批量为所有学生生成周报"""
    if not week_start or not week_end:
        week_start, week_end = get_current_week_range()

    config = await get_active_config(db)
    if not config:
        raise HTTPException(status_code=400, detail="未配置 AI 模型")

    # 查所有学生
    result = await db.execute(select(Student))
    students = result.scalars().all()

    success = 0
    failed = 0
    errors = []

    for student in students:
        try:
            # 检查是否已有本周报告
            existing = await db.execute(
                select(AIReport).where(
                    AIReport.student_id == student.student_id,
                    AIReport.week_start == week_start,
                )
            )
            if existing.scalar_one_or_none():
                continue  # 跳过已有报告的学生

            await generate_weekly_report(db, student, week_start, week_end, config)
            success += 1
        except Exception as e:
            failed += 1
            errors.append(f"{student.student_id}: {str(e)}")

    return ApiResponse(
        success=failed == 0,
        message=f"生成完成: 成功{success}人, 失败{failed}人",
        data={"success": success, "failed": failed, "errors": errors[:10]},
    )


# ==================== 报告查看（管理端 + 手表端） ====================

@router.get("/reports", response_model=list[AIReportOut])
async def list_reports(
    student_id: str | None = None,
    page: int = Query(1, ge=1),
    page_size: int = Query(20, ge=1, le=100),
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    """获取报告列表（教师可看自己班级学生，管理员看全部）"""
    query = select(AIReport)

    if student_id:
        query = query.where(AIReport.student_id == student_id)

    # 教师权限过滤
    if user.role != "admin":
        tc = await db.execute(
            select(TeacherClass.class_id).where(TeacherClass.teacher_id == user.id)
        )
        class_ids = [row[0] for row in tc.all()]
        if class_ids:
            stu_ids = await db.execute(
                select(Student.student_id).where(Student.class_id.in_(class_ids))
            )
            allowed_ids = [row[0] for row in stu_ids.all()]
            if student_id and student_id not in allowed_ids:
                return []
            query = query.where(AIReport.student_id.in_(allowed_ids))
        else:
            return []

    query = query.order_by(AIReport.created_at.desc())
    query = query.offset((page - 1) * page_size).limit(page_size)
    result = await db.execute(query)
    reports = result.scalars().all()

    # 获取学生姓名
    out = []
    for r in reports:
        stu = await db.execute(select(Student).where(Student.student_id == r.student_id))
        student = stu.scalar_one_or_none()
        out.append(AIReportOut(
            id=r.id,
            student_id=r.student_id,
            student_name=student.name if student else None,
            week_start=r.week_start,
            week_end=r.week_end,
            report_summary=r.report_summary,
            provider=r.provider,
            model_name=r.model_name,
            created_at=r.created_at,
        ))
    return out


@router.get("/reports/{report_id}", response_model=AIReportDetail)
async def get_report_detail(
    report_id: int,
    db: AsyncSession = Depends(get_db),
    _=Depends(get_current_user),
):
    """获取报告详情（含完整 HTML）"""
    result = await db.execute(select(AIReport).where(AIReport.id == report_id))
    report = result.scalar_one_or_none()
    if not report:
        raise HTTPException(status_code=404, detail="报告不存在")

    stu = await db.execute(select(Student).where(Student.student_id == report.student_id))
    student = stu.scalar_one_or_none()

    return AIReportDetail(
        id=report.id,
        student_id=report.student_id,
        student_name=student.name if student else None,
        week_start=report.week_start,
        week_end=report.week_end,
        report_html=report.report_html,
        report_summary=report.report_summary,
        provider=report.provider,
        model_name=report.model_name,
        created_at=report.created_at,
    )


# ==================== 手表端专用接口（无需认证） ====================

@router.get("/watch/report/latest")
async def watch_latest_report(
    student_id: str,
    db: AsyncSession = Depends(get_db),
):
    """手表端获取最新一份周报（无需认证，用 student_id 查询）"""
    result = await db.execute(
        select(AIReport)
        .where(AIReport.student_id == student_id)
        .order_by(AIReport.created_at.desc())
        .limit(1)
    )
    report = result.scalar_one_or_none()
    if not report:
        return {"has_report": False, "message": "暂无分析报告"}

    return {
        "has_report": True,
        "id": report.id,
        "week_start": report.week_start.isoformat(),
        "week_end": report.week_end.isoformat(),
        "summary": report.report_summary,
        "created_at": report.created_at.isoformat(),
    }


@router.get("/watch/report/{report_id}")
async def watch_report_detail(
    report_id: int,
    db: AsyncSession = Depends(get_db),
):
    """手表端获取报告详情（无需认证）"""
    result = await db.execute(select(AIReport).where(AIReport.id == report_id))
    report = result.scalar_one_or_none()
    if not report:
        raise HTTPException(status_code=404, detail="报告不存在")

    return {
        "id": report.id,
        "student_id": report.student_id,
        "week_start": report.week_start.isoformat(),
        "week_end": report.week_end.isoformat(),
        "report_html": report.report_html,
        "summary": report.report_summary,
        "provider": report.provider,
        "model_name": report.model_name,
        "created_at": report.created_at.isoformat(),
    }


@router.get("/watch/reports")
async def watch_report_list(
    student_id: str,
    limit: int = Query(5, ge=1, le=20),
    db: AsyncSession = Depends(get_db),
):
    """手表端获取报告列表（最近 N 份，无需认证）"""
    result = await db.execute(
        select(AIReport)
        .where(AIReport.student_id == student_id)
        .order_by(AIReport.created_at.desc())
        .limit(limit)
    )
    reports = result.scalars().all()

    return [
        {
            "id": r.id,
            "week_start": r.week_start.isoformat(),
            "week_end": r.week_end.isoformat(),
            "summary": r.report_summary,
            "created_at": r.created_at.isoformat(),
        }
        for r in reports
    ]
