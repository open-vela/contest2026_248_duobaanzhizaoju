"""学生管理路由"""
from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select, func, delete

from database import get_db
from models import Student, Class, School, TeacherClass
from schemas import StudentCreate, StudentOut, ApiResponse
from auth import get_current_user

router = APIRouter(prefix="/api/v1/students", tags=["学生管理"])


async def _get_teacher_class_ids(user, db) -> list[int] | None:
    """获取教师的绑定班级ID列表，admin返回None表示不限"""
    if user.role == "admin":
        return None
    result = await db.execute(
        select(TeacherClass.class_id).where(TeacherClass.teacher_id == user.id)
    )
    return [row[0] for row in result.all()]


@router.get("", response_model=list[StudentOut])
async def list_students(
    grade: str | None = None,
    class_id: int | None = None,
    keyword: str | None = None,
    page: int = Query(1, ge=1),
    page_size: int = Query(50, ge=1, le=200),
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    teacher_class_ids = await _get_teacher_class_ids(user, db)
    query = select(Student)

    # 教师只能看到自己绑定班级的学生
    if teacher_class_ids is not None:
        query = query.where(Student.class_id.in_(teacher_class_ids))

    if grade:
        query = query.where(Student.grade == grade)
    if class_id:
        # 教师如果请求了不在自己绑定范围内的班级，返回空
        if teacher_class_ids is not None and class_id not in teacher_class_ids:
            return []
        query = query.where(Student.class_id == class_id)
    if keyword:
        query = query.where(
            (Student.name.contains(keyword)) | (Student.student_id.contains(keyword))
        )
    query = query.offset((page - 1) * page_size).limit(page_size)
    result = await db.execute(query)
    return result.scalars().all()


@router.post("", response_model=StudentOut)
async def create_student(
    data: StudentCreate,
    db: AsyncSession = Depends(get_db),
    _=Depends(get_current_user),
):
    existing = await db.execute(select(Student).where(Student.student_id == data.student_id))
    if existing.scalar_one_or_none():
        raise HTTPException(status_code=400, detail="学号已存在")

    student = Student(**data.model_dump())
    db.add(student)
    await db.flush()
    await db.refresh(student)
    return student


@router.put("/{student_id}", response_model=StudentOut)
async def update_student(
    student_id: str,
    data: StudentCreate,
    db: AsyncSession = Depends(get_db),
    _=Depends(get_current_user),
):
    result = await db.execute(select(Student).where(Student.student_id == student_id))
    student = result.scalar_one_or_none()
    if not student:
        raise HTTPException(status_code=404, detail="学生不存在")

    for k, v in data.model_dump(exclude_unset=True).items():
        setattr(student, k, v)
    await db.flush()
    await db.refresh(student)
    return student


@router.delete("/{student_id}")
async def delete_student(
    student_id: str,
    db: AsyncSession = Depends(get_db),
    _=Depends(get_current_user),
):
    result = await db.execute(select(Student).where(Student.student_id == student_id))
    student = result.scalar_one_or_none()
    if not student:
        raise HTTPException(status_code=404, detail="学生不存在")
    await db.delete(student)
    return ApiResponse(message="已删除")


@router.post("/batch")
async def batch_import_students(
    students: list[StudentCreate],
    db: AsyncSession = Depends(get_db),
    _=Depends(get_current_user),
):
    """批量导入学生"""
    created = 0
    skipped = 0
    for s in students:
        existing = await db.execute(select(Student).where(Student.student_id == s.student_id))
        if existing.scalar_one_or_none():
            skipped += 1
            continue
        db.add(Student(**s.model_dump()))
        created += 1
    await db.flush()
    return {"created": created, "skipped": skipped}
