# Portable MP3 Player

**Embedded systems · Electronic design · ESP-IDF · C**

A university engineering project exploring a dedicated, offline music player: local audio files, physical controls, and a focused interface without streaming services or smartphone dependency.

> **Status:** firmware proof of concept (PoC) in progress, and custom PCB **Rev A design frozen** (schematics and layout complete with test points, ERC/DRC clean, reviewed by the instructor). On the ESP32-S31-Korvo-1 PoC board, SD storage is validated, WAV and MP3 files play audibly from the microSD card, and the board works as a player with buttons (play/pause, next/previous, limited volume) and an orderly shutdown on a simulated low battery. The display UI is pending, and the custom hardware (PCB Rev A) is **not yet validated**.

## Project goals

- Browse and play music stored on a microSD card.
- Display basic track information and provide physical button controls.
- Explore USB mass-storage access and optional Bluetooth connectivity.
- Design a dedicated, battery-powered hardware platform.

These are **design goals**, not a claim that every feature is implemented.

## Hardware architecture (PCB Rev A)

| Subsystem | Rev A design | Validation status |
| --- | --- | --- |
| Microcontroller | ESP32-S3-WROOM-1-N16R8 | In schematic and layout; not built |
| Audio | PCM5102A I²S DAC + TPA6138A2 headphone amplifier; 3.5 mm jack | In schematic and layout; not built |
| Storage | microSD over SPI | In schematic and layout; PoC board uses a different interface |
| User interface | SSD1306 OLED module and five physical buttons | In schematic and layout; not implemented in firmware |
| Power | 1S Li-Po, TP4056 charger, load sharing (AP2301), AP2112K 3.3 V LDO | In schematic and layout; power budget pending |
| USB | USB-C 2.0 (power and data), USBLC6 ESD protection; USB MSC planned | In schematic and layout; firmware pending |
| Wireless | BM83 Bluetooth module | Deferred; not part of Rev A |

Board: 2 layers, 65 × 90.17 mm, designed for JLCPCB and hand soldering. Details in **[hardware/README.md](hardware/README.md)**.

**Important platform distinction:** The ESP32-S31-Korvo-1 is a temporary PoC board. It uses an ESP32-S31, ES8389 audio codec, and SDMMC storage interface. Results obtained with this board do **not** validate the ESP32-S3, PCM5102A/TPA6138A2, or SPI-based architecture of the custom PCB.

## What exists today

**Implemented in the PoC source**
- SD-card mounting through the Korvo board support package.
- Discovery, filtering, and deterministic sorting of `.mp3` filenames under `MUSIC`.
- Audio output through the Korvo BSP route (ES8389 codec, standard I²S).
- Playback of 16-bit PCM WAV files and MP3 files (minimp3) from the microSD card.
- A player that ties the `MUSIC` catalog to MP3 playback, driven by the board buttons (play/pause, next/previous, limited volume).
- Orderly shutdown (mute, close file and codec) triggered by a simulated low battery.
- UART diagnostic output and explicit error handling.

**Evidence available**
- Host tests covering the catalog, WAV parsing, the MP3 decoder wrapper, and supported sample rates.
- ESP-IDF firmware-build results for the temporary Korvo target.
- On the physical ESP32-S31-Korvo-1: SD scenarios SD-01 to SD-07 validated (SD-08 deferred); audible test tone; WAV and MP3 files played from the microSD card, with timing logs.

**Not yet demonstrated**
- On-screen UI (OLED).
- Real battery measurement and a real power cut (only a simulated battery and an orderly firmware stop on the Korvo).
- Sample rates other than 8, 16, 44.1, and 48 kHz on the Korvo (a limitation of its codec setup).
- USB MSC, Bluetooth, or a fabricated custom PCB (Rev A hardware not validated).

For reproducible evidence and caveats, see the **[PoC status report](poc/esp32-s31-korvo-1/docs/POC_STATUS.md)**. For setup and build instructions, see the **[PoC README](poc/esp32-s31-korvo-1/README.md)**.

## Repository layout

```text
hardware/
├── altium/
│   ├── MP3_Player/   # Altium project: 7 schematic sheets and the PCB
│   └── libraries/    # Own component library (MP3_RevA)
└── docs/             # Rev A work plan
poc/
└── esp32-s31-korvo-1/
    ├── main/         # PoC firmware and music catalog
    ├── tests/host/   # Host-side catalog tests
    ├── docs/         # Status, validation notes, and evidence
    └── tools/        # PoC-specific development utilities
```

Final-device firmware is **not yet versioned**; it will be added only when the corresponding engineering artifacts exist.

## Next milestones

1. ~~Build a player with the catalog and physical buttons on the Korvo (E5)~~.
2. ~~Simulate a low-battery cutoff on the PoC board (E7)~~.
3. Next: port the firmware to PCB Rev A (microSD over SPI, PCM5102A).
4. ~~Complete the target schematic and PCB layout~~ (Rev A frozen and reviewed); ~~power budget~~ ([estimate](hardware/docs/POWER_BUDGET_REV_A.md)); ~~generate fabrication outputs~~ (`hardware/outputs/`).
5. Fabricate, assemble, and validate the custom PCB separately from the Korvo.
6. Implement final-device firmware, then assess USB MSC and BM83 integration.

## License

Licensed under the **GNU General Public License v3.0**. See [LICENSE](LICENSE).
