"""体测评分引擎 — 基于《国家学生体质健康标准》"""
from typing import Optional


# 评分表: {test_type: {gender: {threshold: score}}}
# threshold 对计数类是"≥"，对计时类是"≤"
SCORE_TABLES = {
    "jump_rope": {
        "M": {180: 100, 170: 95, 160: 90, 140: 80, 120: 70, 100: 60, 80: 50, 0: 0},
        "F": {170: 100, 160: 95, 150: 90, 130: 80, 110: 70, 90: 60, 70: 50, 0: 0},
    },
    "50m_run": {  # 秒，值越小越好
        "M": {0: 100, 7.0: 95, 7.3: 90, 7.5: 80, 7.8: 70, 8.0: 60, 8.5: 50, 99: 0},
        "F": {0: 100, 7.5: 95, 7.8: 90, 8.0: 80, 8.3: 70, 8.5: 60, 9.0: 50, 99: 0},
    },
    "800m_run": {  # 秒（女生）
        "F": {0: 100, 220: 95, 230: 90, 240: 80, 255: 70, 270: 60, 285: 50, 9999: 0},
    },
    "1000m_run": {  # 秒（男生）
        "M": {0: 100, 230: 95, 240: 90, 255: 80, 270: 70, 285: 60, 300: 50, 9999: 0},
    },
    "long_jump": {  # 立定跳远，cm
        "M": {250: 100, 240: 95, 230: 90, 215: 80, 200: 70, 185: 60, 170: 50, 0: 0},
        "F": {200: 100, 190: 95, 180: 90, 170: 80, 155: 70, 140: 60, 130: 50, 0: 0},
    },
    "sit_ups": {  # 仰卧起坐，次/分钟
        "M": {55: 100, 52: 95, 49: 90, 44: 80, 39: 70, 34: 60, 29: 50, 0: 0},
        "F": {50: 100, 47: 95, 44: 90, 39: 80, 34: 70, 29: 60, 24: 50, 0: 0},
    },
    "pull_up": {  # 引体向上，次（男生）
        "M": {15: 100, 13: 95, 11: 90, 9: 80, 7: 70, 5: 60, 3: 50, 0: 0},
    },
    "sit_reach": {  # 坐位体前屈，cm
        "M": {23: 100, 20: 95, 17: 90, 13: 80, 9: 70, 5: 60, 1: 50, -99: 0},
        "F": {25: 100, 22: 95, 19: 90, 15: 80, 11: 70, 7: 60, 3: 50, -99: 0},
    },
}

# 等级映射
GRADE_MAP = [
    (90, "excellent", "优秀"),
    (80, "good", "良好"),
    (60, "pass", "及格"),
    (0, "fail", "不及格"),
]


def calculate_score(test_type: str, gender: str, value: float) -> tuple[int, str]:
    """
    计算体测评分。
    
    Args:
        test_type: 体测项目类型
        gender: M 或 F
        value: 成绩值（计数类为次数，计时类为秒数，距离类为cm）
    
    Returns:
        (score 0-100, grade: excellent/good/pass/fail)
    """
    table = SCORE_TABLES.get(test_type, {})
    gender_table = table.get(gender, table.get("M", {}))

    if not gender_table:
        return (0, "fail")

    # 判断是"越大越好"还是"越小越好"
    # 计时类项目：50m_run, 800m_run, 1000m_run
    is_time_based = test_type in ("50m_run", "800m_run", "1000m_run")

    if is_time_based:
        # 值越小成绩越好，threshold从0递增
        for threshold in sorted(gender_table.keys()):
            if threshold == 0:
                continue
            if value <= threshold:
                score = gender_table[threshold]
                break
        else:
            score = 0
    else:
        # 值越大成绩越好
        for threshold in sorted(gender_table.keys(), reverse=True):
            if value >= threshold:
                score = gender_table[threshold]
                break
        else:
            score = 0

    # 映射等级
    grade = "fail"
    for threshold, grade_en, _ in GRADE_MAP:
        if score >= threshold:
            grade = grade_en
            break

    return (score, grade)


def get_grade_label(grade: str) -> str:
    """获取等级中文标签"""
    for _, grade_en, label in GRADE_MAP:
        if grade == grade_en:
            return label
    return "未知"


# 体测项目中文名
TEST_TYPE_LABELS = {
    "jump_rope": "跳绳",
    "50m_run": "50米跑",
    "800m_run": "800米跑",
    "1000m_run": "1000米跑",
    "long_jump": "立定跳远",
    "sit_ups": "仰卧起坐",
    "pull_up": "引体向上",
    "sit_reach": "坐位体前屈",
}
