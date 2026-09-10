# wapi Command Reference — Correct Syntax (Learned 2026-08-25)

## CRITICAL: ALL wapi parameters are NUMERIC

The `wapi` tool rejects string parameter names. All flags/algorithms must be numeric indices.

### essid command
```bash
wapi essid <iface> <ssid> <flag>
# flag: 0=OFF, 1=ON, 2=DELAY_ON
# Example:
wapi essid wlan0 "MyNetwork" 1
```

### psk command
```bash
wapi psk <iface> <password> <algorithm>
# algorithm: 0=NONE, 1=WEP, 2=TKIP, 3=CCMP
# argv[0]=iface, argv[1]=password(8-63 chars), argv[2]=algorithm
# SSID is NOT an argument to psk — set it with essid first
# Example:
wapi psk wlan0 "mypassword123" 3
```

### Full connection sequence
```bash
wapi essid wlan0 "RD2" 1           # ① Set SSID
wapi psk wlan0 "123qwe##" 3        # ② Set password + CCMP(WPA2)
ifup wlan0                          # ③ Bring up interface
renew wlan0                         # ④ DHCP
```

### Common mistakes
```bash
# WRONG: "on" is not numeric
wapi essid wlan0 "SSID" on          # → ERROR: Invalid option string: on

# WRONG: "CCMP" is not numeric
wapi psk wlan0 "pass" CCMP          # → ERROR: Invalid option string: CCMP

# WRONG: SSID is not a psk argument
wapi psk wlan0 "SSID" "pass" 3      # → ERROR: password too short (SSID treated as password)

# WRONG: "NONE" is not numeric
wapi psk wlan0 "pass" NONE          # → ERROR: Invalid option string: NONE
```

### Scan
```bash
wapi scan wlan0                     # Trigger scan
wapi scan_results wlan0             # Get results
# Note: scan output may contain MAC addresses instead of SSIDs
# Workaround: hardcode known network SSIDs as fallback
```

### Other useful commands
```bash
wapi show wlan0                     # Show current config
wapi disconnect wlan0               # Disconnect
wapi power_save wlan0 1             # Enable power save
```
