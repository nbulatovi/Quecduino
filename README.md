# Quecduino

Arduino library for building LTE-M/NB-IoT prototypes with Quectel LPWA modules.

Get a GPS fix, publish it over MQTT, and go to sleep — in under 60 lines of code.

## What it does

The included **TrackerDemo** builds a complete low-power GPS tracker:

1. Download AGNSS aiding data for fast time-to-first-fix
2. Acquire GPS position (typically < 10s with AGNSS)
3. Publish location as JSON over MQTT
4. Enter PSM/eDRX sleep to conserve battery
5. Wake up and repeat

## Quick start

### MCU board

- Arduino dev board (ESP-WROOM-32 was used for testing)
- Quectel LPWA TE-B evaluation board (see [supported modules](#supported-modules))
- Active LTE-M SIM card
- [Arduino CLI](https://arduino.github.io/arduino-cli/) or Arduino IDE

### Wiring (ESP-WROOM-32)

```
ESP32          Quectel TE-B
─────────────────────────
D25      →     RXD
D26      →     TXD
D27      →     DTR
D33      →     PON_TRIG
GND      →     GND
```

On the Quectel TE-B, set **PON_TRIG → OFF** and **AUTO_POWER_ON → ON**.

### Receive messages

Subscribe to the MQTT topic from any machine:

```bash
mosquitto_sub -h broker.emqx.io -t "lpwa/tracker/location" -v
```

## Configuration

All settings have sensible defaults. Override them before calling `configure()`:

```cpp
LPWA.config.module = BG950A;

// Network bands (default: US bands 2,4,5,12,13,14,25,26 + EU band 20)
LPWA.config.catm_bands  = "308381A";

// Power saving
LPWA.config.psm_t3324   = "00000010";   // Active timer: 2s
LPWA.config.psm_t3412   = "00100001";   // Periodic TAU: 1h
LPWA.config.edrx_cycle  = "1100";       // eDRX cycle: ~22 min
LPWA.config.edrx_ptw    = "0011";       // Paging window: 5.12s
```

## Other boards

The library uses `HardwareSerial` and standard `digitalWrite`/`digitalRead`. Any board with a free UART and 3.3V logic should work. Pass your pin assignments to `LPWA.begin()`.

## Supported modules

| Module | Form factor | Technologies | GNSS | Notes |
|--------|------------|--------------|------|-------|
| BG770A | Ultra-compact | Cat M1/NB2 | Yes | |
| BG772A | Ultra-compact | Cat M1/NB2 | Yes | |
| BG773A | Ultra-compact | Cat M1/NB2 | Yes | |
| BG950A | Compact | Cat M1/NB1/NB2/GPRS | Yes | iSIM |
| BG952A | Compact | Cat M1/NB1/NB2/GPRS | Yes | iSIM |
| BG953A | Compact | Cat M1/NB1/NB2/GPRS | Yes | |
| BG955A | Compact | Cat M1/NB1/NB2/GPRS | Yes | |
| BG770S | Ultra-compact | Cat M1/NB2 | Yes | ALT1350 |
| BG950S | Compact | Cat M1/NB2 | Yes | ALT1350 |
