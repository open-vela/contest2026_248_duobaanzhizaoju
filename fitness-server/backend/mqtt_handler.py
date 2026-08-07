"""
MQTT Handler - paho-mqtt subscriber
Connects to mosquitto broker (localhost:1883) to receive device data.

Watch device connects to: <server_ip>:1883 (mosquitto)
Publish:  fitness/{device_id}/data    (fitness test data)
          fitness/{device_id}/status  (device heartbeat)

Setup:
  1. Install mosquitto: choco install mosquitto or download from mosquitto.org
  2. Config: listener 1883 / allow_anonymous true
  3. Start: mosquitto -c mosquitto.conf
  4. This handler auto-connects on server startup
"""
import json
import logging
import asyncio
import threading
from datetime import datetime, timezone
from typing import Optional

import paho.mqtt.client as mqtt

from config import settings
from database import async_session
from models import FitnessRecord, Student, Device
from scoring import calculate_score, TEST_TYPE_LABELS
from sqlalchemy import select

logger = logging.getLogger(__name__)


class MQTTHandler:
    def __init__(self):
        self.client: Optional[mqtt.Client] = None
        self._loop: Optional[asyncio.AbstractEventLoop] = None
        self._running = False
        self._stats = {"received": 0, "processed": 0, "failed": 0}

    async def start(self):
        self._loop = asyncio.get_event_loop()
        self._running = True
        self._start_client()

    async def stop(self):
        self._running = False
        if self.client:
            self.client.loop_stop()
            self.client.disconnect()
        logger.info("MQTT stopped")

    def _start_client(self):
        self.client = mqtt.Client(client_id="fitness-server-sub", protocol=mqtt.MQTTv311)
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.client.on_disconnect = self._on_disconnect
        self.client.reconnect_delay_set(min_delay=2, max_delay=30)

        host = settings.MQTT_HOST or "127.0.0.1"
        try:
            self.client.connect(host, settings.MQTT_PORT, 60)
            self.client.loop_start()
            logger.info(f"MQTT subscriber connecting to {host}:{settings.MQTT_PORT}")
        except Exception as e:
            logger.warning(f"MQTT connect failed: {e}, will retry in background")
            self.client.loop_start()

    def _on_connect(self, client, userdata, flags, rc, *args):
        if rc == 0:
            client.subscribe("fitness/+/data")
            client.subscribe("fitness/+/status")
            logger.info("MQTT connected, subscribed: fitness/+/data, fitness/+/status")
        else:
            logger.error(f"MQTT connect failed: rc={rc}")

    def _on_disconnect(self, client, userdata, rc, *args):
        if rc != 0:
            logger.warning(f"MQTT disconnected (rc={rc}), auto-reconnecting...")

    def _on_message(self, client, userdata, msg):
        self._stats["received"] += 1
        topic = msg.topic
        try:
            payload = json.loads(msg.payload.decode("utf-8"))
        except (json.JSONDecodeError, UnicodeDecodeError) as e:
            logger.warning(f"MQTT JSON parse error on {topic}: {e}")
            self._stats["failed"] += 1
            return

        if self._loop and self._loop.is_running():
            asyncio.run_coroutine_threadsafe(
                self._handle_message(topic, payload), self._loop
            )
        else:
            logger.error("Event loop not available")

    async def _handle_message(self, topic: str, payload: dict):
        try:
            parts = topic.split("/")
            if len(parts) < 3:
                return
            device_id = parts[1]
            msg_type = parts[2]

            if msg_type == "data":
                await self._handle_fitness_data(device_id, payload)
            elif msg_type == "status":
                await self._handle_device_status(device_id, payload)

            self._stats["processed"] += 1
        except Exception as e:
            self._stats["failed"] += 1
            logger.error(f"MQTT handle error: {e}")

    async def _handle_fitness_data(self, device_id: str, data: dict):
        student_id = data.get("student_id")
        test_type = data.get("test_type", "")
        if not student_id or not test_type:
            logger.warning(f"MQTT data missing fields from {device_id}")
            return

        async with async_session() as db:
            stu_result = await db.execute(
                select(Student).where(Student.student_id == student_id)
            )
            student = stu_result.scalar_one_or_none()
            if not student:
                logger.warning(f"MQTT: student not found: {student_id}")
                return

            score = None
            grade = None
            value = None

            if test_type in ("jump_rope", "sit_ups", "pull_up"):
                count = data.get("count")
                if count is not None:
                    value = float(count)
            elif test_type in ("50m_run", "800m_run", "1000m_run"):
                ts = data.get("time_sec")
                if ts is not None:
                    value = float(ts)
            elif test_type == "long_jump":
                dist = data.get("distance_m")
                if dist is not None:
                    value = float(dist) * 100
            elif test_type == "sit_reach":
                dist = data.get("distance_m")
                if dist is not None:
                    value = float(dist)

            if value is not None:
                score, grade = calculate_score(test_type, student.gender, value)

            test_date_str = data.get("test_date")
            if test_date_str:
                try:
                    test_date = datetime.fromisoformat(test_date_str)
                except ValueError:
                    test_date = datetime.now(timezone.utc)
            else:
                test_date = datetime.now(timezone.utc)

            record = FitnessRecord(
                student_id=student_id,
                device_id=device_id,
                test_type=test_type,
                test_date=test_date,
                duration_sec=data.get("duration_sec"),
                count=data.get("count"),
                distance_m=data.get("distance_m"),
                time_sec=data.get("time_sec"),
                avg_hr=data.get("avg_hr"),
                max_hr=data.get("max_hr"),
                hr_zones=data.get("hr_zones"),
                score=score,
                grade=grade,
                raw_data=json.dumps(data, ensure_ascii=False)[:2000] if data.get("raw_data") else None,
                note=data.get("note"),
            )
            db.add(record)

            dev_result = await db.execute(
                select(Device).where(Device.device_id == device_id)
            )
            device = dev_result.scalar_one_or_none()
            if device:
                device.last_online = datetime.now(timezone.utc)
                if data.get("battery") is not None:
                    device.battery_level = data.get("battery")

            await db.commit()

            label = TEST_TYPE_LABELS.get(test_type, test_type)
            logger.info(
                f"MQTT data saved: {student_id} {label} "
                f"score={score} grade={grade} (device={device_id})"
            )

    async def _handle_device_status(self, device_id: str, data: dict):
        async with async_session() as db:
            result = await db.execute(
                select(Device).where(Device.device_id == device_id)
            )
            device = result.scalar_one_or_none()
            if device:
                device.last_online = datetime.now(timezone.utc)
                if data.get("battery") is not None:
                    device.battery_level = data.get("battery")
                await db.commit()
                logger.info(f"Device heartbeat: {device_id} battery={data.get('battery')}%")
            else:
                logger.warning(f"Unknown device: {device_id}")

    def get_stats(self) -> dict:
        return {
            "running": self._running,
            "received": self._stats["received"],
            "processed": self._stats["processed"],
            "failed": self._stats["failed"],
        }


mqtt_handler = MQTTHandler()
