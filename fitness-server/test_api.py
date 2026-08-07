import httpx
import json

# Login
r = httpx.post('http://localhost:8000/api/v1/auth/login', json={'username':'admin','password':'admin123'})
print('Login:', r.status_code, r.json()['real_name'])
token = r.json()['access_token']
headers = {'Authorization': f'Bearer {token}'}

# Overview
r = httpx.get('http://localhost:8000/api/v1/statistics/overview', headers=headers)
print('Overview:', json.dumps(r.json(), ensure_ascii=False))

# Students
r = httpx.get('http://localhost:8000/api/v1/students?page=1&page_size=5', headers=headers)
print('Students:', len(r.json()))

# Records
r = httpx.get('http://localhost:8000/api/v1/fitness/records?page=1&page_size=5', headers=headers)
print('Records:', len(r.json()))
for rec in r.json()[:3]:
    sid = rec['student_id']
    ttype = rec['test_type']
    score = rec['score']
    grade = rec['grade']
    print(f'  {sid} {ttype} score={score} grade={grade}')

# Leaderboard
r = httpx.get('http://localhost:8000/api/v1/statistics/leaderboard?limit=5', headers=headers)
print('Leaderboard:')
for item in r.json():
    print(f'  #{item["rank"]} {item["name"]} score={item["best_score"]}')

# Web UI check
r = httpx.get('http://localhost:8000/')
print('Web UI status:', r.status_code, 'HTML' if '<html' in r.text.lower() else 'not html')
