# Portable MP3 Player | Hardware Rev A

Custom PCB designed in **Altium Designer 26.11.1** for the university Electronic Design course.
Firmware proof-of-concept work stays under `poc/`.

**Status (2026-11-13): Rev A design frozen.** Schematics and PCB complete, including test points,
button labels and the Bottom logo. ERC: 0 errors. DRC: 0 violations. No further Altium changes are
planned; the next step is the instructor review, then fabrication outputs (Gerbers, drill, BOM).

## Course constraints

| Constraint | Rev A |
|---|---|
| Copper layers | 2 (Top and Bottom), Bottom used mainly as GND plane |
| Board size | ≤ 100 × 100 mm → final outline **65 × 90 mm** |
| Fabrication | JLCPCB |
| Assembly | Hand soldered by the students |
| Libraries | Instructor's `shared_lib` first; own parts only when justified |

## Architecture

| Block | Sheet | Main parts |
|---|---|---|
| Block diagram | `00_System_Top` | Hierarchical sheet entries between all blocks |
| Power | `10_Power_Management` | TP4056 charger (R_PROG 4.7 kΩ ≈ 255 mA), AP2301 P-MOSFET + Schottky load sharing (Microchip AN1149), AP2112K-3.3 LDO, 1S Li-Po on JST-PH, VBAT/2 and VBUS dividers to the ADC |
| MCU | `20_MCU_Core` | ESP32-S3-WROOM-1-N16R8, EN reset RC, BOOT and RESET buttons, UART header |
| USB | `30_USB_Interface` | USB-C 2.0 (5 V + data), 4.7 kΩ CC pull-downs, USBLC6-2SC6 ESD protection |
| Storage | `40_Storage` | microSD socket (GCT MEM2075) in SPI mode, 10 kΩ pull-ups, card detect |
| Audio | `50_Audio_Codec` | PCM5102A I²S DAC → 470 Ω / 2.2 nF filter → TPA6138A2 headphone amplifier (G = −1) → 3.5 mm jack |
| User interface | `60_User_Interface` | 5 buttons (active low, 10 kΩ pull-ups), SSD1306 128×64 I²C OLED module on a 4-pin header (GND, VCC, SCL, SDA) |

Power rails: `VBUS` (5 V USB), `VBAT` (3.0–4.2 V), `VSYS` (USB or battery), `+3V3` (logic).

## Component sources

- **Instructor library (`shared_lib`)**: USB-C, TP4056, AP2301, JST battery connector, Schottky diode,
  USBLC6, LED, push buttons, headers and all generic 0603 passives.
- **Own library (`MP3_RevA`)**: ESP32-S3 module, AP2112K, PCM5102A, TPA6138A2, microSD socket,
  audio jack, four 0603 values missing from the course library (470 Ω, 100 kΩ, 2.2 µF, 2.2 nF)
  and a single-pad test point (`TP_1.5mm`, not in the BOM).
  The reason for each own part is in [GUIA_ESQUEMATICOS_RevA.md](altium/GUIA_ESQUEMATICOS_RevA.md), section 0.4.

`shared_lib` is not published in this repository (license not confirmed). The schematic and PCB
documents embed every symbol and footprint, so the project opens without it.

## PCB

- Stack-up: FR-4, 1.6 mm, 1 oz copper on both layers.
- GND polygon pours on Top and Bottom, stitched with vias.
- ESP32-S3 antenna placed outside the board edge, as indicated by the instructor.
- Four M3 non-plated mounting holes (3.2 mm). The battery sits under the board.
- The OLED module is mounted above the board, between the ESP32 and the button cross.
- Test points TP1–TP17: 1.5 mm round SMD pads on Top, placed from the schematics, on the power
  rails (VBUS, VBAT, VSYS, +3V3, GND), the ADC dividers, EN and IO0, the I²S bus, the DAC output, the
  amplifier outputs and its MUTE line. Excluded from the BOM.
- Button silkscreen: `VOL+`, `VOL-`, `PLAY/PAUSE`, `PREV`, `NEXT`, plus `BOOT` and reset.
  The five user buttons share the same circuit (10 kΩ pull-up to an input), so their function is
  assigned in firmware; see the GPIO table in `GUIA_ESQUEMATICOS_RevA.md`, section 3.
- Logo (dinosaur, ~18.5 × 22 mm) on the Bottom Overlay, generated from
  `altium/MP3_Player/Logos/dino_mirrored.bmp` (already mirrored for the Bottom side). It is drawn
  as 5 mil silkscreen tracks grouped into a single union, placed in the lower-right area of the
  Bottom side, clear of pads, vias, mask openings and the battery, and ≥ 1 mm from the board edge.

Main design rules (JLCPCB 2-layer, with margin for hand soldering):

| Rule | Value |
|---|---|
| Clearance | 0.20 mm (0.15 mm inside the USB-C footprint only) |
| Track width | 0.25 mm signals, 0.60 mm power (0.40 mm minimum) |
| Vias | 0.30 mm hole / 0.60 mm diameter |
| Board outline clearance | 0.30 mm (0 mm for edge connectors and the ESP32) |
| Solder mask sliver | 0.10 mm (0.13 mm if black or white mask is ordered) |
| Silkscreen | 1.0 mm text height, 0.15 mm stroke |

## Repository layout

```text
hardware/
├── README.md
├── .gitignore
├── altium/
│   ├── GUIA_ESQUEMATICOS_RevA.md   # Step-by-step schematic guide and part justification
│   ├── MP3_Player/                 # Altium project: .PrjPcb, 7 .SchDoc sheets, .PcbDoc
│   │   └── Logos/                  # Bottom Overlay logo bitmap
│   └── libraries/
│       ├── project/                # Own library MP3_RevA (.SchLib, .PcbLib, .LibPkg)
│       └── external/               # Course/vendor libraries, ignored by Git
└── docs/
    └── WORKFLOW_REV_A.md
```

## Pending

- [ ] Instructor review of schematics and PCB
- [ ] Fabrication outputs: Gerbers, drill file and BOM (to be added under `hardware/outputs/`)
- [ ] Confirm battery cable polarity and OLED module pin order before soldering
- [ ] Power budget and battery autonomy estimate
- Not planned for Rev A: per-block frames on sheet 60 (sheets 00–50 have them)

## Git practice

Work on `hardware/rev-a-altium` and merge to `main` through pull requests at each milestone.
Do not modify `poc/` from hardware work. Preserve the original GPLv3 licensing.
