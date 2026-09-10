# BES2800 WiFi & Sensor Integration Guide

## WiFi on BES2800

### Architecture
WiFi runs on APC1 core via RPMSG. AP core gets `wlan0` virtual network interface.
Defconfig already includes: `CONFIG_BES_WIFI_RPMSG_APC0C1=y`, `CONFIG_DRIVERS_IEEE80211=y`, `CONFIG_WIRELESS_WAPI=y`.

### wapi Command Syntax (CRITICAL: ALL parameters are NUMERIC)
```bash
# essid: flag is numeric, NOT string
wapi essid wlan0 "SSID" 1        # 0=OFF, 1=ON, 2=DELAY_ON
# WRONG: wapi essid wlan0 "SSID" on  → ERROR: Invalid option string: on

# psk: args are <iface> <password> <algorithm> — SSID is NOT a psk arg
wapi psk wlan0 "password" 3      # 0=NONE, 1=WEP, 2=TKIP, 3=CCMP
# WRONG: wapi psk wlan0 "SSID" "pass" 3  → SSID treated as password
# WRONG: wapi psk wlan0 "pass" CCMP      → ERROR: must be numeric
# WRONG: wapi psk wlan0 "pass" NONE      → ERROR: must be numeric

# Full connection sequence
wapi essid wlan0 "MyNetwork" 1   # ① Set SSID
wapi psk wlan0 "mypassword" 3    # ② Set password + WPA2
ifup wlan0                        # ③ Bring up interface
renew wlan0                       # ④ DHCP
```

### In-App WiFi Management
Use `popen()` to run `wapi` commands and parse output. Key pattern:
```c
static int run_cmd_capture(const char *cmd, char *buf, int buf_size) {
  FILE *fp = popen(cmd, "r");
  if (!fp) return -1;
  int len = fread(buf, 1, buf_size - 1, fp);
  pclose(fp);
  buf[len > 0 ? len : 0] = '\0';
  return len;
}
```

WiFi scan output may contain MAC addresses instead of SSIDs. Fallback: hardcode known networks.

### Server API (http://101.35.231.154)
- `GET /health` → `{"status":"ok"}`
- `POST /api/v1/auth/login` → `{"username":"...","password":"..."}` returns JWT
- `POST /api/v1/vitals` → vital signs upload (needs auth)
- `POST /api/v1/alarms` → alarm upload (needs auth)
- `POST /api/v1/devices` → device registration (needs auth)

## Sensor Integration (BMI270)

### Recommended: Bosch BMI270
- 6-axis IMU (accel + gyro), I2C/SPI
- Ultra-low power (<800μA), 2.5×3mm package
- Built-in step counter, fall detection, wrist-tilt detection
- NuttX has complete driver: `CONFIG_SENSORS_BMI270=y`
- I2C address: 0x68 (SDO→GND) or 0x69 (SDO→VCC)

### Wiring (BES1700 EVB — ACTUAL pin mapping)
⚠️ Previous docs said PA33/PA30 — WRONG for ZE7 EVB variant. Correct pins:
- **I2C1 SCL = G36** (GPIO_PIN36)
- **I2C1 SDA = G37** (GPIO_PIN37)
- **INT1 = G40** (GPIO_PIN40) — for hardware fall detection interrupt
- SDO → GND (I2C addr 0x68)
- I2C1 IOMUX_INDEX = 94 maps to G36/G37
- defconfig: `CONFIG_BES_I2C1=y`, `CONFIG_SENSORS=y`, `CONFIG_SENSORS_BMI270=y`, `CONFIG_SENSORS_BMI270_I2C=y`

### BMI270 Character Device Driver (NOT sensor framework)
The NuttX BMI270 driver (`bmi270.c`) registers as a **character device** `/dev/imu0`, NOT the sensor framework `/dev/sensor_accel0`. Data format:
```c
#include <nuttx/sensors/bmi270.h>
struct accel_gyro_st_s data;  // { struct accel_t accel; struct gyro_t gyro; uint32_t sensor_time; }
int fd = open("/dev/imu0", O_RDONLY);
read(fd, &data, sizeof(data));
// data.accel.x → int16_t raw, convert: raw * 9.81/16384 = m/s² (±2g mode)
// data.gyro.x → int16_t raw, convert: raw / 16.4 = °/s (±2000°/s mode)
```

### BMI270 Registration
Board init in library mode can't be modified. Enable `CONFIG_SENSORS_BMI270=y` in defconfig and let board_late_initialize() handle registration. Then just `open("/dev/imu0", O_RDONLY)`.
⚠️ `up_i2cinitialize()` declaration may not be found in headers. Don't call it from app code.

### GPIO includes for BES
```c
#include <arch/chip/bes_gpio.h>  // Provides GPIO_PIN36 etc.
// bes_i2c.h does NOT exist — don't include it
```

### Fall Detection Algorithm
Detect free-fall → impact pattern:
1. `accel_magnitude < 3.0 m/s²` → free-fall detected
2. Within 1.5s: `accel_magnitude > 25.0 m/s²` → impact detected → confirmed fall
3. 10s cooldown after each detection
4. Hardware interrupt from BMI270 INT1 pin (G40) provides faster detection

### Posture Recognition
Based on gravity vector direction (accel when stationary):
- **Standing**: Z ≈ 9.8, X≈0, Y≈0
- **Sitting**: Z ≈ 5-8
- **Lying flat**: Z ≈ 0, Y ≈ 9.8
- **Walking**: periodic accel + gyro > 30°/s
Use voting over 4 samples for stability.

### Sensor Stub Pattern
When sensor hardware is not connected, provide a stub `sensor_manager.c` that generates simulated data. This allows the full UI to be developed and tested without hardware.

## LVGL v9 Symbols Reference
Not all symbols exist. Known issues:
- `LV_SYMBOL_SLEEP` → does NOT exist, use `LV_SYMBOL_PAUSE`
- `LV_SYMBOL_HOME`, `LV_SYMBOL_PLAY`, `LV_SYMBOL_PAUSE`, `LV_SYMBOL_REFRESH` → confirmed working

## HTTP Upload on NuttX
Use POSIX sockets directly (not curl/system):
```c
int fd = socket(AF_INET, SOCK_STREAM, 0);
// DNS: gethostbyname() or inet_pton() for IP
// Standard connect/send/recv with SO_RCVTIMEO/SO_SNDTIMEO
// Connection: close after each request
```
