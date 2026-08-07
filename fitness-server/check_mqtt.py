import httpx
r = httpx.post('http://localhost:8000/api/v1/auth/login', json={'username':'admin','password':'admin123'})
token = r.json()['access_token']
h = {'Authorization': f'Bearer {token}'}

r = httpx.get('http://localhost:8000/api/v1/mqtt/status', headers=h)
print('MQTT Status:', r.json())

r = httpx.get('http://localhost:8000/api/v1/statistics/overview', headers=h)
print('Overview:', r.json())

r = httpx.get('http://localhost:8000/api/v1/fitness/records?page=1&page_size=5', headers=h)
print('Latest 5 records:')
for rec in r.json():
    sid = rec['student_id']
    ttype = rec['test_type']
    score = rec['score']
    grade = rec['grade']
    dev = rec['device_id']
    print(f'  {sid} {ttype} score={score} grade={grade} device={dev}')
