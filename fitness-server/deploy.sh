#!/bin/bash
# 部署脚本 — 中学生体测数据管理平台
# 使用: chmod +x deploy.sh && ./deploy.sh

set -e

echo "============================================"
echo "  中学生体测数据管理平台 — 部署脚本"
echo "============================================"

# 检查Docker
if ! command -v docker &> /dev/null; then
    echo "❌ Docker 未安装，请先安装 Docker"
    exit 1
fi

if ! command -v docker-compose &> /dev/null && ! docker compose version &> /dev/null; then
    echo "❌ Docker Compose 未安装，请先安装 Docker Compose"
    exit 1
fi

# 确定compose命令
COMPOSE_CMD="docker-compose"
if ! command -v docker-compose &> /dev/null; then
    COMPOSE_CMD="docker compose"
fi

# 检查.env
if [ ! -f .env ]; then
    echo "📝 创建 .env 配置文件..."
    cp .env.example .env
    echo "⚠️  请编辑 .env 修改密码等配置"
fi

echo ""
echo "📦 构建并启动服务..."
$COMPOSE_CMD up -d --build

echo ""
echo "⏳ 等待数据库就绪..."
sleep 10

echo ""
echo "🔧 初始化数据库（创建表 + 示例数据）..."
$COMPOSE_CMD exec -T server python init_db.py

echo ""
echo "============================================"
echo "✅ 部署完成!"
echo "============================================"
echo ""
echo "  📡 API地址:    http://localhost:8000"
echo "  📖 API文档:    http://localhost:8000/docs"
echo "  🖥️ 管理平台:   http://localhost:8000"
echo "  📊 MQTT Dashboard: http://localhost:18083"
echo ""
echo "  默认账号:"
echo "    管理员: admin / admin123"
echo "    教师:   teacher / teacher123"
echo ""
echo "  常用命令:"
echo "    查看日志: $COMPOSE_CMD logs -f server"
echo "    停止服务: $COMPOSE_CMD down"
echo "    重启服务: $COMPOSE_CMD restart"
echo "============================================"
