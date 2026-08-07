# Deploy script for fitness-server (Windows PowerShell)
# Usage: .\deploy.ps1

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Fitness Test Platform - Deploy Script" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan

# Check Docker
if (!(Get-Command docker -ErrorAction SilentlyContinue)) {
    Write-Host "Docker is not installed. Please install Docker Desktop first." -ForegroundColor Red
    exit 1
}

# Determine compose command
$composeCmd = "docker-compose"
if (!(Get-Command docker-compose -ErrorAction SilentlyContinue)) {
    $composeCmd = "docker compose"
}

# Check .env file
if (!(Test-Path .env)) {
    Write-Host "Creating .env config file..." -ForegroundColor Yellow
    Copy-Item .env.example .env
    Write-Host "Please edit .env to change passwords" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Building and starting services..." -ForegroundColor Green
Invoke-Expression "$composeCmd up -d --build"

Write-Host ""
Write-Host "Waiting for database to be ready..." -ForegroundColor Yellow
Start-Sleep -Seconds 15

Write-Host ""
Write-Host "Initializing database..." -ForegroundColor Green
try {
    Invoke-Expression "$composeCmd exec -T server python init_db.py"
} catch {
    Write-Host "Database init may need retry. Run: $composeCmd exec server python init_db.py" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "Deploy complete!" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "  API:        http://localhost:8000"
Write-Host "  Docs:       http://localhost:8000/docs"
Write-Host "  Web UI:     http://localhost:8000"
Write-Host "  MQTT UI:    http://localhost:18083"
Write-Host ""
Write-Host "  Admin:      admin / admin123"
Write-Host "  Teacher:    teacher / teacher123"
Write-Host ""
Write-Host "  Commands:"
Write-Host "    Logs:     $composeCmd logs -f server"
Write-Host "    Stop:     $composeCmd down"
Write-Host "    Restart:  $composeCmd restart"
Write-Host "============================================" -ForegroundColor Cyan
