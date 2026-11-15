# Rev A power budget and battery autonomy (estimate)

Status (2026-11-14): **estimate from datasheet typical values, not measured.** Validate on the assembled board
(see "How to validate" below) and update this file with the measured numbers.

## Power path recap

- On USB, `VSYS` comes from `VBUS` through the Schottky; the AP2301 P-MOSFET disconnects the battery from the load
  (Microchip AN1149), so the TP4056 charges the battery while the system runs from USB.
- On battery, `VSYS ≈ VBAT` and the AP2112K-3.3 LDO feeds `+3V3`. With a linear regulator the battery current is
  practically the same as the `+3V3` load current (plus 55 µA LDO quiescent current).
- Everything on the board runs from `+3V3`; nothing runs directly from `VBAT` except the ADC divider.

## Load budget on +3V3 (playback, Wi-Fi and Bluetooth off)

| Block | Part | Typical | Peak / worst case | Basis |
|---|---|---|---|---|
| MCU | ESP32-S3-WROOM-1-N16R8 | 40–60 mA | ~70 mA | Datasheet modem-sleep (radio off): 20–28 mA at 240 MHz with the CPU idle; MP3 decoding with both cores, flash and PSRAM access adds to that (estimate) |
| DAC | PCM5102A | 30 mA | 45 mA | TI SLAS859C, 3.3 V, 48 kHz, 1 kHz at −1 dBFS: DVDD 8 mA + AVDD/CPVDD 22 mA (max 13 + 32) |
| Headphone amplifier | TPA6138A2 | 14 mA | 25 mA | TI SLOS704B, supply current with no load (max 25 mA) |
| Headphone load | 2 × 32 Ω | ~4 mA | ~20 mA | ~1 mW per channel at listening level; worst case near the ~40 mW limit (firmware caps volume at about −12 dB) |
| Display | SSD1306 0.96" OLED module | ~10 mA | ~20 mA | Typical module figure, depends on lit pixels and contrast |
| Storage | microSD in SPI mode | ~10 mA average | ~100 mA burst | Reads are short bursts at MP3 bit rates; card-dependent |
| Regulator | AP2112K-3.3 | 0.055 mA | — | Diodes DS39724 quiescent current |
| Dividers and pull-ups | 390k/390k, 10k pull-ups | < 0.01 mA | — | VBAT divider ≈ 5 µA at 4.2 V; button pull-ups only draw while pressed |
| **Total** | | **≈ 110–120 mA** | **≈ 280 mA** | |

Screen off (or dimmed) and CPU at 160 MHz: **≈ 85–95 mA**.

Checks against the design:
- AP2112K rating 600 mA → worst case ≈ 280 mA leaves > 2× margin.
- LDO dissipation on battery: (4.2 V − 3.3 V) × 0.12 A ≈ 0.11 W typical, ≈ 0.25 W worst case. Acceptable for SOT-23-5
  with the copper pour around it.
- TP4056 charge current (R_PROG 4.7 kΩ) ≈ 255 mA. On USB the load is fed from `VBUS`, so charging and playback do not
  share the 255 mA.

## Battery autonomy

Usable capacity is taken as **85 %** of the rating: the AP2112K stops regulating 3.3 V when the battery falls to
about 3.5 V, and the cell protection cuts off near 3.0 V.

Autonomy ≈ capacity × 0.85 / current.

| Battery (1S Li-Po, protected) | Playback, screen on (≈ 115 mA) | Screen off, 160 MHz (≈ 90 mA) | Charge time at 255 mA |
|---|---|---|---|
| 1000 mAh (e.g. 803040) | ≈ 7.5 h | ≈ 9.5 h | ≈ 4.5–5 h |
| 1200 mAh (e.g. 603450) | ≈ 9 h | ≈ 11 h | ≈ 5–5.5 h |
| 2000 mAh (e.g. 103450) | ≈ 15 h | ≈ 19 h | ≈ 8.5–9 h |

Charge time ≈ capacity / 255 mA plus about 0.5–1 h of constant-voltage taper.

Firmware levers that extend autonomy: turn the OLED off after a few seconds without input, run the CPU at 160 MHz or
lower if decoding allows, keep Wi-Fi and Bluetooth off, and mute the amplifier (`AMP_MUTE_N`) and DAC (`DAC_XSMT`)
when paused.

## How to validate

1. Put a multimeter in series with the battery (mA range) and measure: idle on the menu, playback with screen on,
   playback with screen off, and paused.
2. Measure `+3V3` at the OLED header VCC pin under playback to confirm the LDO stays in regulation as the battery discharges.
3. Run a full playback test from a full charge and log the time until the firmware's low-battery cut-off.
4. Replace the estimates in this file with the measured values.

## Sources

- [TI PCM5102A datasheet (SLAS859C), specifications](https://www.ti.com/document-viewer/PCM5102A/datasheet/specifications)
- [TI TPA6138A2 datasheet (SLOS704B), specifications](https://www.ti.com/document-viewer/TPA6138A2/datasheet/specifications)
- [ESP32-S3-WROOM-1 datasheet (Espressif)](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- Diodes AP2112 datasheet (DS39724), quiescent current
