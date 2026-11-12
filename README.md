# Portable MP3 Player

**Embedded systems · Electronic design · ESP-IDF · C**

A university engineering project exploring a dedicated, offline music player: local audio files, physical controls, and a focused interface without streaming services or smartphone dependency.

> **Status:** firmware proof of concept (PoC) in progress, and custom PCB **Rev A designed** (schematics and layout complete, ERC/DRC clean, awaiting instructor review). File-catalog logic has host tests and recorded firmware-build evidence. Physical operation, audio playback, and the custom hardware are **not yet validated**.

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

Board: 2 layers, 65 × 90 mm, designed for JLCPCB and hand soldering. Details in **[hardware/README.md](hardware/README.md)**.

**Important platform distinction:** The ESP32-S31-Korvo-1 is a temporary PoC board. It uses an ESP32-S31, ES8389 audio codec, and SDMMC storage interface. Results obtained with this board do **not** validate the ESP32-S3, PCM5102A/TPA6138A2, or SPI-based architecture of the custom PCB.

## What exists today

**Implemented in the PoC source**
- Request for SD-card mounting through the Korvo board support package.
- Discovery, filtering, and deterministic sorting of `.mp3` filenames under `MUSIC`.
- UART diagnostic output and explicit error handling.

**Evidence available**
- Host tests covering catalog behavior and boundary cases.
- Recorded ESP-IDF firmware-build results for the temporary Korvo target.

**Not yet demonstrated**
- SD-card operation on physical hardware.
- MP3 decoding or audio playback (including PCM/WAV paths).
- OLED and physical controls.
- USB MSC, Bluetooth, or a fabricated custom PCB.

Some audio/player interfaces are currently placeholders, not completed playback implementations.

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

1. Validate SD mounting and the catalog on a physical ESP32-S31-Korvo-1.
2. Establish and test an audio path on the PoC board.
3. ~~Complete the target schematic and PCB layout~~ (Rev A done); add the power budget and fabrication outputs.
4. Fabricate, assemble, and validate the custom PCB separately from the Korvo.
5. Implement final-device firmware, then assess USB MSC and BM83 integration.

## License

Licensed under the **GNU General Public License v3.0**. See [LICENSE](LICENSE).
