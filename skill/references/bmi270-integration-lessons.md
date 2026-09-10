# BES2800 BMI270 Integration — Lessons Learned (2026-08-31)

## BMI270 Registration — CAN Be Done From App Layer

### BES I2C API (exports from BSP library)
```c
// These ARE exported by libbes1700_evb_ap.a:
extern struct i2c_master_s *bes_i2cbus_initialize(int bus);
extern void hal_iomux_set_i2c1(void);

// This is NOT exported:
// extern struct i2c_master_s *up_i2cinitialize(int bus);  // LINKER ERROR
```

### I2C Bus Availability
- **I2C0** (`bes_i2cbus_initialize(0)`) ✅ Works — shared with TMA525C touch (addr 0x20)
- **I2C1** (`bes_i2cbus_initialize(1)`) ❌ Returns NULL — even after `hal_iomux_set_i2c1()`

### Registration Code (in sensor_manager.c)
```c
// Try I2C1 first, fall back to I2C0
struct i2c_master_s *i2c = bes_i2cbus_initialize(1);
if (i2c == NULL) {
    i2c = bes_i2cbus_initialize(0);  // I2C0 always works
}

// Try both I2C addresses
int ret = bmi270_register("/dev/imu0", i2c, 0x68);  // SDO→GND
if (ret < 0) {
    ret = bmi270_register("/dev/imu0", i2c, 0x69);  // SDO→VCC
}
```

### Error Codes
- `-19` (ENODEV): BMI270 chip not responding on I2C bus — wrong address or wiring issue
- `errno=2` (ENOENT): `/dev/imu0` device not registered

### Wiring for I2C0 (Recommended)
```
BMI270 SCL  → P04 (PA04, I2C0 SCL, shared with touch)
BMI270 SDA  → P05 (PA05, I2C0 SDA, shared with touch)
BMI270 INT1 → G40 (GPIO_PIN40)
BMI270 VCC  → 3.3V
BMI270 GND  → GND
BMI270 SDO  → GND (addr 0x68) or VCC (addr 0x69)
```

### BMI270 Data Format
```c
#include <nuttx/sensors/bmi270.h>

struct accel_gyro_st_s data;
int fd = open("/dev/imu0", O_RDONLY);
read(fd, &data, sizeof(data));
// data.accel.x/y/z = int16_t raw LSB
// data.gyro.x/y/z  = int16_t raw LSB

// Conversion (±2g mode: 16384 LSB/g, ±2000°/s: 16.4 LSB/°/s)
float accel_ms2 = data.accel.x * (9.81f / 16384.0f);
float gyro_dps  = data.gyro.x * (1.0f / 16.4f);
```

## data_collector_init() Must Be Called Explicitly

In `elderly_main.c`, after `ui_manager_init()`:
```c
data_collector_init();  // Initializes sensor_manager, WiFi, HTTP
```
Without this call, sensor_manager and WiFi never initialize — no error, just silence.

## WiFi Connection — Exact wapi Sequence

```bash
# ALL wapi params are NUMERIC indices, not strings
wapi essid wlan0 "RD2" 1       # flag: 0=OFF, 1=ON (NOT "on")
wapi psk wlan0 "123qwe##" 3    # alg: 0=NONE, 1=WEP, 2=TKIP, 3=CCMP
ifup wlan0
renew wlan0
```

**Critical**: `wapi psk` args are `<iface> <password> <alg>` — NO SSID. SSID is set via `wapi essid`.

WiFi subsystem takes 5-10s after boot to be ready. Always `ifup wlan0` first.

## HTTP Upload via POSIX Sockets

Server: `http://101.35.231.154`
- `GET /health` → `{"status":"ok"}`
- `POST /api/v1/vitals` → vital signs
- `POST /api/v1/alarms` → alarm events
- `POST /api/v1/auth/login` → login (needs credentials)

Build HTTP/1.1 POST manually with `socket()` / `connect()` / `send()` / `recv()`.
