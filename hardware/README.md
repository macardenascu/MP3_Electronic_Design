# Portable MP3 Player | Hardware Rev A

This directory is for the custom PCB designed in **Altium Designer 26.11.1** for the university Electronic Design course. Firmware proof-of-concept work stays under `poc/`.

## Academic constraints

- **Two copper layers maximum**: Top and Bottom, no internal copper layers.
- **PCB outline no larger than 100 × 100 mm**.
- Primary deliverable: a documented and reviewable schematic/PCB. Physical fabrication, assembly, and functional tests have **not** been confirmed as grading requirements.
- Functional plausibility still matters: no invented power values, datasheet pins, footprints, or test results.

## Architecture in scope

- ESP32-S3-WROOM-1-N16R8
- ES8388 audio codec and wired 3.5 mm stereo headphones
- SPI microSD
- SSD1306 OLED and five physical buttons
- One-cell Li-Po; proposed MCP73871 power-path charger and TPS63802 buck-boost
- USB-C power/data

The BM83 Bluetooth module is **deferred for the academic board proposal**, pending acceptance of the scope change. Its feasibility for a later revision is documented separately; it must not block wired audio.

All parts remain conceptual selections until electrical, footprint, and interface review.

## Working locations

```text
hardware/
├── README.md
├── .gitignore
├── altium/
│   ├── project/           # Native .PrjPcb, .SchDoc, .PcbDoc files (added from local Altium)
│   └── libraries/
│       ├── project/       # Owned/verified schematic + PCB libraries
│       └── external/      # Course/vendor libraries, ignored until licensing is checked
└── docs/
    └── WORKFLOW_REV_A.md
```

**Current repository status:** only the version-control scaffold is present. Native Altium design files visible on the local computer have **not** been uploaded or validated.

The `shared_lib.IntLib` supplied by the instructor is useful as a reference, but do not publish its binary source in this public repository without confirming redistribution permission. Keep it locally in `hardware/altium/libraries/external/`.

See [WORKFLOW_REV_A.md](docs/WORKFLOW_REV_A.md) for checkpoints and acceptance criteria.

## Git practice

Work on `hardware/rev-a-altium`, keep commits small and reviewable, and avoid modifying `poc/`. Check `git status` and the current branch before every commit. Preserve the original GPLv3 licensing.
