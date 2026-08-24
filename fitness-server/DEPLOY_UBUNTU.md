# Ubuntu 部署指南

## 一键部署

### 方式一：直接在服务器上运行

```bash
# 1. 将项目上传到服务器
scp -r fitness-server/ user@your-server:/tmp/fitness-server

# 2. SSH 登录服务器
ssh user@your-server

# 3. 运行一键部署脚本
cd /tmp/fitness-server
sudo chmod +x deploy-ubuntu.sh
sudo ./deploy-ubuntu.sh
```

### 方式二：Git 同步部署

```bash
# 1. 在服务器上克隆/拉取代码
cd /opt
git clone <your-repo> fitness-server
cd fitness-server

# 2. 运行部署
sudo chmod +x deploy-ubuntu.sh
sudo ./deploy-ubuntu.sh
```

## 部署后访问

| 服务 | 地址 | 说明 |
|------|------|------|
| API 服务 | `http://YOUR_IP:9000` | 主服务（管理平台 + API） |
| API 文档 | `http://YOUR_IP:9000/docs` | Swagger 交互式文档 |
| MQTT 控制台 | `http://YOUR_IP:18084` | EMQX 管理面板 |

## 端口映射

| 服务 | 宿主机端口 | 容器端口 | 说明 |
|------|-----------|---------|------|
| API | **9000** | 8000 | FastAPI 服务 |
| PostgreSQL | 5433 | 5432 | 数据库 |
| Redis | 6380 | 6379 | 缓存 |
| MQTT | 1884 | 1883 | 消息队列 |
| MQTT WebSocket | 8084 | 8083 | WebSocket |
| MQTT Dashboard | 18084 | 18083 | EMQX 管理 |

> 端口设计原则：宿主机端口避开常见冲突（8000/5432/6379/1883），容器内部保持默认端口不变。

## 手表端配置

部署完成后，需要在手表应用的设置页修改服务器地址：

```
http://YOUR_SERVER_IP:9000
```

对应代码位置：`Xsport/src/pages/settings/settings.ux` 中的服务器地址输入框。

## 常用运维命令

```bash
cd /opt/fitness-server

# 查看服务状态
docker compose ps

# 查看实时日志
docker compose logs -f server

# 重启所有服务
docker compose restart

# 重启单个服务
docker compose restart server

# 停止所有服务
docker compose down

# 重新构建并启动（代码更新后）
docker compose up -d --build

# 进入容器调试
docker compose exec server bash

# 查看数据库
docker compose exec db psql -U fitness -d fitness_db
```

## 环境变量

部署脚本会自动生成 `.env` 文件，包含随机密码。如需手动修改：

```bash
vi /opt/fitness-server/.env
docker compose restart
```

## 数据备份

```bash
# 备份 PostgreSQL
docker compose exec db pg_dump -U fitness fitness_db > backup_$(date +%Y%m%d).sql

# 恢复
cat backup_20260718.sql | docker compose exec -T db psql -U fitness -d fitness_db
```

## 卸载

```bash
cd /opt/fitness-server
docker compose down -v    # -v 删除数据卷
sudo rm -rf /opt/fitness-server
```
