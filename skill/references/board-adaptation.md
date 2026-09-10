# Board Adaptation Workflow for OpenVela LVGL Apps

## Critical First Step: Check Actual Hardware

Design images can be misleading. Always verify display specs against the driver code.

### Finding screen resolution
```bash
# Search the board's LCD driver Kconfig
grep -r "LCD_HOR_RES_MAX\|LCD_VER_RES_MAX" vendor/<vendor>/boards/<chip>/drivers/lcd/Kconfig
```

Common resolutions found in practice:
| Board | Display | Resolution | Shape |
|-------|---------|------------|-------|
| SF32LB52-DevKit-LCD | CO5300 AMOLED | 390×450 | Rectangular |
| SF32LB52-LCHSPI-ULP | CO5300 AMOLED | 390×450 | Rectangular |
| lckfb_huangshan_pi | CO5300 AMOLED | 390×450 | Rectangular |

### Board directory structure
```
vendor/<vendor>/boards/<chip>/<board>/
├── README_zh-cn.md          # Hardware specs, GPIO map, limitations
├── Kconfig                  # Board-level config options
├── configs/
│   └── nsh/defconfig        # Default NSH config
│   └── <appname>/defconfig  # Custom app config (you create this)
├── drivers/
│   ├── lcd/                 # LCD driver (check Kconfig for resolution!)
│   └── input/               # Touch/keyboard driver
├── include/
│   └── board.h              # Clock, GPIO, MTD definitions
└── src/
    ├── bsp_pinmux.c         # Pin multiplexing
    ├── bsp_lcd_tp.c         # LCD power-on + touch reset
    └── etc/init.d/rcS       # Startup script
```

## Creating a Defconfig Variant

NEVER start from scratch. Copy and modify:

```bash
# Copy existing
cp -r vendor/<vendor>/boards/<chip>/<board>/configs/nsh \
      vendor/<vendor>/boards/<chip>/<board>/configs/<appname>

# Edit: add your app
echo "CONFIG_EXAMPLES_<APPNAME>=y" >> configs/<appname>/defconfig

# Edit: remove unused tests to save flash space
# Remove lines like:
#   CONFIG_TESTING_MM=y
#   CONFIG_TESTING_OSTEST=y
#   CONFIG_EXAMPLES_LSM6DSL_READER=y
#   CONFIG_BT=y (if not using Bluetooth)
```

## SIFLI SF32LB52 Specific Notes

### Flashing
```bash
sftool -c SF32LB52 -p /dev/ttyUSB0 -b 1000000 \
       --before default_reset --after soft_reset \
       write_flash cmake_out/<board>/nuttx.bin@0x12010000
```

### Serial console (CRITICAL)
```bash
# MUST use picocom with RTS deasserted
picocom -b 1000000 --noreset --lower-rts --lower-dtr /dev/ttyUSB0

# DO NOT use:
# - minicom (asserts RTS on open, resets SoC)
# - screen (no RTS control, holds SoC in reset)
# - cu (same issue as screen)
```

Why: SF32LB52 has no dedicated reset pin. The USB-UART bridge's RTS pin connects to a load switch that cuts VCC. Asserting RTS = hard reset.

### GPIO Pin Map (DevKit-LCD)
| Function | GPIO |
|----------|------|
| UART1 console RX/TX | PA18/PA19 |
| UART2 debug log RX/TX | PA20/PA27 |
| Touch I2C1 SDA/SCL | PA33/PA30 |
| Touch INT/RST | PA31/PA09 |
| QSPI LCD CS/CLK/TE | PA03/PA04/PA02 |
| LCD reset/VADD_EN | PA00/PA37 |
| Backlight PWM (GPTIM1_CH4) | PA01 |
| KEY1/KEY2 | PA34/PA11 |

### Network
No built-in WiFi/Ethernet. Options:
1. USB RNDIS: `CONFIG_RNDIS=y` + `CONFIG_NET=y` + `CONFIG_NET_TCP=y`
2. Design app to work offline (local simulation, queue uploads)
3. Add external WiFi module via SPI

### Kconfig Chain for Peripheral Drivers

The SF32LB52 board's `Kconfig` is **empty by default**. This means peripheral driver configs (touch, LCD) defined in `drivers/*/Kconfig` are invisible to the Kconfig system.

**Problem**: `CONFIG_INPUT_FT6146=y` in defconfig gets silently dropped by `olddefconfig`.

**Fix**: Board Kconfig must source the drivers Kconfig chain:

```kconfig
# vendor/sifli/boards/sf32lb52/sf32lb52_devkit_lcd/Kconfig
source "/opt/openvela_2026/vendor/sifli/boards/sf32lb52/drivers/Kconfig"
```

And `drivers/Kconfig` must use **absolute paths** (not `$BINDIR` which resolves to the cmake build directory):

```kconfig
# vendor/sifli/boards/sf32lb52/drivers/Kconfig
menu "SiFli On-Board Device Drivers"
source "/opt/openvela_2026/vendor/sifli/boards/sf32lb52/drivers/platform/lcd/Kconfig"
source "/opt/openvela_2026/vendor/sifli/boards/sf32lb52/drivers/platform/input/Kconfig"
endmenu
```

### Touch Screen (FT6146) Dependencies

The FT6146 Kconfig requires three dependencies:

```kconfig
config INPUT_FT6146
    depends on INPUT && I2C && INPUT_TOUCHSCREEN
```

The SiFli BSP enables I2C via `CONFIG_BSP_USING_I2C=y`, but this is NOT the same as the NuttX-level `CONFIG_I2C=y`. Both must be set:

```
CONFIG_I2C=y                    # NuttX I2C bus framework
CONFIG_BSP_USING_I2C=y          # SiFli BSP I2C driver
CONFIG_BSP_USING_I2C1=y         # SiFli BSP I2C1 peripheral
CONFIG_INPUT=y                  # NuttX input subsystem
CONFIG_INPUT_TOUCHSCREEN=y      # NuttX touchscreen framework
CONFIG_INPUT_FT6146=y           # FT6146 driver
CONFIG_TOUCH_IRQ_PIN=41         # PA31 touch interrupt
```

### Board-Level vs BSP Kconfig Symbols

| NuttX Symbol | SiFli BSP Symbol | Notes |
|---|---|---|
| `CONFIG_I2C` | `CONFIG_BSP_USING_I2C` | Both needed for I2C drivers |
| `CONFIG_SPI` | `CONFIG_BSP_USING_SPI` | Both needed for SPI drivers |
| `CONFIG_PWM` | `CONFIG_BSP_USING_PWM` | Both needed for PWM drivers |
