#!/bin/bash
# ============================================================
#  中学生体测数据管理平台 — Ubuntu 一键部署脚本
#  端口: 9000 (API) / 18083 (MQTT Dashboard)
#  使用: scp 到服务器后 chmod +x deploy-ubuntu.sh && sudo ./deploy-ubuntu.sh
# ============================================================

set -e

# 颜色
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PROJECT_DIR="/opt/fitness-server"
API_PORT=9000
DB_PORT=5433
REDIS_PORT=6380
MQTT_PORT=1884
MQTT_WS_PORT=8084
MQTT_DASH_PORT=18084

echo -e "${BLUE}============================================${NC}"
echo -e "${BLUE}  中学生体测数据管理平台 — Ubuntu 一键部署${NC}"
echo -e "${BLUE}  API 端口: ${API_PORT}${NC}"
echo -e "${BLUE}============================================${NC}"
echo ""

# ============ 1. 检查 root 权限 ============
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}❌ 请使用 sudo 运行此脚本${NC}"
    echo "   sudo ./deploy-ubuntu.sh"
    exit 1
fi

# ============ 2. 安装 Docker ============
echo -e "${YELLOW}[1/7] 检查 Docker...${NC}"
if ! command -v docker &> /dev/null; then
    echo "  安装 Docker..."
    apt-get update -qq
    apt-get install -y -qq ca-certificates curl gnupg lsb-release

    # 添加 Docker 官方 GPG key
    install -m 0755 -d /etc/apt/keyrings
    curl -fsSL https://download.docker.com/linux/ubuntu/gpg | gpg --dearmor -o /etc/apt/keyrings/docker.gpg
    chmod a+r /etc/apt/keyrings/docker.gpg

    echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" > /etc/apt/sources.list.d/docker.list

    apt-get update -qq
    apt-get install -y -qq docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

    systemctl enable docker
    systemctl start docker
    echo -e "${GREEN}  ✅ Docker 安装完成${NC}"
else
    echo -e "${GREEN}  ✅ Docker 已安装: $(docker --version)${NC}"
fi

# ============ 3. 创建项目目录并复制文件 ============
echo -e "${YELLOW}[2/7] 部署项目文件到 ${PROJECT_DIR}...${NC}"

# 备份旧版本（如果存在）
if [ -d "$PROJECT_DIR" ]; then
    BACKUP_DIR="${PROJECT_DIR}.bak.$(date +%Y%m%d%H%M%S)"
    echo "  备份旧版本到 ${BACKUP_DIR}"
    cp -r "$PROJECT_DIR" "$BACKUP_DIR"
fi

mkdir -p "$PROJECT_DIR"

# 获取脚本所在目录（源文件位置）
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# 复制项目文件（排除 node_modules, __pycache__, .git）
rsync -av --exclude='node_modules' --exclude='__pycache__' --exclude='.git' \
    --exclude='*.bak.*' --exclude='fitness.db' \
    "$SCRIPT_DIR/" "$PROJECT_DIR/"

echo -e "${GREEN}  ✅ 文件同步完成${NC}"

# ============ 4. 生成 .env 配置 ============
echo -e "${YELLOW}[3/7] 生成环境配置...${NC}"

SECRET_KEY=$(openssl rand -hex 32 2>/dev/null || head -c 32 /dev/urandom | base64 | tr -d '/+=' | head -c 32)

cat > "$PROJECT_DIR/.env" << EOF
# ===== 体测平台环境配置 =====
SECRET_KEY=${SECRET_KEY}

# 数据库 (PostgreSQL in Docker, 映射到宿主机 ${DB_PORT} 避免冲突)
POSTGRES_HOST=db
POSTGRES_PORT=5432
POSTGRES_USER=fitness
POSTGRES_PASSWORD=fitness$(openssl rand -hex 4 2>/dev/null || echo "pass")
POSTGRES_DB=fitness_db

# Redis (Docker 内部端口)
REDIS_HOST=redis
REDIS_PORT=6379

# MQTT (Docker 内部端口)
MQTT_HOST=emqx
MQTT_PORT=1883
EOF

echo -e "${GREEN}  ✅ .env 已生成 (密码已随机化)${NC}"

# ============ 5. 更新 docker-compose.yml 端口映射 ============
echo -e "${YELLOW}[4/7] 配置端口映射 (API:${API_PORT})...${NC}"

cat > "$PROJECT_DIR/docker-compose.yml" << 'COMPOSEOF'
version: "3.8"

services:
  # PostgreSQL 数据库
  db:
    image: postgres:16-alpine
    environment:
      POSTGRES_USER: ${POSTGRES_USER}
      POSTGRES_PASSWORD: ${POSTGRES_PASSWORD}
      POSTGRES_DB: ${POSTGRES_DB}
    ports:
      - "DB_PORT_PLACEHOLDER:5432"
    volumes:
      - pgdata:/var/lib/postgresql/data
      - ./init-db.sql:/docker-entrypoint-initdb.d/init.sql
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U ${POSTGRES_USER} -d ${POSTGRES_DB}"]
      interval: 5s
      timeout: 5s
      retries: 5
    restart: unless-stopped

  # Redis 缓存
  redis:
    image: redis:7-alpine
    ports:
      - "REDIS_PORT_PLACEHOLDER:6379"
    restart: unless-stopped

  # MQTT Broker (EMQX)
  emqx:
    image: emqx/emqx:5.4
    ports:
      - "MQTT_PORT_PLACEHOLDER:1883"
      - "MQTT_WS_PORT_PLACEHOLDER:8083"
      - "MQTT_DASH_PORT_PLACEHOLDER:18083"
    environment:
      EMQX_NAME: fitness-emqx
      EMQX_HOST: 127.0.0.1
    restart: unless-stopped

  # 后端 API
  server:
    build: .
    ports:
      - "API_PORT_PLACEHOLDER:8000"
    environment:
      POSTGRES_HOST: db
      POSTGRES_PORT: "5432"
      POSTGRES_USER: ${POSTGRES_USER}
      POSTGRES_PASSWORD: ${POSTGRES_PASSWORD}
      POSTGRES_DB: ${POSTGRES_DB}
      REDIS_HOST: redis
      MQTT_HOST: emqx
      MQTT_PORT: "1883"
    depends_on:
      db:
        condition: service_healthy
      redis:
        condition: service_started
      emqx:
        condition: service_started
    volumes:
      - ./backend:/app/backend
      - ./frontend:/app/frontend
    restart: unless-stopped

volumes:
  pgdata:
COMPOSEOF

# 替换占位符为实际端口
sed -i "s/API_PORT_PLACEHOLDER/${API_PORT}/g" "$PROJECT_DIR/docker-compose.yml"
sed -i "s/DB_PORT_PLACEHOLDER/${DB_PORT}/g" "$PROJECT_DIR/docker-compose.yml"
sed -i "s/REDIS_PORT_PLACEHOLDER/${REDIS_PORT}/g" "$PROJECT_DIR/docker-compose.yml"
sed -i "s/MQTT_PORT_PLACEHOLDER/${MQTT_PORT}/g" "$PROJECT_DIR/docker-compose.yml"
sed -i "s/MQTT_WS_PORT_PLACEHOLDER/${MQTT_WS_PORT}/g" "$PROJECT_DIR/docker-compose.yml"
sed -i "s/MQTT_DASH_PORT_PLACEHOLDER/${MQTT_DASH_PORT}/g" "$PROJECT_DIR/docker-compose.yml"

echo -e "${GREEN}  ✅ 端口配置完成${NC}"

# ============ 6. 构建并启动 ============
echo -e "${YELLOW}[5/7] 构建 Docker 镜像 (首次可能需要 3-5 分钟)...${NC}"
cd "$PROJECT_DIR"
docker compose build --no-cache

echo -e "${YELLOW}[6/7] 启动所有服务...${NC}"
docker compose up -d

echo ""
echo -e "${YELLOW}  等待服务就绪...${NC}"
for i in $(seq 1 30); do
    if curl -sf http://localhost:${API_PORT}/health > /dev/null 2>&1; then
        echo -e "${GREEN}  ✅ API 服务就绪!${NC}"
        break
    fi
    sleep 2
    echo -n "."
done
echo ""

# ============ 7. 初始化数据库 ============
echo -e "${YELLOW}[7/7] 初始化数据库...${NC}"
docker compose exec -T server python init_db.py 2>/dev/null || echo "  (数据库已初始化或将在首次请求时自动创建表)"

# ============ 配置防火墙 ============
echo -e "${YELLOW}[附加] 配置 UFW 防火墙...${NC}"
if command -v ufw &> /dev/null; then
    ufw allow ${API_PORT}/tcp comment "体测平台 API" 2>/dev/null || true
    ufw allow ${MQTT_DASH_PORT}/tcp comment "MQTT Dashboard" 2>/dev/null || true
    echo -e "${GREEN}  ✅ 防火墙规则已添加${NC}"
else
    echo -e "${YELLOW}  ⚠️  ufw 未安装，跳过防火墙配置${NC}"
fi

# ============ 完成 ============
LOCAL_IP=$(hostname -I 2>/dev/null | awk '{print $1}' || echo "YOUR_SERVER_IP")

echo ""
echo -e "${GREEN}============================================${NC}"
echo -e "${GREEN}  ✅ 部署完成!${NC}"
echo -e "${GREEN}============================================${NC}"
echo ""
echo -e "  📡 API 地址:     http://${LOCAL_IP}:${API_PORT}"
echo -e "  📖 API 文档:     http://${LOCAL_IP}:${API_PORT}/docs"
echo -e "  🖥️  管理平台:    http://${LOCAL_IP}:${API_PORT}"
echo -e "  📊 MQTT 控制台:  http://${LOCAL_IP}:${MQTT_DASH_PORT}"
echo -e "     (默认账号: admin / public)"
echo ""
echo -e "  ${YELLOW}端口映射:${NC}"
echo -e "    API:    宿主机 ${API_PORT}  → 容器 8000"
echo -e "    DB:     宿主机 ${DB_PORT}   → 容器 5432"
echo -e "    Redis:  宿主机 ${REDIS_PORT}   → 容器 6379"
echo -e "    MQTT:   宿主机 ${MQTT_PORT}   → 容器 1883"
echo -e "    WS:     宿主机 ${MQTT_WS_PORT}   → 容器 8083"
echo -e "    Dash:   宿主机 ${MQTT_DASH_PORT} → 容器 18083"
echo ""
echo -e "  ${YELLOW}默认账号:${NC}"
echo -e "    管理员:  admin / admin123"
echo -e "    教师:    teacher / teacher123"
echo ""
echo -e "  ${YELLOW}常用命令:${NC}"
echo -e "    查看日志:   cd ${PROJECT_DIR} && docker compose logs -f server"
echo -e "    停止服务:   cd ${PROJECT_DIR} && docker compose down"
echo -e "    重启服务:   cd ${PROJECT_DIR} && docker compose restart"
echo -e "    查看状态:   cd ${PROJECT_DIR} && docker compose ps"
echo -e "    更新部署:   sudo ./deploy-ubuntu.sh"
echo ""
echo -e "  ${YELLOW}⚠️  手表端配置:${NC}"
echo -e "    在手表设置页将服务器地址改为:"
echo -e "    http://${LOCAL_IP}:${API_PORT}"
echo ""
echo -e "${GREEN}============================================${NC}"
