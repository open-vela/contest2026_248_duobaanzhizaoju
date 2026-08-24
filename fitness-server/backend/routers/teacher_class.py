"""教师班级绑定路由"""
from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select, insert, delete

from database import get_db
from models import User, TeacherClass, Class, Student
from schemas import ApiResponse, TeacherBindRequest
from auth import get_current_user, require_admin

router = APIRouter(prefix="/api/v1/teacher-class", tags=["教师班级绑定"])


@router.get("/my-classes")
async def my_classes(
    db: AsyncSession = Depends(get_db),
    user: User = Depends(get_current_user),
):
    """获取当前教师绑定的班级列表"""
    if user.role == "admin":
        result = await db.execute(select(Class))
        classes = result.scalars().all()
    else:
        result = await db.execute(
            select(Class)
            .join(TeacherClass, TeacherClass.class_id == Class.id)
            .where(TeacherClass.teacher_id == user.id)
        )
        classes = result.scalars().all()

    return [
        {
            "id": c.id,
            "grade": c.grade,
            "class_name": c.class_name,
            "head_teacher": c.head_teacher,
        }
        for c in classes
    ]


@router.post("/bind")
async def bind_class(
    teacher_id: int,
    class_id: int,
    db: AsyncSession = Depends(get_db),
    user: User = Depends(get_current_user),
):
    """绑定教师与班级（管理员操作或教师自己绑定）"""
    if user.role != "admin" and user.id != teacher_id:
        raise HTTPException(status_code=403, detail="只能绑定自己或需要管理员权限")

    existing = await db.execute(
        select(TeacherClass).where(
            TeacherClass.teacher_id == teacher_id,
            TeacherClass.class_id == class_id,
        )
    )
    if existing.scalar_one_or_none():
        return ApiResponse(message="已绑定，无需重复操作")

    db.add(TeacherClass(teacher_id=teacher_id, class_id=class_id))
    await db.flush()
    return ApiResponse(success=True, message="绑定成功")


@router.delete("/unbind")
async def unbind_class(
    teacher_id: int,
    class_id: int,
    db: AsyncSession = Depends(get_db),
    user: User = Depends(get_current_user),
):
    """解绑教师与班级"""
    if user.role != "admin" and user.id != teacher_id:
        raise HTTPException(status_code=403, detail="只能解绑自己或需要管理员权限")

    await db.execute(
        delete(TeacherClass).where(
            TeacherClass.teacher_id == teacher_id,
            TeacherClass.class_id == class_id,
        )
    )
    await db.flush()
    return ApiResponse(success=True, message="解绑成功")


@router.get("/students")
async def my_students(
    class_id: int | None = None,
    db: AsyncSession = Depends(get_db),
    user: User = Depends(get_current_user),
):
    """获取当前教师班级下的学生"""
    if user.role == "admin":
        query = select(Student)
        if class_id:
            query = query.where(Student.class_id == class_id)
    else:
        subq = select(TeacherClass.class_id).where(TeacherClass.teacher_id == user.id)
        query = select(Student).where(Student.class_id.in_(subq))
        if class_id:
            query = query.where(Student.class_id == class_id)

    result = await db.execute(query)
    students = result.scalars().all()
    return [
        {
            "student_id": s.student_id,
            "name": s.name,
            "gender": s.gender,
            "grade": s.grade,
            "class_id": s.class_id,
            "height_cm": s.height_cm,
            "weight_kg": s.weight_kg,
        }
        for s in students
    ]


# ===== 管理员专用端点 =====

@router.get("/all-teachers")
async def all_teachers(
    db: AsyncSession = Depends(get_db),
    user: User = Depends(require_admin),
):
    """获取所有教师列表（管理员）"""
    result = await db.execute(
        select(User).where(User.role == "teacher", User.is_active == True)
    )
    teachers = result.scalars().all()
    return [
        {
            "id": t.id,
            "username": t.username,
            "real_name": t.real_name,
            "phone": t.phone,
        }
        for t in teachers
    ]


@router.get("/all-classes")
async def all_classes(
    db: AsyncSession = Depends(get_db),
    user: User = Depends(get_current_user),
):
    """获取所有班级列表（任何登录用户）"""
    if user.role == "admin":
        result = await db.execute(select(Class))
    else:
        # 教师只返回自己绑定的班级
        result = await db.execute(
            select(Class)
            .join(TeacherClass, TeacherClass.class_id == Class.id)
            .where(TeacherClass.teacher_id == user.id)
        )
    classes = result.scalars().all()
    return [
        {
            "id": c.id,
            "grade": c.grade,
            "class_name": c.class_name,
            "head_teacher": c.head_teacher,
        }
        for c in classes
    ]


@router.get("/bindings")
async def all_bindings(
    db: AsyncSession = Depends(get_db),
    user: User = Depends(require_admin),
):
    """获取所有教师-班级绑定关系（管理员）"""
    result = await db.execute(select(TeacherClass))
    bindings = result.scalars().all()

    binding_list = []
    for b in bindings:
        teacher_r = await db.execute(select(User).where(User.id == b.teacher_id))
        teacher = teacher_r.scalar_one_or_none()
        class_r = await db.execute(select(Class).where(Class.id == b.class_id))
        cls = class_r.scalar_one_or_none()

        binding_list.append({
            "id": b.id,
            "teacher_id": b.teacher_id,
            "teacher_name": teacher.real_name if teacher else "",
            "teacher_username": teacher.username if teacher else "",
            "class_id": b.class_id,
            "class_name": f"{cls.grade} {cls.class_name}" if cls else "",
        })

    return binding_list


@router.post("/admin-bind")
async def admin_bind(
    req: TeacherBindRequest,
    db: AsyncSession = Depends(get_db),
    user: User = Depends(require_admin),
):
    """管理员绑定教师与班级"""
    teacher_id = req.teacher_id
    class_id = req.class_id
    existing = await db.execute(
        select(TeacherClass).where(
            TeacherClass.teacher_id == teacher_id,
            TeacherClass.class_id == class_id,
        )
    )
    if existing.scalar_one_or_none():
        return ApiResponse(message="已绑定，无需重复操作")

    db.add(TeacherClass(teacher_id=teacher_id, class_id=class_id))
    await db.flush()
    return ApiResponse(success=True, message="绑定成功")


@router.delete("/admin-unbind")
async def admin_unbind(
    req: TeacherBindRequest,
    db: AsyncSession = Depends(get_db),
    user: User = Depends(require_admin),
):
    """管理员解绑教师与班级"""
    teacher_id = req.teacher_id
    class_id = req.class_id
    await db.execute(
        delete(TeacherClass).where(
            TeacherClass.teacher_id == teacher_id,
            TeacherClass.class_id == class_id,
        )
    )
    await db.flush()
    return ApiResponse(success=True, message="解绑成功")
