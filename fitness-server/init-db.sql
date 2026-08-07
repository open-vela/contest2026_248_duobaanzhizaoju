-- 数据库初始化SQL（Docker启动时自动执行）
-- 仅创建扩展，表由应用ORM自动创建
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- 启用JSONB操作支持
CREATE EXTENSION IF NOT EXISTS "pgcrypto";
