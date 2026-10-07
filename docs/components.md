# Components

Components and pin map for the current build.

| Component | Role |
|---|---|
| Arduino Uno R3 | Main controller, runs all sketch logic |
| 2x MG90S servo | Azimuth and elevation (load-bearing axes) |
| 2x SG90 servo | Clamshell cover and dawn wiper |
| 3x LDR (photoresistor) | Sun-position sensing — top-left (A0), top-right (A1), bottom (A2) |
| Resistor-divider voltage sensor | Battery voltage only, wired to A3 |
| 16x2 I2C LCD (address 0x27) | Status display — battery/USB on one screen, cover/day-night on the other |
| 4-channel relay module | Only channel 1 (D4) is used, for USB charge control. Channels 2-4 are unused |
| TP4056 charging module | Li-ion charge management |
| MT3608 boost converter | Regulates battery output to a stable 5V rail |
| Li-ion cell (single) | Primary power for logic/LCD/relay rail |
| AA battery packs (2x, 4 cells each) | Separate supply for the four servos (common ground with the Arduino) |
| 4x 70x70mm solar panel | Power source, wired in parallel with one inline diode per panel |

## Pin map

| Pin | Function | Note |
|---|---|---|
| D11 | Azimuth servo | |
| D12 | Elevation servo | |
| D5 | Cover servo | |
| D6 | Wiper servo | |
| D4 | Relay channel 1 — USB charge control | Active-LOW |
| D2, D3 | Relay channels 2, 3 | Reserved, unused |
| A0 | LDR top-left | |
| A1 | LDR top-right | |
| A2 | LDR bottom | |
| A3 | Voltage sensor | |
| A4 | I2C SDA (LCD) | Do not reuse for analog reads |
| A5 | I2C SCL (LCD) | Do not reuse for analog reads |

## Power architecture

The system runs on separate supplies, not one:

- **Logic rail** (Arduino, LCD, relay, voltage sensor): single Li-ion cell through
  TP4056 and an MT3608 boost converter.
- **Servo rail**: two separate 4-cell AA battery packs, each driving two servos, sharing
  a common ground with the Arduino so the servo signals work.

This is a deliberate engineering choice over a single-battery design. Running all
four servos off the same rail as the Arduino caused measurable voltage sag under load
(down to ~3.7V at worst) and brownout/reset loops on the Arduino — confirmed with an
incremental 1-to-4-servo load test, not assumed. Splitting the supply resolved it.
