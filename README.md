# Portable MP3 Player

**Embedded systems · Electronic design · ESP-IDF · C**

A university engineering project exploring a dedicated, offline music player: local audio files, physical controls, and a focused interface without streaming services or smartphone dependency.

> **Status: Proof of concept (PoC) in progress.** File-catalog logic has host tests and recorded firmware-build evidence. Physical operation, audio playback, and the custom hardware are **not yet validated**.

## Project goals

- Browse and play music stored on a microSD card.
- Display basic track information and provide physical button controls.
- Explore USB mass-storage access and optional Bluetooth connectivity.
- Design a dedicated, battery-powered hardware platform.

These are **design goals**, not a claim that every feature is implemented.

## Proposed hardware architecture

| Subsystem | Selected / proposed approach | Validation status |
| --- | --- | --- |
| Microcontroller | ESP32-S3-WROOM-1-N16R8 | Target selection; integration pending |
| Audio | ES8388 codec via I²S; 3.5 mm headphone output | Proposed; not physically verified |
| Storage | microSD over SPI with planned FAT32 support | Proposed; PoC board uses a different interface |
| User interface | SSD1306 OLED and five physical buttons | Proposed; not implemented |
| Power | Li-Po, MCP73871, TPS63802 | Conceptual selection; circuit and power budget pending |
| USB | USB-C and proposed USB MSC | Pending implementation |
| Wireless | BM83 Bluetooth module | Optional; requires separate feasibility validation |

**Important platform distinction:** The ESP32-S31-Korvo-1 is a temporary PoC board. It uses an ESP32-S31, ES8389 audio codec, and SDMMC storage interface. Results obtained with this board do **not** validate the proposed ESP32-S3, ES8388, or SPI-based production architecture.

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
poc/
└── esp32-s31-korvo-1/
    ├── main/         # PoC firmware and music catalog
    ├── tests/host/   # Host-side catalog tests
    ├── docs/         # Status, validation notes, and evidence
    └── tools/        # PoC-specific development utilities
```

Dedicated hardware design files and final firmware are **not yet versioned**; they will be added only when corresponding engineering artifacts exist.

## Next milestones

1. Validate SD mounting and the catalog on a physical ESP32-S31-Korvo-1.
2. Establish and test an audio path on the PoC board.
3. Complete the target schematic, interface definitions, and power budget.
4. Prototype and validate custom hardware separately from the Korvo.
5. Implement final-device firmware, then assess USB MSC and BM83 integration.

## License

Licensed under the **GNU General Public License v3.0**. See [LICENSE](LICENSE).
