"""
Simulate watch device sending data via MQTT to the server.
Requires: paho-mqtt, mosquitto running on localhost:1883
"""
import json
import random
import paho.mqtt.client as mqtt
from datetime import datetime, timezone, timedelta

BROKER_HOST = "127.0.0.1"
BROKER_PORT = 1883

STUDENTS = [
    ("STU20260001", "M"),
    ("STU20260002", "M"),
    ("STU20260003", "F"),
    ("STU20260004", "M"),
    ("STU20260005", "F"),
]

TESTS = [
    {"test_type": "jump_rope", "count_range": (120, 185), "duration": 60},
    {"test_type": "50m_run", "time_range": (7.0, 8.5)},
    {"test_type": "long_jump", "dist_range": (1.7, 2.5)},
    {"test_type": "sit_ups", "count_range": (35, 55), "duration": 60},
    {"test_type": "1000m_run", "time_range": (220, 300)},
    {"test_type": "800m_run", "time_range": (210, 280)},
]


def send_mqtt_data():
    client = mqtt.Client(client_id="watch-simulator", protocol=mqtt.MQTTv311)
    client.connect(BROKER_HOST, BROKER_PORT, 60)
    client.loop_start()

    tz = timezone(timedelta(hours=8))
    now = datetime.now(tz)

    for student_id, gender in STUDENTS:
        for test in TESTS:
            # Skip gender-inappropriate tests
            if gender == "M" and test["test_type"] == "800m_run":
                continue
            if gender == "F" and test["test_type"] == "1000m_run":
                continue

            data = {
                "student_id": student_id,
                "test_type": test["test_type"],
                "test_date": now.isoformat(),
                "avg_hr": random.randint(120, 150),
                "max_hr": random.randint(150, 180),
                "battery": random.randint(60, 100),
            }

            if "count_range" in test:
                data["count"] = random.randint(*test["count_range"])
                data["duration_sec"] = test.get("duration", 60)
            elif "time_range" in test:
                data["time_sec"] = round(random.uniform(*test["time_range"]), 2)
            elif "dist_range" in test:
                data["distance_m"] = round(random.uniform(*test["dist_range"]), 2)

            device_id = "HS-2026-0001"
            topic = f"fitness/{device_id}/data"
            payload = json.dumps(data, ensure_ascii=False)

            client.publish(topic, payload, qos=1)
            print(f"  MQTT -> {topic}: {student_id} {test['test_type']}")

    # Send device status
    status_topic = "fitness/HS-2026-0001/status"
    status = json.dumps({"battery": 85, "firmware": "1.0.0"})
    client.publish(status_topic, status, qos=1)
    print(f"  MQTT -> {status_topic}: battery=85%")

    import time
    time.sleep(2)
    client.disconnect()


if __name__ == "__main__":
    print("=" * 50)
    print("  Watch MQTT Simulator")
    print("=" * 50)
    send_mqtt_data()
    print("\nDone! Check server logs for received data.")
    print("Verify at: http://localhost:8000/api/v1/mqtt/status")
