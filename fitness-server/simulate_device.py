"""
设备端模拟脚本 — 模拟黄山派上传体测数据
用于测试服务器端接口

使用: python simulate_device.py
"""
import httpx
import random
import asyncio
from datetime import datetime, timezone, timedelta

SERVER_URL = "http://localhost:8000"

# 测试学生
STUDENTS = [
    ("STU20260001", "M"),
    ("STU20260002", "M"),
    ("STU20260003", "F"),
    ("STU20260004", "M"),
    ("STU20260005", "F"),
]

# 体测项目模板
TESTS = [
    {"test_type": "jump_rope", "count_range": (120, 185), "time_range": (55, 65)},
    {"test_type": "50m_run", "time_range": (7.0, 8.5)},
    {"test_type": "long_jump", "dist_range": (1.7, 2.5)},
    {"test_type": "sit_ups", "count_range": (35, 55), "time_range": (55, 65)},
    {"test_type": "1000m_run", "time_range": (220, 300)},
    {"test_type": "800m_run", "time_range": (210, 280)},
]


async def upload_record(student_id: str, test: dict):
    """上传一条体测记录"""
    test_type = test["test_type"]
    now = datetime.now(timezone(timedelta(hours=8)))

    body = {
        "student_id": student_id,
        "device_id": f"HS-2026-0001",
        "test_type": test_type,
        "test_date": now.isoformat(),
        "avg_hr": random.randint(120, 150),
        "max_hr": random.randint(150, 180),
    }

    if "count_range" in test:
        body["count"] = random.randint(*test["count_range"])
        body["duration_sec"] = random.randint(*test["time_range"])
    elif "time_range" in test and test_type in ("50m_run", "800m_run", "1000m_run"):
        body["time_sec"] = round(random.uniform(*test["time_range"]), 2)
    elif "dist_range" in test:
        body["distance_m"] = round(random.uniform(*test["dist_range"]), 2)

    async with httpx.AsyncClient() as client:
        resp = await client.post(f"{SERVER_URL}/api/v1/fitness/upload", json=body)
        result = resp.json()
        score = result.get("data", {}).get("score", "?")
        grade = result.get("data", {}).get("grade", "?")
        print(f"  {student_id} | {test_type:15s} | 评分: {score:>3} | 等级: {grade}")
        return result


async def upload_batch():
    """批量上传所有学生的所有项目"""
    print("=" * 60)
    print("  模拟黄山派批量上传体测数据")
    print("=" * 60)

    for student_id, gender in STUDENTS:
        print(f"\n📤 学生 {student_id} 上传中...")
        for test in TESTS:
            # 男生不跑800m，女生不跑1000m
            if gender == "M" and test["test_type"] == "800m_run":
                continue
            if gender == "F" and test["test_type"] == "1000m_run":
                continue
            await upload_record(student_id, test)
            await asyncio.sleep(0.1)

    print("\n✅ 批量上传完成!")


async def mqtt_simulate():
    """MQTT 模拟（如果EMQX已启动）"""
    try:
        import paho.mqtt.client as mqtt
        import json

        client = mqtt.Client()
        client.connect("localhost", 1883, 60)

        for student_id, gender in STUDENTS:
            data = {
                "student_id": student_id,
                "test_type": "jump_rope",
                "count": random.randint(120, 185),
                "duration_sec": 60,
                "avg_hr": random.randint(120, 150),
                "max_hr": random.randint(150, 180),
            }
            topic = f"fitness/HS-2026-0001/data"
            client.publish(topic, json.dumps(data))
            print(f"  MQTT → {topic}: {student_id} jump_rope={data['count']}")
            await asyncio.sleep(0.5)

        client.disconnect()
        print("\n✅ MQTT 模拟完成!")
    except Exception as e:
        print(f"MQTT 模拟跳过（EMQX未启动？）: {e}")


async def main():
    print("\n选择操作:")
    print("1. HTTP 批量上传模拟")
    print("2. MQTT 实时上传模拟")
    print("3. 全部执行")
    choice = input("输入选择 (1/2/3): ").strip()

    if choice in ("1", "3"):
        await upload_batch()
    if choice in ("2", "3"):
        print("\n📡 MQTT 模拟上传...")
        await mqtt_simulate()


if __name__ == "__main__":
    asyncio.run(main())
