# Local deployment script (no Docker needed)
# Usage: .\run_local.ps1

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$backendDir = Join-Path $scriptDir "backend"

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Fitness Test Platform - Local Deploy" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan

# 0. Start mosquitto MQTT broker
Write-Host ""
Write-Host "[0/3] Starting mosquitto MQTT broker..." -ForegroundColor Green
$mosq = Get-Process -Name "mosquitto" -ErrorAction SilentlyContinue
if (!$mosq) {
    $mosqPath = "C:\Program Files\mosquitto\mosquitto.exe"
    if (Test-Path $mosqPath) {
        Start-Process $mosqPath -ArgumentList "-c", "`"C:\Program Files\mosquitto\mosquitto.conf`"" -WindowStyle Hidden
        Start-Sleep -Seconds 2
        Write-Host "  mosquitto started on port 1883" -ForegroundColor Green
    } else {
        Write-Host "  mosquitto not found, MQTT disabled" -ForegroundColor Yellow
    }
} else {
    Write-Host "  mosquitto already running" -ForegroundColor Green
}

# 1. Install Python dependencies
Write-Host ""
Write-Host "[1/3] Installing Python dependencies..." -ForegroundColor Green
$env:PIP_INDEX_URL = "https://pypi.org/simple/"
pip install -r (Join-Path $backendDir "requirements.txt") -q
pip install aiosqlite "bcrypt==4.2.0" -q
Write-Host "  Dependencies installed." -ForegroundColor Green

# 2. Init database (only if not exists)
$dbFile = Join-Path $backendDir "fitness.db"
if (Test-Path $dbFile) {
    Write-Host ""
    Write-Host "[2/3] Database already exists, skipping init." -ForegroundColor Yellow
} else {
    Write-Host ""
    Write-Host "[2/3] Initializing database (SQLite)..." -ForegroundColor Green
    Push-Location $backendDir
    python init_db.py
    Pop-Location
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  Database init failed!" -ForegroundColor Red
        exit 1
    }
}

# 3. Start server
Write-Host ""
Write-Host "[3/3] Starting server..." -ForegroundColor Green
Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Server: http://localhost:8000" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "  Web UI:  http://localhost:8000"
Write-Host "  Docs:    http://localhost:8000/docs"
Write-Host "  MQTT:    localhost:1883"
Write-Host ""
Write-Host "  Admin:   admin / admin123"
Write-Host "  Teacher: teacher / teacher123"
Write-Host ""
Write-Host "  Press Ctrl+C to stop."
Write-Host ""

Set-Location $backendDir
python -m uvicorn main:app --host 0.0.0.0 --port 8000
