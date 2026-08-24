/**
 * 中学生体测数据管理平台 - 前端逻辑
 */

const API = '/api/v1';
let token = localStorage.getItem('fitness_token') || '';
let currentUser = null;
let userRole = null;
let chartOverview = null;
let chartStat = null;

const TEST_LABELS = {
    jump_rope: '跳绳', '50m_run': '50米跑', '800m_run': '800米跑',
    '1000m_run': '1000米跑', long_jump: '立定跳远', sit_ups: '仰卧起坐',
    pull_up: '引体向上', sit_reach: '坐位体前屈'
};

const GRADE_LABELS = { excellent: '优秀', good: '良好', pass: '及格', fail: '不及格' };

// ===== API 请求封装 =====
async function api(path, options = {}) {
    const headers = { 'Content-Type': 'application/json', ...options.headers };
    if (token) headers['Authorization'] = `Bearer ${token}`;
    const resp = await fetch(`${API}${path}`, { ...options, headers });
    if (resp.status === 401) { logout(); throw new Error('未授权'); }
    if (resp.status === 403) {
        const err = await resp.json().catch(() => ({}));
        throw new Error(err.detail || '无权访问');
    }
    if (!resp.ok) {
        const err = await resp.json().catch(() => ({}));
        throw new Error(err.detail || `请求失败 ${resp.status}`);
    }
    return resp.json();
}

// ===== 登录/登出 =====
document.getElementById('login-form').addEventListener('submit', async (e) => {
    e.preventDefault();
    const username = document.getElementById('username').value;
    const password = document.getElementById('password').value;
    try {
        const data = await api('/auth/login', {
            method: 'POST',
            body: JSON.stringify({ username, password })
        });
        token = data.access_token;
        localStorage.setItem('fitness_token', token);
        currentUser = data;
        userRole = data.role;
        showApp();
    } catch (err) {
        alert('登录失败: ' + err.message);
    }
});

document.getElementById('logout-btn').addEventListener('click', logout);

function logout() {
    token = '';
    currentUser = null;
    userRole = null;
    localStorage.removeItem('fitness_token');
    document.getElementById('app').style.display = 'none';
    document.getElementById('login-page').style.display = 'flex';
}

function showApp() {
    document.getElementById('login-page').style.display = 'none';
    document.getElementById('app').style.display = 'flex';
    document.getElementById('user-info').textContent = currentUser?.real_name || '用户';

    // 显示角色标识
    const roleBadge = document.getElementById('role-badge');
    if (userRole === 'admin') {
        roleBadge.textContent = '管理员';
        roleBadge.className = 'role-badge role-admin';
    } else {
        roleBadge.textContent = '教师';
        roleBadge.className = 'role-badge role-teacher';
    }

    // 管理员才显示教师绑定管理 & AI 配置
    const navBind = document.getElementById('nav-teacher-bind');
    navBind.style.display = userRole === 'admin' ? 'block' : 'none';
    const navAIConfig = document.getElementById('nav-ai-config');
    if (navAIConfig) navAIConfig.style.display = userRole === 'admin' ? 'block' : 'none';

    loadDashboard();
}

// ===== 动态加载班级列表 =====
async function loadClassOptions(selectId, includeAll = true) {
    try {
        const classes = await api('/teacher-class/all-classes');
        const select = document.getElementById(selectId);
        select.innerHTML = includeAll ? '<option value="">全部班级</option>' : '<option value="">选择班级</option>';
        classes.forEach(c => {
            const opt = document.createElement('option');
            opt.value = c.id;
            opt.textContent = `${c.grade} ${c.class_name}`;
            select.appendChild(opt);
        });
    } catch (err) { console.error('加载班级列表失败', err); }
}

// ===== 导航切换 =====
document.querySelectorAll('.nav-item').forEach(item => {
    item.addEventListener('click', (e) => {
        e.preventDefault();
        document.querySelectorAll('.nav-item').forEach(n => n.classList.remove('active'));
        item.classList.add('active');
        const page = item.dataset.page;
        document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
        document.getElementById(`page-${page}`).classList.add('active');
        const loaders = {
            dashboard: loadDashboard, students: loadStudents, records: loadRecords,
            statistics: loadStatistics, devices: loadDevices, ranking: loadRanking,
            teacherBind: loadTeacherBind, 'ai-config': loadAIConfig,
            'ai-reports': loadAIReports
        };
        loaders[page]?.();
    });
});

// ===== 数据总览 =====
async function loadDashboard() {
    try {
        const data = await api('/statistics/overview');
        document.getElementById('ov-students').textContent = data.total_students;
        document.getElementById('ov-records').textContent = data.total_records;
        document.getElementById('ov-today').textContent = data.today_records;
        document.getElementById('ov-score').textContent = data.avg_score;
        document.getElementById('ov-pass').textContent = data.pass_rate + '%';
        document.getElementById('ov-excellent').textContent = data.excellent_rate + '%';

        const ranking = await api('/statistics/leaderboard?limit=10');
        renderOverviewChart(ranking);
    } catch (err) { console.error(err); }
}

function renderOverviewChart(ranking) {
    const ctx = document.getElementById('chart-overview');
    if (chartOverview) chartOverview.destroy();
    chartOverview = new Chart(ctx, {
        type: 'bar',
        data: {
            labels: ranking.map(r => r.name),
            datasets: [{
                label: '最高分',
                data: ranking.map(r => r.best_score),
                backgroundColor: '#667eea',
                borderRadius: 6
            }]
        },
        options: { responsive: true, scales: { y: { beginAtZero: true, max: 100 } } }
    });
}

// ===== 学生管理 =====
async function loadStudents() {
    loadClassOptions('stu-class-filter', true);
    try {
        const keyword = document.getElementById('stu-search').value;
        const classId = document.getElementById('stu-class-filter').value;
        let path = '/students?page=1&page_size=200';
        if (keyword) path += `&keyword=${encodeURIComponent(keyword)}`;
        if (classId) path += `&class_id=${classId}`;
        const data = await api(path);

        // 获取班级信息以显示班级名称
        const classes = await api('/teacher-class/all-classes');
        const classMap = {};
        classes.forEach(c => classMap[c.id] = `${c.grade} ${c.class_name}`);

        const tbody = document.getElementById('stu-tbody');
        tbody.innerHTML = data.map(s => `
            <tr>
                <td>${s.student_id}</td><td>${s.name}</td>
                <td>${s.gender === 'M' ? '男' : '女'}</td><td>${s.grade}</td>
                <td>${classMap[s.class_id] || '-'}</td>
                <td>${s.height_cm || '-'}</td><td>${s.weight_kg || '-'}</td>
                <td><button onclick="viewStudentRecords('${s.student_id}')">查看记录</button></td>
            </tr>`).join('');
    } catch (err) { console.error(err); }
}

document.getElementById('stu-search')?.addEventListener('input', loadStudents);
document.getElementById('stu-class-filter')?.addEventListener('change', loadStudents);
document.getElementById('stu-add-btn')?.addEventListener('click', () => showAddStudentModal());

function showAddStudentModal() {
    document.getElementById('modal-body').innerHTML = `
        <h3>添加学生</h3>
        <label>学号</label><input id="add-stu-id" placeholder="STU20260001">
        <label>姓名</label><input id="add-stu-name">
        <label>性别</label><select id="add-stu-gender"><option value="M">男</option><option value="F">女</option></select>
        <label>年级</label><input id="add-stu-grade" value="Grade9" placeholder="Grade9">
        <label>班级</label><select id="add-stu-class"><option value="">选择班级</option></select>
        <label>身高(cm)</label><input id="add-stu-height" type="number">
        <label>体重(kg)</label><input id="add-stu-weight" type="number">
        <button onclick="submitAddStudent()">添加</button>`;
    document.getElementById('modal').style.display = 'flex';
    // 动态加载班级选项
    loadClassOptions('add-stu-class', false);
}

async function submitAddStudent() {
    try {
        await api('/students', {
            method: 'POST',
            body: JSON.stringify({
                student_id: document.getElementById('add-stu-id').value,
                name: document.getElementById('add-stu-name').value,
                gender: document.getElementById('add-stu-gender').value,
                grade: document.getElementById('add-stu-grade').value,
                class_id: parseInt(document.getElementById('add-stu-class').value) || null,
                height_cm: parseFloat(document.getElementById('add-stu-height').value) || null,
                weight_kg: parseFloat(document.getElementById('add-stu-weight').value) || null,
            })
        });
        closeModal();
        loadStudents();
    } catch (err) { alert(err.message); }
}

// ===== 体测记录 =====
async function loadRecords() {
    try {
        let path = '/fitness/records?page=1&page_size=200';
        const sid = document.getElementById('rec-student').value;
        const ttype = document.getElementById('rec-type').value;
        if (sid) path += `&student_id=${encodeURIComponent(sid)}`;
        if (ttype) path += `&test_type=${ttype}`;
        const data = await api(path);
        const tbody = document.getElementById('rec-tbody');
        tbody.innerHTML = data.map(r => {
            let scoreText = r.score ?? '-';
            let gradeClass = r.grade ? `grade-${r.grade}` : '';
            let perfText = '';
            if (r.count) perfText = `${r.count}次`;
            else if (r.time_sec) perfText = `${r.time_sec}秒`;
            else if (r.distance_m) perfText = `${r.distance_m}米`;
            return `<tr>
                <td>${r.student_id}</td>
                <td>${TEST_LABELS[r.test_type] || r.test_type}</td>
                <td>${perfText}</td>
                <td>${r.avg_hr ? r.avg_hr + '/' + r.max_hr : '-'}</td>
                <td>${scoreText}</td>
                <td class="${gradeClass}">${GRADE_LABELS[r.grade] || '-'}</td>
                <td>${new Date(r.test_date).toLocaleString('zh-CN')}</td>
            </tr>`;
        }).join('');
    } catch (err) { console.error(err); }
}

document.getElementById('rec-search-btn')?.addEventListener('click', loadRecords);
document.getElementById('rec-add-btn')?.addEventListener('click', () => showAddRecordModal());

function showAddRecordModal() {
    document.getElementById('modal-body').innerHTML = `
        <h3>手动录入体测成绩</h3>
        <label>学号</label><input id="add-rec-sid" placeholder="STU20260001">
        <label>体测项目</label><select id="add-rec-type">
            <option value="jump_rope">跳绳</option><option value="50m_run">50米跑</option>
            <option value="800m_run">800米跑</option><option value="1000m_run">1000米跑</option>
            <option value="long_jump">立定跳远</option><option value="sit_ups">仰卧起坐</option>
            <option value="pull_up">引体向上</option><option value="sit_reach">坐位体前屈</option>
        </select>
        <label>成绩值（次数/秒数/米数）</label><input id="add-rec-val" type="number" step="0.01" placeholder="如跳绳填次数，50m填秒数">
        <label>平均心率（可选）</label><input id="add-rec-avghr" type="number">
        <label>最高心率（可选）</label><input id="add-rec-maxhr" type="number">
        <button onclick="submitAddRecord()">提交</button>`;
    document.getElementById('modal').style.display = 'flex';
}

async function submitAddRecord() {
    const sid = document.getElementById('add-rec-sid').value;
    const ttype = document.getElementById('add-rec-type').value;
    const val = parseFloat(document.getElementById('add-rec-val').value);
    const avgHr = parseInt(document.getElementById('add-rec-avghr').value) || null;
    const maxHr = parseInt(document.getElementById('add-rec-maxhr').value) || null;

    const body = { student_id: sid, test_type: ttype, test_date: new Date().toISOString(), avg_hr: avgHr, max_hr: maxHr };
    if (ttype === 'jump_rope' || ttype === 'sit_ups' || ttype === 'pull_up') body.count = val;
    else if (ttype === '50m_run' || ttype === '800m_run' || ttype === '1000m_run') body.time_sec = val;
    else if (ttype === 'long_jump' || ttype === 'sit_reach') body.distance_m = val;

    try {
        await api('/fitness/records/manual', { method: 'POST', body: JSON.stringify(body) });
        closeModal();
        loadRecords();
    } catch (err) { alert(err.message); }
}

// ===== 统计分析 =====
async function loadStatistics() {
    loadClassOptions('stat-class', false);
    try {
        const classId = document.getElementById('stat-class').value;
        if (!classId) {
            document.getElementById('stat-result').innerHTML = '<p style="text-align:center;color:#888">请选择一个班级</p>';
            return;
        }
        const ttype = document.getElementById('stat-type').value;
        let path = `/statistics/class/${classId}`;
        if (ttype) path += `?test_type=${ttype}`;
        const data = await api(path);

        document.getElementById('stat-result').innerHTML = `
            <div class="stat-result-card"><div class="label">班级人数</div><div class="value">${data.total_students}</div></div>
            <div class="stat-result-card"><div class="label">已测人数</div><div class="value">${data.tested_students}</div></div>
            <div class="stat-result-card"><div class="label">测试率</div><div class="value">${data.test_rate}%</div></div>
            <div class="stat-result-card"><div class="label">平均分</div><div class="value">${data.avg_score}</div></div>
            <div class="stat-result-card"><div class="label">及格率</div><div class="value">${data.pass_rate}%</div></div>
            <div class="stat-result-card"><div class="label">优秀率</div><div class="value">${data.excellent_rate}%</div></div>`;

        const ctx = document.getElementById('chart-stat');
        if (chartStat) chartStat.destroy();
        if (data.test_type_stats && data.test_type_stats.length) {
            chartStat = new Chart(ctx, {
                type: 'bar',
                data: {
                    labels: data.test_type_stats.map(t => t.label),
                    datasets: [
                        { label: '平均分', data: data.test_type_stats.map(t => t.avg_score), backgroundColor: '#667eea', borderRadius: 6 },
                        { label: '最高分', data: data.test_type_stats.map(t => t.max_score), backgroundColor: '#52c41a', borderRadius: 6 },
                        { label: '最低分', data: data.test_type_stats.map(t => t.min_score), backgroundColor: '#f5222d', borderRadius: 6 }
                    ]
                },
                options: { responsive: true, scales: { y: { beginAtZero: true, max: 100 } } }
            });
        }
    } catch (err) { console.error(err); }
}

document.getElementById('stat-search-btn')?.addEventListener('click', loadStatistics);

// ===== 设备管理 =====
async function loadDevices() {
    try {
        const data = await api('/devices');
        const tbody = document.getElementById('dev-tbody');
        tbody.innerHTML = data.map(d => `
            <tr>
                <td>${d.device_id}</td>
                <td>${d.student_id || '-'}</td>
                <td>${d.device_name || '-'}</td>
                <td>${d.battery_level !== null ? d.battery_level + '%' : '-'}</td>
                <td>${d.last_online ? new Date(d.last_online).toLocaleString('zh-CN') : '-'}</td>
                <td>${d.is_active ? '在线' : '离线'}</td>
                <td>${d.student_id ? `<button onclick="unbindDevice('${d.device_id}')">解绑</button>` : ''}</td>
            </tr>`).join('');
    } catch (err) { console.error(err); }
}

document.getElementById('dev-register-btn')?.addEventListener('click', () => {
    document.getElementById('modal-body').innerHTML = `
        <h3>注册设备</h3>
        <label>设备ID</label><input id="reg-dev-id" placeholder="HS-2026-0001">
        <label>设备名称</label><input id="reg-dev-name" placeholder="张明的手表">
        <label>绑定学号（可选）</label><input id="reg-dev-sid" placeholder="STU20260001">
        <button onclick="submitRegisterDevice()">注册</button>`;
    document.getElementById('modal').style.display = 'flex';
});

async function submitRegisterDevice() {
    try {
        await api('/devices/register', {
            method: 'POST',
            body: JSON.stringify({
                device_id: document.getElementById('reg-dev-id').value,
                device_name: document.getElementById('reg-dev-name').value,
                student_id: document.getElementById('reg-dev-sid').value || null,
            })
        });
        closeModal();
        loadDevices();
    } catch (err) { alert(err.message); }
}

async function unbindDevice(deviceId) {
    if (!confirm('确定解绑设备？')) return;
    try {
        await api(`/devices/${deviceId}/unbind`, { method: 'PUT' });
        loadDevices();
    } catch (err) { alert(err.message); }
}

// ===== 排行榜 =====
async function loadRanking() {
    try {
        const ttype = document.getElementById('rank-type').value;
        let path = '/statistics/leaderboard?limit=20';
        if (ttype) path += `&test_type=${ttype}`;
        const data = await api(path);
        const tbody = document.getElementById('rank-tbody');
        tbody.innerHTML = data.map(r => {
            const medal = r.rank === 1 ? '🥇' : r.rank === 2 ? '🥈' : r.rank === 3 ? '🥉' : r.rank;
            return `<tr>
                <td>${medal}</td>
                <td>${r.student_id}</td>
                <td>${r.name}</td>
                <td>${r.grade}</td>
                <td>${r.best_score}</td>
                <td>${r.test_date ? new Date(r.test_date).toLocaleDateString('zh-CN') : '-'}</td>
            </tr>`;
        }).join('');
    } catch (err) { console.error(err); }
}

document.getElementById('rank-search-btn')?.addEventListener('click', loadRanking);

// ===== 教师绑定管理 (管理员) =====
async function loadTeacherBind() {
    if (userRole !== 'admin') return;
    try {
        // 加载绑定列表
        const bindings = await api('/teacher-class/bindings');
        const bindTbody = document.getElementById('bind-tbody');
        bindTbody.innerHTML = bindings.map(b => `
            <tr>
                <td>${b.teacher_name}</td>
                <td>${b.teacher_username}</td>
                <td>${b.class_name}</td>
                <td><button onclick="unbindTeacher(${b.teacher_id}, ${b.class_id})">解绑</button></td>
            </tr>`).join('');

        // 加载教师列表
        const teachers = await api('/teacher-class/all-teachers');
        const teacherTbody = document.getElementById('teacher-tbody');
        // 计算每个教师的绑定班级数
        const teacherBindCount = {};
        bindings.forEach(b => {
            teacherBindCount[b.teacher_id] = (teacherBindCount[b.teacher_id] || 0) + 1;
        });
        teacherTbody.innerHTML = teachers.map(t => `
            <tr>
                <td>${t.id}</td>
                <td>${t.username}</td>
                <td>${t.real_name}</td>
                <td>${teacherBindCount[t.id] || 0}</td>
            </tr>`).join('');

        // 加载班级列表
        const classes = await api('/teacher-class/all-classes');
        const classTbody = document.getElementById('class-tbody');
        classTbody.innerHTML = classes.map(c => `
            <tr>
                <td>${c.id}</td>
                <td>${c.grade}</td>
                <td>${c.class_name}</td>
                <td>${c.head_teacher}</td>
            </tr>`).join('');
    } catch (err) { console.error(err); }
}

document.getElementById('bind-add-btn')?.addEventListener('click', showBindModal);
document.getElementById('refresh-bind-btn')?.addEventListener('click', loadTeacherBind);

function showBindModal() {
    document.getElementById('modal-body').innerHTML = `
        <h3>绑定教师与班级</h3>
        <label>教师</label><select id="bind-teacher"><option value="">选择教师</option></select>
        <label>班级</label><select id="bind-class"><option value="">选择班级</option></select>
        <button onclick="submitBind()">绑定</button>`;
    document.getElementById('modal').style.display = 'flex';

    // 动态加载选项
    api('/teacher-class/all-teachers').then(teachers => {
        const select = document.getElementById('bind-teacher');
        teachers.forEach(t => {
            const opt = document.createElement('option');
            opt.value = t.id;
            opt.textContent = `${t.real_name} (${t.username})`;
            select.appendChild(opt);
        });
    });

    api('/teacher-class/all-classes').then(classes => {
        const select = document.getElementById('bind-class');
        classes.forEach(c => {
            const opt = document.createElement('option');
            opt.value = c.id;
            opt.textContent = `${c.grade} ${c.class_name}`;
            select.appendChild(opt);
        });
    });
}

async function submitBind() {
    const teacherId = parseInt(document.getElementById('bind-teacher').value);
    const classId = parseInt(document.getElementById('bind-class').value);
    if (!teacherId || !classId) { alert('请选择教师和班级'); return; }

    try {
        await api('/teacher-class/admin-bind', {
            method: 'POST',
            body: JSON.stringify({ teacher_id: teacherId, class_id: classId })
        });
        closeModal();
        loadTeacherBind();
    } catch (err) { alert(err.message); }
}

async function unbindTeacher(teacherId, classId) {
    if (!confirm('确定解绑？')) return;
    try {
        await api('/teacher-class/admin-unbind', {
            method: 'DELETE',
            body: JSON.stringify({ teacher_id: teacherId, class_id: classId })
        });
        loadTeacherBind();
    } catch (err) { alert(err.message); }
}

// ===== 模态框 =====
document.getElementById('modal-close').addEventListener('click', closeModal);
function closeModal() { document.getElementById('modal').style.display = 'none'; }
document.getElementById('modal').addEventListener('click', (e) => {
    if (e.target.id === 'modal') closeModal();
});

// ===== 查看学生记录 =====
function viewStudentRecords(sid) {
    document.querySelectorAll('.nav-item').forEach(n => n.classList.remove('active'));
    document.querySelector('[data-page="records"]').classList.add('active');
    document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
    document.getElementById('page-records').classList.add('active');
    document.getElementById('rec-student').value = sid;
    loadRecords();
}

// ===== AI 模型配置 =====
async function loadAIConfig() {
    if (userRole !== 'admin') {
        document.getElementById('ai-config-tbody').innerHTML = '<tr><td colspan="8" style="text-align:center;color:#999">仅管理员可访问</td></tr>';
        return;
    }
    try {
        const configs = await api('/ai/config');
        const tbody = document.getElementById('ai-config-tbody');
        tbody.innerHTML = configs.length === 0
            ? '<tr><td colspan="8" style="text-align:center;color:#999">暂无配置，请先添加一个 AI 模型</td></tr>'
            : configs.map(c => `
                <tr>
                    <td>${c.id}</td>
                    <td>${c.provider}</td>
                    <td>${c.model_name}</td>
                    <td><code>${c.api_key_masked}</code></td>
                    <td>${c.api_base || '默认'}</td>
                    <td>${c.is_active ? '<span style="color:#52c41a;font-weight:bold">✓ 激活</span>' : '<span style="color:#999">停用</span>'}</td>
                    <td>${new Date(c.created_at).toLocaleString('zh-CN')}</td>
                    <td>
                        ${c.is_active ? '' : `<button onclick="activateAIConfig(${c.id})">激活</button>`}
                        <button onclick="deleteAIConfig(${c.id})" style="color:#f5222d">删除</button>
                    </td>
                </tr>`).join('');
    } catch (err) { console.error('加载 AI 配置失败', err); }
}

document.getElementById('ai-add-btn')?.addEventListener('click', showAddAIConfigModal);

function showAddAIConfigModal() {
    document.getElementById('modal-body').innerHTML = `
        <h3>添加 AI 模型配置</h3>
        <label>服务商</label>
        <select id="ai-provider">
            <option value="deepseek">DeepSeek（推荐）</option>
            <option value="zhipu">智谱 GLM</option>
            <option value="mimo">小米 MiMo</option>
            <option value="qwen">通义千问</option>
            <option value="doubao">豆包</option>
            <option value="kimi">Kimi</option>
            <option value="openai">OpenAI</option>
        </select>
        <label>模型名称</label>
        <input id="ai-model" placeholder="deepseek-chat" value="deepseek-chat">
        <label>API Key</label>
        <input id="ai-key" type="password" placeholder="sk-...">
        <label>自定义 API 地址（可选，留空使用默认）</label>
        <input id="ai-base" placeholder="https://api.deepseek.com">
        <button onclick="submitAddAIConfig()">保存并激活</button>`;
    document.getElementById('modal').style.display = 'flex';

    // 选择服务商时自动填充默认 API 地址
    document.getElementById('ai-provider').addEventListener('change', (e) => {
        const bases = {
            deepseek: 'https://api.deepseek.com',
            zhipu: 'https://open.bigmodel.cn/api/paas/v4',
            mimo: 'https://token-plan-cn.xiaomimimo.com/v1',
            qwen: 'https://dashscope.aliyuncs.com/compatible-mode/v1',
            doubao: 'https://ark.cn-beijing.volces.com/api/v3',
            kimi: 'https://api.moonshot.cn/v1',
            openai: 'https://api.openai.com/v1'
        };
        document.getElementById('ai-base').value = bases[e.target.value] || '';
        const models = {
            deepseek: 'deepseek-chat',
            zhipu: 'glm-4-plus',
            mimo: 'mimo-v2.5-pro',
            qwen: 'qwen-max',
            doubao: 'doubao-1.5-pro-32k',
            kimi: 'moonshot-v1-32k',
            openai: 'gpt-4o'
        };
        document.getElementById('ai-model').value = models[e.target.value] || '';
    });
}

async function submitAddAIConfig() {
    const provider = document.getElementById('ai-provider').value;
    const model_name = document.getElementById('ai-model').value.trim();
    const api_key = document.getElementById('ai-key').value.trim();
    const api_base = document.getElementById('ai-base').value.trim() || null;

    if (!api_key) { alert('请填写 API Key'); return; }
    if (!model_name) { alert('请填写模型名称'); return; }

    try {
        await api('/ai/config', {
            method: 'POST',
            body: JSON.stringify({ provider, model_name, api_key, api_base })
        });
        closeModal();
        loadAIConfig();
    } catch (err) { alert('保存失败: ' + err.message); }
}

async function activateAIConfig(id) {
    try {
        await api(`/ai/config/${id}/activate`, { method: 'PUT' });
        loadAIConfig();
    } catch (err) { alert(err.message); }
}

async function deleteAIConfig(id) {
    if (!confirm('确定删除该 AI 配置？')) return;
    try {
        await api(`/ai/config/${id}`, { method: 'DELETE' });
        loadAIConfig();
    } catch (err) { alert(err.message); }
}

// ===== AI 报告管理 =====
async function loadAIReports() {
    const sid = document.getElementById('ai-rpt-student')?.value?.trim() || '';
    let path = '/ai/reports?page=1&page_size=100';
    if (sid) path += `&student_id=${encodeURIComponent(sid)}`;
    try {
        const reports = await api(path);
        const tbody = document.getElementById('ai-rpt-tbody');
        tbody.innerHTML = reports.length === 0
            ? '<tr><td colspan="8" style="text-align:center;color:#999">暂无报告，请先配置 AI 模型后生成</td></tr>'
            : reports.map(r => `
                <tr>
                    <td>${r.id}</td>
                    <td>${r.student_id}</td>
                    <td>${r.student_name || '-'}</td>
                    <td>${fmtDate(r.week_start)} ~ ${fmtDate(r.week_end)}</td>
                    <td title="${(r.report_summary || '').replace(/"/g, '&quot;')}">${(r.report_summary || '').slice(0, 40)}…</td>
                    <td>${r.provider} · ${r.model_name}</td>
                    <td>${new Date(r.created_at).toLocaleString('zh-CN')}</td>
                    <td><button onclick="viewAIReport(${r.id})">查看报告</button></td>
                </tr>`).join('');
    } catch (err) { console.error('加载 AI 报告失败', err); }
}

function fmtDate(iso) {
    if (!iso) return '-';
    return iso.slice(0, 10);
}

document.getElementById('ai-rpt-search-btn')?.addEventListener('click', loadAIReports);

document.getElementById('ai-rpt-gen-btn')?.addEventListener('click', async () => {
    const sid = document.getElementById('ai-rpt-student')?.value?.trim();
    if (!sid) { alert('请在左侧输入框填写学号，再点击生成'); return; }
    if (!confirm(`为学生 ${sid} 生成本周 AI 周报？`)) return;
    try {
        await api(`/ai/reports/generate/${encodeURIComponent(sid)}`, { method: 'POST' });
        alert('生成成功');
        loadAIReports();
    } catch (err) { alert('生成失败: ' + err.message); }
});

document.getElementById('ai-rpt-batch-btn')?.addEventListener('click', async () => {
    if (!confirm('为所有学生批量生成本周 AI 周报？（已有本周报告的学生会跳过）')) return;
    try {
        const result = await api('/ai/reports/generate-batch', { method: 'POST' });
        alert(result.message || '批量生成完成');
        loadAIReports();
    } catch (err) { alert('批量生成失败: ' + err.message); }
});

async function viewAIReport(reportId) {
    try {
        const r = await api(`/ai/reports/${reportId}`);
        const content = document.getElementById('modal-body');
        content.innerHTML = `
            <h3>AI 周报 — ${r.student_name || r.student_id}</h3>
            <p style="color:#888;font-size:13px;">周期：${fmtDate(r.week_start)} ~ ${fmtDate(r.week_end)} ｜ ${r.provider} · ${r.model_name}</p>
            <div style="margin-top:16px;padding:20px;background:#fafafa;border-radius:8px;border:1px solid #eee;max-height:75vh;overflow-y:auto;font-size:15px;line-height:1.9;">
                ${r.report_html || '<p style="color:#999">暂无报告内容</p>'}
            </div>`;
        document.getElementById('modal').style.display = 'flex';
    } catch (err) { alert('查看报告失败: ' + err.message); }
}

// ===== 初始化 =====
if (token) {
    api('/auth/me').then(user => {
        currentUser = { real_name: user.real_name, role: user.role };
        userRole = user.role;
        showApp();
    }).catch(() => logout());
}
