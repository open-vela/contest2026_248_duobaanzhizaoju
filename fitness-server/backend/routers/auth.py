"""认证路由"""
from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select

from database import get_db
from models import User
from schemas import LoginRequest, Token
from auth import verify_password, create_access_token, hash_password, get_current_user
from config import settings

router = APIRouter(prefix="/api/v1/auth", tags=["认证"])


@router.post("/login", response_model=Token)
async def login(req: LoginRequest, db: AsyncSession = Depends(get_db)):
    result = await db.execute(select(User).where(User.username == req.username))
    user = result.scalar_one_or_none()
    if not user or not verify_password(req.password, user.password_hash):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="用户名或密码错误",
        )
    if not user.is_active:
        raise HTTPException(status_code=403, detail="账号已被禁用")

    token = create_access_token({"sub": user.username, "role": user.role})
    return Token(access_token=token, role=user.role, real_name=user.real_name)


@router.post("/register")
async def register(
    username: str,
    password: str,
    real_name: str,
    role: str = "teacher",
    db: AsyncSession = Depends(get_db),
    _=Depends(get_current_user),
):
    """注册新用户（需要已登录）"""
    existing = await db.execute(select(User).where(User.username == username))
    if existing.scalar_one_or_none():
        raise HTTPException(status_code=400, detail="用户名已存在")

    user = User(
        username=username,
        password_hash=hash_password(password),
        real_name=real_name,
        role=role,
    )
    db.add(user)
    await db.flush()
    return {"id": user.id, "username": user.username}


@router.get("/me")
async def get_me(user: User = Depends(get_current_user)):
    return {
        "id": user.id,
        "username": user.username,
        "real_name": user.real_name,
        "role": user.role,
    }
