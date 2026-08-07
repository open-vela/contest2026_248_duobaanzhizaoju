"""FastAPI 应用主入口"""
import logging
from contextlib import asynccontextmanager

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse

from config import settings
from database import init_db
from routers import auth, students, devices, fitness, statistics, teacher_class, ai_report
from mqtt_handler import mqtt_handler

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(name)s] %(levelname)s: %(message)s")
logger = logging.getLogger(__name__)


@asynccontextmanager
async def lifespan(app: FastAPI):
    """应用生命周期"""
    # 启动
    logger.info("Initializing database...")
    await init_db()
    logger.info("Database ready")

    # 启动内嵌 MQTT Broker + 订阅客户端
    logger.info("Starting MQTT Broker...")
    await mqtt_handler.start()

    yield

    # 关闭
    await mqtt_handler.stop()
    logger.info("Server stopped")


app = FastAPI(
    title=settings.APP_NAME,
    version=settings.APP_VERSION,
    lifespan=lifespan,
)

# CORS
app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.CORS_ORIGINS,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# 注册路由
app.include_router(auth.router)
app.include_router(students.router)
app.include_router(devices.router)
app.include_router(fitness.router)
app.include_router(statistics.router)
app.include_router(teacher_class.router)
app.include_router(ai_report.router)


# 静态文件（前端）
import os
frontend_dir = os.path.join(os.path.dirname(__file__), "..", "frontend")
if os.path.isdir(frontend_dir):
    app.mount("/static", StaticFiles(directory=frontend_dir), name="static")


@app.get("/")
async def index():
    """返回前端首页"""
    index_path = os.path.join(frontend_dir, "index.html")
    if os.path.isfile(index_path):
        return FileResponse(index_path)
    return {"message": "Fitness Test Platform API", "docs": "/docs"}


@app.get("/health")
async def health():
    return {"status": "ok", "mqtt": mqtt_handler.get_stats()}


@app.get("/api/v1/mqtt/status")
async def mqtt_status():
    """MQTT Broker 状态"""
    return mqtt_handler.get_stats()
