# Board Adaptation Reference — BES2800 (best1700_ep)

## BES2800BP Board Specifications

| Item | Spec |
|------|------|
| Chip | BES2800BP (Cortex-M55 + HiFi4 DSP) |
| SDK codename | `best1700_ep` |
| Display | RM69330 DSI round AMOLED, 454×454 pixels |
| Touch | TMA525C capacitive touch |
| Network | Built-in Wi-Fi |
| Console | USB serial (921600 baud) |
| Vendor path | `vendor/bes/boards/best1700_ep/aos_evb/` |

### Board directory structure (BES)
```
vendor/bes/
├── boards/
│   ├── best1700_ep/
│   │   ├── aos_evb/
│   │   │   ├── configs/
│   │   │   │   ├── ap/defconfig        # AP core config
│   │   │   │   └── apc1/defconfig      # APC1 core config
│   │   │   ├── include/board_bes1700_evb.h  # GPIO, LCD resolution
│   │   │   └── src/ap.c
│   │   └── common/
│   └── common/
├── chips/bes/                            # Chip-level code (prebuilt libs)
└── drivers/
```

### Screen resolution source
Found in `vendor/bes/boards/best1700_ep/aos_evb/include/board_bes1700_evb.h`:
```c
#if defined(CONFIG_BES_LCD_RM69330)
#define BOARD_LCDC_WIDTH               454
#define BOARD_LCDC_HEIGHT              454
#endif
```

### Build command
```bash
cd /opt/openvela_2026
./build.sh vendor/bes/boards/best1700_ep/aos_evb/configs/ap --cmake
```

Output: `cmake_out/bes1700_ap/nuttx.bin`

### Serial console
```bash
picocom -b 921600 /dev/ttyACM0
```

### LVGL fonts in defconfig
BES1700 AP defconfig includes: Montserrat 20/22/24/28/32/36/38/40/48 (not just 14).

### Key defconfig entries for LVGL + elderly app
```
CONFIG_GRAPHICS_LVGL=y
CONFIG_LV_USE_NUTTX=y
CONFIG_LV_USE_NUTTX_TOUCHSCREEN=y
CONFIG_LV_COLOR_DEPTH_32=y
CONFIG_LV_MEM_SIZE_KILOBYTES=2048
CONFIG_BES_LCD_RM69330=y
CONFIG_BES_TP_TMA525C=y
CONFIG_INPUT=y
CONFIG_INPUT_TOUCHSCREEN=y
CONFIG_NET=y                # WiFi built-in
CONFIG_NET_TCP=y
```

### Known board peripherals
| Peripheral | Driver | Device node |
|-----------|--------|-------------|
| RM69330 AMOLED | BES LCD driver | `/dev/lcd0` |
| TMA525C touch | BES touch driver | `/dev/input0` |
| Battery ADC | BES ADC | `/dev/adc0` |
| RTC | BES RTC | `/dev/rtc0` |

See `references/board-adaptation.md` for other boards (SF32LB52).
