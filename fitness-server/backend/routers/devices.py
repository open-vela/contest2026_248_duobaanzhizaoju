"""设备管理路由"""
from datetime import datetime, timezone
from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select

from database import get_db
from models import Device
from schemas import DeviceRegister, DeviceOut, ApiResponse
from auth import get_current_user

router = APIRouter(prefix="/api/v1/devices", tags=["设备管理"])


@router.get("", response_model=list[DeviceOut])
async def list_devices(
    active_only: bool = False,
    db: AsyncSession = Depends(get_db),
    _=Depends(get_current_user),
):
    query = select(Device)
    if active_only:
        query = query.where(Device.is_active == True)
    result = await db.execute(query)
    return result.scalars().all()


@router.post("/register", response_model=DeviceOut)
async def register_device(
    data: DeviceRegister,
    db: AsyncSession = Depends(get_db),
):
    """设备注册（无需认证，手表端直接调用）"""
    existing = await db.execute(select(Device).where(Device.device_id == data.device_id))
    device = existing.scalar_one_or_none()
    if device:
        device.student_id = data.student_id or device.student_id
        device.device_name = data.device_name or device.device_name
        device.is_active = True
    else:
        device = Device(**data.model_dump())
        db.add(device)
    await db.flush()
    await db.refresh(device)
    return device


@router.put("/{device_id}/unbind")
async def unbind_device(
    device_id: str,
    db: AsyncSession = Depends(get_db),
    _=Depends(get_current_user),
):
    result = await db.execute(select(Device).where(Device.device_id == device_id))
    device = result.scalar_one_or_none()
    if not device:
        raise HTTPException(status_code=404, detail="设备不存在")
    device.student_id = None
    await db.flush()
    return ApiResponse(message="已解绑")


@router.post("/{device_id}/heartbeat")
async def device_heartbeat(
    device_id: str,
    battery: int | None = None,
    db: AsyncSession = Depends(get_db),
):
    """设备心跳上报（无需认证，设备直接调用）"""
    result = await db.execute(select(Device).where(Device.device_id == device_id))
    device = result.scalar_one_or_none()
    if not device:
        raise HTTPException(status_code=404, detail="设备未注册")
    device.last_online = datetime.now(timezone.utc)
    if battery is not None:
        device.battery_level = battery
    await db.flush()
    return {"status": "ok"}
