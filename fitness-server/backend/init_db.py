"""Database init script - creates tables + sample data"""
import asyncio
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))

from database import engine, async_session, Base
from models import School, Class, Student, Device, User, FitnessRecord, TeacherClass
from auth import hash_password
from sqlalchemy import select
from datetime import datetime, timezone, date
from scoring import calculate_score


async def init():
    async with engine.begin() as conn:
        await conn.run_sync(Base.metadata.create_all)
        print("[OK] Database tables created")

    async with async_session() as db:
        # ===== 学校 =====
        school = School(name="Demo Middle School", address="Beijing")
        db.add(school)
        await db.flush()
        print(f"[OK] School: {school.name} (ID={school.id})")

        # ===== 班级 (6个班) =====
        classes_data = [
            ("Grade7", "Class1", "Zhao"),
            ("Grade7", "Class2", "Sun"),
            ("Grade8", "Class1", "Qian"),
            ("Grade8", "Class2", "Zhou"),
            ("Grade9", "Class1", "Wang"),
            ("Grade9", "Class2", "Li"),
        ]
        cls_objs = []
        for grade, name, teacher in classes_data:
            cls = Class(school_id=school.id, grade=grade, class_name=name, head_teacher=teacher)
            db.add(cls)
            cls_objs.append(cls)
        await db.flush()
        print(f"[OK] Classes: {len(cls_objs)} created")
        for i, c in enumerate(cls_objs):
            print(f"   {c.grade} {c.class_name} (ID={c.id})")

        # ===== 用户 =====
        admin = User(username="admin", password_hash=hash_password("admin123"), real_name="Admin", role="admin")
        db.add(admin)
        await db.flush()

        # 教师：每个班级一个教师，初三2班额外绑定一个教师做示范
        teachers_data = [
            ("teacher_zhao", "teacher123", "Teacher Zhao", "teacher"),   # Grade7 Class1
            ("teacher_sun", "teacher123", "Teacher Sun", "teacher"),     # Grade7 Class2
            ("teacher_qian", "teacher123", "Teacher Qian", "teacher"),   # Grade8 Class1
            ("teacher_zhou", "teacher123", "Teacher Zhou", "teacher"),   # Grade8 Class2
            ("teacher_wang", "teacher123", "Teacher Wang", "teacher"),   # Grade9 Class1
            ("teacher_li", "teacher123", "Teacher Li", "teacher"),       # Grade9 Class2
        ]
        teacher_objs = []
        for uname, pwd, rname, role in teachers_data:
            t = User(username=uname, password_hash=hash_password(pwd), real_name=rname, role=role)
            db.add(t)
            teacher_objs.append(t)
        await db.flush()
        print(f"[OK] Users: admin + {len(teacher_objs)} teachers")

        # ===== 教师-班级绑定 =====
        # 每个教师绑定一个班级
        bindings = [
            (teacher_objs[0].id, cls_objs[0].id),  # Zhao -> Grade7 Class1
            (teacher_objs[1].id, cls_objs[1].id),  # Sun -> Grade7 Class2
            (teacher_objs[2].id, cls_objs[2].id),  # Qian -> Grade8 Class1
            (teacher_objs[3].id, cls_objs[3].id),  # Zhou -> Grade8 Class2
            (teacher_objs[4].id, cls_objs[4].id),  # Wang -> Grade9 Class1
            (teacher_objs[5].id, cls_objs[5].id),  # Li -> Grade9 Class2
        ]
        for tid, cid in bindings:
            db.add(TeacherClass(teacher_id=tid, class_id=cid))
        await db.flush()
        print("[OK] Teacher-Class bindings created")

        # ===== 学生 (每班8人) =====
        # 中文姓名列表
        boy_names = ["张明", "李华", "刘阳", "王强", "陈杰", "赵磊", "孙浩", "周鹏",
                     "吴勇", "郑涛", "马飞", "黄超", "林峰", "徐亮", "高志"]
        girl_names = ["王芳", "陈静", "刘丽", "张雪", "李婷", "赵敏", "孙琳", "周慧",
                      "吴婉", "郑萍", "马莹", "黄悦", "林娇", "徐颖", "高兰"]

        students_list = []
        student_id_counter = 1

        for cls_idx, cls in enumerate(cls_objs):
            grade = cls.grade
            # 每班4男4女
            for j in range(4):
                sid = f"STU2026{student_id_counter:04d}"
                name = boy_names[j + cls_idx * 4] if j + cls_idx * 4 < len(boy_names) else f"Boy{j}"
                h = 155 + cls_idx * 3 + j * 2 + (3 if grade == "Grade9" else 0)
                w = 45 + cls_idx * 2 + j * 3 + (5 if grade == "Grade9" else 0)
                s = Student(
                    student_id=sid, name=name, gender="M", grade=grade,
                    class_id=cls.id, school_id=school.id,
                    birth_date=date(2008 + cls_idx, 3 + j, 10),
                    height_cm=h, weight_kg=w
                )
                db.add(s)
                students_list.append(s)
                student_id_counter += 1

            for j in range(4):
                sid = f"STU2026{student_id_counter:04d}"
                name = girl_names[j + cls_idx * 4] if j + cls_idx * 4 < len(girl_names) else f"Girl{j}"
                h = 148 + cls_idx * 2 + j * 2 + (2 if grade == "Grade9" else 0)
                w = 40 + cls_idx * 1 + j * 2 + (3 if grade == "Grade9" else 0)
                s = Student(
                    student_id=sid, name=name, gender="F", grade=grade,
                    class_id=cls.id, school_id=school.id,
                    birth_date=date(2008 + cls_idx, 6 + j, 15),
                    height_cm=h, weight_kg=w
                )
                db.add(s)
                students_list.append(s)
                student_id_counter += 1

        await db.flush()
        print(f"[OK] Students: {len(students_list)} created")
        for s in students_list[:5]:
            print(f"   {s.student_id}: {s.name} ({s.gender}), Class={s.class_id}")

        # ===== 设备 =====
        # 为部分学生绑定设备
        device_students = students_list[:12]  # 前12个学生
        device_objs = []
        for i, stu in enumerate(device_students):
            dev = Device(
                device_id=f"HS-2026-{i+1:04d}",
                student_id=stu.student_id,
                device_name=f"{stu.name} Watch",
                battery_level=80 + i % 20,
                last_online=datetime(2026, 7, 8, 10 + i, 0),
                is_active=True if i < 10 else False
            )
            db.add(dev)
            device_objs.append(dev)
        await db.flush()
        print(f"[OK] Devices: {len(device_objs)} created")

        # ===== 体测记录 =====
        # 每个学生有2-3个项目的记录，2个测试日期
        test_dates = [
            datetime(2026, 7, 1, 10, 0),
            datetime(2026, 7, 5, 10, 0),
            datetime(2026, 7, 8, 14, 0),
        ]

        # 测试项目及对应的值生成函数
        def gen_value(test_type, gender, base_variation):
            """根据项目类型和性别生成合理的测试值"""
            if test_type == "jump_rope":
                base = 160 if gender == "M" else 140
                return max(60, int(base + base_variation * 10)), None, None, None
            elif test_type == "50m_run":
                base = 7.2 if gender == "M" else 7.8
                return None, None, None, round(base + base_variation * 0.3, 2)
            elif test_type == "800m_run" and gender == "F":
                base = 230
                return None, None, None, round(base + base_variation * 10, 1)
            elif test_type == "1000m_run" and gender == "M":
                base = 240
                return None, None, None, round(base + base_variation * 10, 1)
            elif test_type == "long_jump":
                base = 2.3 if gender == "M" else 1.8
                return None, round(base + base_variation * 0.1, 2), None, None
            elif test_type == "sit_ups":
                base = 48 if gender == "M" else 42
                return max(20, int(base + base_variation * 5)), None, None, None
            elif test_type == "pull_up" and gender == "M":
                base = 8
                return max(1, int(base + base_variation * 2)), None, None, None
            elif test_type == "sit_reach":
                base = 15 if gender == "M" else 18
                return None, round(base + base_variation * 2, 1) / 100, None, None  # sit_reach distance in m
            return None, None, None, None

        # 为每个学生生成2-4个项目的记录
        record_count = 0
        for idx, stu in enumerate(students_list):
            # 随机选项目（确保性别匹配）
            available_types = ["jump_rope", "50m_run", "long_jump", "sit_ups", "sit_reach"]
            if stu.gender == "M":
                available_types.append("1000m_run")
                available_types.append("pull_up")
            else:
                available_types.append("800m_run")

            # 每个学生2-3个项目
            import random
            random.seed(idx)  # 固定种子，结果一致
            chosen_types = random.sample(available_types, min(3, len(available_types)))

            variation = (idx % 5 - 2) / 2  # -1 to 1 的变化

            for ttype in chosen_types:
                for date_idx, tdate in enumerate(test_dates[:2]):  # 2个测试日期
                    count_val, dist_val, dur_val, time_val = gen_value(ttype, stu.gender, variation + date_idx * 0.2)

                    # 计算评分
                    value = None
                    if ttype in ("jump_rope", "sit_ups", "pull_up") and count_val is not None:
                        value = float(count_val)
                    elif ttype in ("50m_run", "800m_run", "1000m_run") and time_val is not None:
                        value = time_val
                    elif ttype == "long_jump" and dist_val is not None:
                        value = dist_val * 100
                    elif ttype == "sit_reach" and dist_val is not None:
                        value = dist_val

                    score, grade = (0, "fail")
                    if value is not None:
                        score, grade = calculate_score(ttype, stu.gender, value)

                    avg_hr = 130 + int(variation * 15) + date_idx * 5
                    max_hr = avg_hr + 20 + int(variation * 10)

                    dev_id = None
                    # 如果该学生有设备，用设备ID
                    for dev in device_objs:
                        if dev.student_id == stu.student_id:
                            dev_id = dev.device_id
                            break

                    rec = FitnessRecord(
                        student_id=stu.student_id,
                        device_id=dev_id,
                        test_type=ttype,
                        test_date=tdate,
                        count=count_val,
                        distance_m=dist_val,
                        duration_sec=dur_val,
                        time_sec=time_val,
                        avg_hr=avg_hr,
                        max_hr=max_hr,
                        score=score,
                        grade=grade,
                    )
                    db.add(rec)
                    record_count += 1

        await db.flush()
        print(f"[OK] Sample records: {record_count} created")

        await db.commit()
        print("\n[DONE] Database init complete!")
        print("   Admin:   admin / admin123")
        print("   Teacher: teacher_zhao / teacher123 (Grade7 Class1)")
        print("   Teacher: teacher_sun / teacher123 (Grade7 Class2)")
        print("   Teacher: teacher_qian / teacher123 (Grade8 Class1)")
        print("   Teacher: teacher_zhou / teacher123 (Grade8 Class2)")
        print("   Teacher: teacher_wang / teacher123 (Grade9 Class1)")
        print("   Teacher: teacher_li / teacher123 (Grade9 Class2)")
        print(f"   Total students: {len(students_list)} (8 per class x 6 classes)")
        print(f"   Total records: {record_count}")
        print(f"   Total devices: {len(device_objs)}")


if __name__ == "__main__":
    asyncio.run(init())
