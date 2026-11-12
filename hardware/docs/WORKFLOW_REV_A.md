# Rev A academic PCB | Work plan

Status (2026-11-12): **schematics and PCB complete (ERC 0 errors, DRC 0 violations). Waiting for instructor review
before generating fabrication outputs.**

## G0 | Repo and Altium setup
- [x] Project in `hardware/altium/MP3_Player/MP3_Player.PrjPcb` with 7 schematic sheets and the board
- [x] Own library `MP3_RevA` linked to the project
- [x] Instructor library used locally and excluded from Git pending redistribution permission
- [x] Native project files committed and merged to `main` (PR #1 schematics, PR #2 PCB)

## G1 | Library and component evidence
- [x] Own symbols and footprints checked against datasheets, pin ↔ pad (see `altium/libraries/project/README.md`)
- [x] Course-library parts preferred; own parts justified (`GUIA_ESQUEMATICOS_RevA.md`, section 0.4)
- [x] Footprint fixes found by DRC: microSD and jack anchoring holes (non-plated), USB-C clearance exception
- [ ] BOM / part register with MPN, manufacturer, supplier and review status (4 own passives still `TBD`)
- [ ] Footprints confirmed against physical parts before soldering

## G2 | System architecture and floorplan
- [x] Interfaces and GPIO table (USB, I²S, SPI, I²C, buttons, ADC) — `GUIA_ESQUEMATICOS_RevA.md`, section 3
- [x] Two layers only (Top and Bottom), Bottom mainly GND plane
- [x] Board outline 65 × 90 mm (course limit 100 × 100 mm)
- [x] Antenna placed outside the board edge, as indicated by the instructor

## G3 | Schematic by blocks
- [x] Power: TP4056 charger, AN1149 load sharing (AP2301 + Schottky), AP2112K LDO, ADC dividers
- [x] MCU core: decoupling, EN reset RC, BOOT/RESET buttons, strapping and PSRAM pins left free, UART header
- [x] USB-C: CC pull-downs, USBLC6 ESD protection
- [x] Storage: SPI microSD with pull-ups and card detect
- [x] Audio: PCM5102A (TI Figure 33 values) → RC filter → TPA6138A2 (G = −1) → 3.5 mm jack
- [x] User interface: five buttons with pull-ups, OLED header with I²C pull-ups
- [x] Top level: hierarchical ports and sheet entries
- [x] ERC: 0 errors (remaining "no driving source" warnings suppressed with No ERC on reviewed nets)
- [ ] Power budget and battery autonomy estimate

## G4 | Board design
- [x] JLCPCB 2-layer rules with margin for hand soldering (see `hardware/README.md`)
- [x] Placement reviewed before routing; edge connectors, OLED area and button cross fixed first
- [x] Power routed first (VBUS, VBAT, VSYS, +3V3), then USB pair, local nets, buses and UI
- [x] GND polygon pours on Top and Bottom, stitching vias, no isolated GND islands
- [x] Silkscreen cleaned up (1.0 mm text, 0.15 mm stroke)
- [x] DRC: 0 violations, 0 warnings; 3D visual review
- [ ] Instructor review

## G5 | Academic deliverables
- [x] Native Altium project and own library
- [ ] PDFs of schematic and board-layer views; 3D captures
- [ ] BOM and design decision notes
- [ ] Exported ERC/DRC reports
- [ ] Fabrication outputs: Gerbers and drill file for JLCPCB, under `hardware/outputs/`
- [ ] Revision-tagged Git checkpoint (`rev-a`)

## Current known risks
- Battery cable polarity is not standard across JST-PH batteries: check before connecting.
- OLED module pin order varies by vendor: buy a GND–VCC–SCL–SDA module and check its silkscreen.
- AP2112K dropout leaves the last part of the battery charge unused.
- TP4056 dissipates up to ≈ 0.4 W while charging; copper around it acts as heatsink.
- USB-C (0.5 mm pitch) and TSSOP parts are the hardest joints to hand-solder.
- The antenna overhangs the 65 × 90 mm outline; check the overall size if an enclosure is made.
- DRC and ERC passing is not functional validation: nothing has been built or tested yet.

## Review discipline
Need → numerical/electrical requirement → alternatives → verified selection → Altium implementation → ERC/DRC checks → status (approved, provisional or pending).
Do not edit the separate PoC firmware during this workflow.
