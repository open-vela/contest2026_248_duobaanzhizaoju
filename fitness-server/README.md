# 中学生体测数据管理平台

基于立创黄山派（SF32LB52）的智能体测穿戴设备后端系统。

## 项目结构

```
fitness-server/
├── backend/
│   ├── main.py              # FastAPI 主入口
│   ├── config.py            # 配置管理
│   ├── database.py          # 数据库连接
│   ├── models.py            # 数据模型 (SQLAlchemy)
│   ├── schemas.py           # 请求/响应模型 (Pydantic)
│   ├── scoring.py           # 体测评分引擎
│   ├── auth.py              # JWT 认证
│   ├── mqtt_handler.py      # MQTT 设备数据处理
│   ├── init_db.py           # 数据库初始化（含示例数据）
│   ├── requirements.txt     # Python 依赖
│   └── routers/
│       ├── auth.py          # 认证路由
│       ├── students.py      # 学生管理
│       ├── devices.py       # 设备管理
│       ├── fitness.py       # 体测数据上传/查询
│       └── statistics.py    # 统计分析/排行榜
├── frontend/
│   ├── index.html           # Web 管理端
│   ├── css/style.css        # 样式
│   └── js/app.js            # 前端逻辑
├── Dockerfile               # Docker 镜像构建
├── docker-compose.yml       # 完整服务编排
├── deploy.sh                # Linux 部署脚本
├── deploy.ps1               # Windows 部署脚本
├── init-db.sql              # PostgreSQL 初始化
├── .env.example             # 环境变量模板
└── README.md
```

## 快速部署

### 方式一：Docker 部署（推荐）

```bash
cd fitness-server
cp .env.example .env
# Linux/Mac
chmod +x deploy.sh && ./deploy.sh
# Windows
.\deploy.ps1
```

### 方式二：手动部署

1. 安装 PostgreSQL 16+ 和 Python 3.11+
2. 创建数据库：
```sql
CREATE USER fitness WITH PASSWORD 'fitness123';
CREATE DATABASE fitness_db OWNER fitness;
```

3. 启动后端：
```bash
cd backend
pip install -r requirements.txt
python init_db.py        # 初始化数据库
uvicorn main:app --host 0.0.0.0 --port 8000
```

## 默认账号

| 角色 | 用户名 | 密码 |
|------|--------|------|
| 管理员 | admin | admin123 |
| 教师 | teacher | teacher123 |

## API 接口

### 认证
- `POST /api/v1/auth/login` — 登录
- `GET /api/v1/auth/me` — 当前用户信息

### 学生管理
- `GET /api/v1/students` — 学生列表（支持搜索/筛选）
- `POST /api/v1/students` — 添加学生
- `PUT /api/v1/students/{id}` — 更新学生
- `DELETE /api/v1/students/{id}` — 删除学生
- `POST /api/v1/students/batch` — 批量导入

### 体测数据
- `POST /api/v1/fitness/upload` — 设备单条上传
- `POST /api/v1/fitness/upload/batch` — 设备批量上传
- `GET /api/v1/fitness/records` — 记录查询
- `GET /api/v1/fitness/records/{student_id}` — 学生记录
- `POST /api/v1/fitness/records/manual` — 手动录入

### 设备管理
- `GET /api/v1/devices` — 设备列表
- `POST /api/v1/devices/register` — 注册设备
- `PUT /api/v1/devices/{id}/unbind` — 解绑设备
- `POST /api/v1/devices/{id}/heartbeat` — 设备心跳

### 统计分析
- `GET /api/v1/statistics/overview` — 总览
- `GET /api/v1/statistics/class/{id}` — 班级统计
- `GET /api/v1/statistics/trend/{student_id}` — 学生趋势
- `GET /api/v1/statistics/leaderboard` — 排行榜

## 设备数据上传格式

### HTTP 方式
```json
POST /api/v1/fitness/upload
{
    "student_id": "STU20260001",
    "device_id": "HS-2026-0001",
    "test_type": "jump_rope",
    "test_date": "2026-07-09T10:00:00+08:00",
    "count": 175,
    "duration_sec": 60,
    "avg_hr": 145,
    "max_hr": 168
}
```

### MQTT 方式
```
Topic: fitness/HS-2026-0001/data
Payload: {"student_id":"STU20260001","test_type":"jump_rope","count":175,...}
```

## 体测项目支持

| 项目 | test_type | 成绩字段 | 评分方式 |
|------|-----------|---------|---------|
| 跳绳 | jump_rope | count | ≥次数 |
| 50米跑 | 50m_run | time_sec | ≤秒数 |
| 800米跑 | 800m_run | time_sec | ≤秒数 |
| 1000米跑 | 1000m_run | time_sec | ≤秒数 |
| 立定跳远 | long_jump | distance_m | ≥距离 |
| 仰卧起坐 | sit_ups | count | ≥次数 |
| 引体向上 | pull_up | count | ≥次数 |
| 坐位体前屈 | sit_reach | distance_m | ≥距离 |

## 技术栈

- 后端: FastAPI + SQLAlchemy + PostgreSQL
- 前端: 原生HTML/JS + Chart.js
- 消息: MQTT (EMQX)
- 缓存: Redis
- 部署: Docker Compose
