"""
AI 报告生成引擎
聚合学生近一周体测数据 + 心率数据，调用大模型 API 生成个人分析报告。
支持：OpenAI / DeepSeek / 智谱 / 通义千问 / 豆包 / Kimi / 小米MiMo
"""
import json
import logging
import httpx
from datetime import datetime, date, timedelta
from typing import Optional

from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from models import AIConfig, AIReport, Student, FitnessRecord
from scoring import TEST_TYPE_LABELS, get_grade_label

logger = logging.getLogger(__name__)

# 各厂商默认 API Base URL
PROVIDER_BASES = {
    "openai":   "https://api.openai.com/v1",
    "deepseek": "https://api.deepseek.com/v1",
    "zhipu":    "https://open.bigmodel.cn/api/paas/v4",
    "qwen":     "https://dashscope.aliyuncs.com/compatible-mode/v1",
    "doubao":   "https://ark.cn-beijing.volces.com/api/v3",
    "kimi":     "https://api.moonshot.cn/v1",
    "mimo":     "https://token-plan-cn.xiaomimimo.com/v1",
}


async def get_active_config(db: AsyncSession) -> Optional[AIConfig]:
    """获取当前激活的 AI 配置"""
    result = await db.execute(
        select(AIConfig).where(AIConfig.is_active == True).order_by(AIConfig.id.desc())
    )
    return result.scalar_one_or_none()


def mask_key(key: str) -> str:
    """掩码 API Key，只显示前4后4"""
    if len(key) <= 8:
        return key[:2] + "****"
    return key[:4] + "****" + key[-4:]


def _build_records_summary(records: list[FitnessRecord], student: Student) -> str:
    """将体测记录汇总为文本"""
    if not records:
        return "本周无体测记录。"

    lines = []
    lines.append(f"学生: {student.name} | 性别: {'男' if student.gender == 'M' else '女'} | 年级: {student.grade}")
    if student.height_cm:
        lines.append(f"身高: {student.height_cm}cm | 体重: {student.weight_kg}kg")

    lines.append(f"\n本周共完成 {len(records)} 次体测，详情如下:\n")

    # 按项目分组
    by_type = {}
    for r in records:
        by_type.setdefault(r.test_type, []).append(r)

    for test_type, recs in by_type.items():
        label = TEST_TYPE_LABELS.get(test_type, test_type)
        scores = [r.score for r in recs if r.score is not None]
        avg_score = sum(scores) / len(scores) if scores else 0
        best_score = max(scores) if scores else 0

        lines.append(f"【{label}】共 {len(recs)} 次 | 平均分: {avg_score:.0f} | 最高分: {best_score}")

        # 成绩详情
        for r in recs:
            date_str = r.test_date.strftime("%m-%d %H:%M")
            parts = [f"  {date_str}"]
            if r.count is not None:
                parts.append(f"{r.count}次")
            if r.time_sec is not None:
                m, s = divmod(int(r.time_sec), 60)
                parts.append(f"{m}'{s:02d}\"")
            if r.distance_m is not None:
                parts.append(f"{r.distance_m}米")
            if r.score is not None:
                parts.append(f"得分{r.score}({get_grade_label(r.grade)})")
            lines.append(" | ".join(parts))

        # 心率数据
        hr_records = [r for r in recs if r.avg_hr is not None]
        if hr_records:
            avg_hr = sum(r.avg_hr for r in hr_records) / len(hr_records)
            max_hr = max((r.max_hr or 0) for r in hr_records)
            lines.append(f"  心率: 平均 {avg_hr:.0f}bpm | 最高 {max_hr}bpm")

    return "\n".join(lines)


def _build_prompt(records_text: str, student: Student, week_start: date, week_end: date) -> str:
    """构建发给大模型的 prompt"""
    gender_cn = "男" if student.gender == "M" else "女"
    bmi_str = ""
    if student.height_cm and student.weight_kg:
        bmi = student.weight_kg / ((student.height_cm / 100) ** 2)
        bmi_str = f"BMI: {bmi:.1f}"

    return f"""你是一位专业的青少年体能训练师和健康顾问。请根据以下学生的体测数据，生成一份本周的个人体测分析报告。

## 报告要求
1. 使用 HTML 格式输出，适合在手机/手表屏幕上阅读
2. 报告结构清晰，包含以下章节：
   - **本周体测概览**：简要总结本周测试项目和总体表现
   - **成绩分析**：逐项分析每个项目的得分、趋势、与上周对比（如有历史数据）
   - **心率分析**：基于平均心率和最高心率，评估运动强度是否合理
   - **体质评估**：结合 BMI、各项成绩给出综合体质评价
   - **运动建议**：针对薄弱项给出具体可执行的训练建议（每周 2-3 次，每次 20-30 分钟）
3. 语气积极鼓励，适合中学生阅读
4. 使用简洁的 HTML 标签（h3, p, ul, li, strong, span），不要使用复杂 CSS
5. 如有心率偏高（平均>170 或最高>200）的情况，给出安全提示

## 学生信息
- 姓名: {student.name}
- 性别: {gender_cn}
- 年级: {student.grade}
- 身高: {student.height_cm or '未知'}cm
- 体重: {student.weight_kg or '未知'}kg
- {bmi_str}

## 本周体测数据（{week_start} ~ {week_end}）
{records_text}

请直接输出 HTML 内容，不要包含 <html><body> 等外层标签。"""


async def call_ai(config: AIConfig, prompt: str) -> str:
    """调用大模型 API"""
    base_url = config.api_base or PROVIDER_BASES.get(config.provider, "")
    if not base_url:
        raise ValueError(f"未知的 AI 服务商: {config.provider}")

    url = f"{base_url}/chat/completions"
    # 小米 MiMo 同时支持 Authorization: Bearer 和 api-key 头
    headers = {
        "Authorization": f"Bearer {config.api_key}",
        "api-key": config.api_key,
        "Content-Type": "application/json",
    }
    payload = {
        "model": config.model_name,
        "messages": [
            {"role": "system", "content": "你是一位专业的青少年体能训练师，擅长分析体测数据并给出运动建议。"},
            {"role": "user", "content": prompt},
        ],
        "temperature": 0.7,
        "max_tokens": 2000,
    }

    async with httpx.AsyncClient(timeout=60) as client:
        resp = await client.post(url, headers=headers, json=payload)
        resp.raise_for_status()
        data = resp.json()
        return data["choices"][0]["message"]["content"]


async def generate_weekly_report(
    db: AsyncSession,
    student: Student,
    week_start: date,
    week_end: date,
    config: Optional[AIConfig] = None,
) -> AIReport:
    """为单个学生生成周报"""
    if config is None:
        config = await get_active_config(db)
    if not config:
        raise ValueError("未配置 AI 模型，请先在设置页面配置 API Key")

    # 查询本周体测记录
    start_dt = datetime.combine(week_start, datetime.min.time())
    end_dt = datetime.combine(week_end, datetime.max.time())

    result = await db.execute(
        select(FitnessRecord)
        .where(FitnessRecord.student_id == student.student_id)
        .where(FitnessRecord.test_date >= start_dt)
        .where(FitnessRecord.test_date <= end_dt)
        .order_by(FitnessRecord.test_date)
    )
    records = list(result.scalars().all())

    # 汇总数据
    records_text = _build_records_summary(records, student)
    prompt = _build_prompt(records_text, student, week_start, week_end)

    # 调用 AI
    logger.info(f"Generating report for {student.student_id} ({student.name}), week {week_start}~{week_end}")
    html_content = await call_ai(config, prompt)

    # 清理 HTML：去掉可能的 markdown 代码块标记
    html_content = html_content.strip()
    if html_content.startswith("```html"):
        html_content = html_content[7:]
    if html_content.startswith("```"):
        html_content = html_content[3:]
    if html_content.endswith("```"):
        html_content = html_content[:-3]
    html_content = html_content.strip()

    # 生成摘要
    summary = f"本周完成 {len(records)} 次体测"
    scores = [r.score for r in records if r.score is not None]
    if scores:
        summary += f"，平均分 {sum(scores)/len(scores):.0f}，最高分 {max(scores)}"

    # 保存报告
    report = AIReport(
        student_id=student.student_id,
        week_start=week_start,
        week_end=week_end,
        report_html=html_content,
        report_summary=summary,
        provider=config.provider,
        model_name=config.model_name,
    )
    db.add(report)
    await db.flush()
    await db.refresh(report)

    logger.info(f"Report generated: id={report.id}, student={student.student_id}")
    return report


def get_current_week_range() -> tuple[date, date]:
    """获取本周一到本周日的日期范围"""
    today = date.today()
    monday = today - timedelta(days=today.weekday())
    sunday = monday + timedelta(days=6)
    return monday, sunday
