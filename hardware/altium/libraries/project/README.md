# MP3_RevA — own Altium library

Components the course library (`shared_lib.IntLib`) does not have, or does not have in a usable form.
Everything else on the board comes from the course library (see `hardware/README.md`).
The reason for each own part is in [GUIA_ESQUEMATICOS_RevA.md](../../GUIA_ESQUEMATICOS_RevA.md), section 0.4.

Files: `MP3_RevA.SchLib`, `MP3_RevA.PcbLib`, `MP3_RevA.LibPkg` (open it and run
*Project → Compile Integrated Library* to get `MP3_RevA.IntLib`).

**Status of every part: pinout and land pattern taken from the datasheet below and
cross-checked symbol pin ↔ footprint pad (all match). Not built or tested.**
Every IC/connector footprint has an embedded manufacturer STEP model.

## Parts used on the Rev A board

| Symbol | Footprint | Source (revision) | Notes |
|---|---|---|---|
| ESP32-S3-WROOM-1-N16R8 | ESP32-S3-WROOM-1 | Espressif datasheet v1.8, Table 3-1, Fig. 11-1 | EPAD = one pad (pin 41), checked against the datasheet land pattern; GND vias added on the board. IO35–IO37 used by the PSRAM. Antenna overhangs the board edge. |
| AP2112K-3.3TRG1 | SOT-23-5 | Diodes DS39724 Rev. 2-2, SOT25 suggested pad layout | 600 mA LDO. Chosen over the course AMS1117 because of dropout on battery. |
| PCM5102APWR | TSSOP-20_4.4x6.5mm_P0.65mm | TI SLAS859C, pin table and PW0020A board layout | I²S DAC, hardware control only. Support parts follow Figure 33. |
| TPA6138A2PWR | TSSOP-14_4.4x5mm_P0.65mm | TI SLOS704B, pin table and PW0014A board layout | Ground-centred outputs, no coupling capacitors. UVP left unconnected (internal pull-up). |
| MEM2075-00-140-01-A | MicroSD_GCT_MEM2075-00-140-01-A | GCT drawing rev A1 | Card-detect switch closes to the shell. NP1/NP2 are non-plated Multi-Layer holes (fixed 2026-11-11). |
| SJ-3523-SMT | Jack_3.5mm_SameSky_SJ-3523-SMT | Same Sky datasheet 04/15/2025 (rev 1.07) | Pin 1 sleeve, 2 tip (L), 3 ring (R). Pin 4 not populated on this variant. NP1/NP2 are non-plated Multi-Layer holes (fixed 2026-11-11). |
| TestPoint | TP_1.5mm | Own design | One 1.5 mm round SMD pad on Top, mask opening. Component type *Standard (No BOM)*. Used for TP1–TP17. |

### 0603 passives missing from the course library

Copies of the course templates (`Res_1k_0603_1/10W`, `Cap_1uF_0603_50V`, `Cap_1nF_0603_50V`) with the same
`0603_res` / `0603_cap` footprint and 3D model. Manufacturer data is `TBD` until a part number is chosen.

| Symbol | Value | Used for |
|---|---|---|
| Res_470R_0603_1/10W | 470 Ω | DAC output RC filter |
| Res_100k_0603_1/10W | 100 kΩ | Amplifier MUTE pull-down |
| Cap_2.2uF_0603_10V | 2.2 µF | DAC charge pump (CAPP–CAPM, VNEG) |
| Cap_2.2nF_0603_50V | 2.2 nF | DAC output RC filter |

## Parts kept in the library but not used (retired in schematic rev 2)

Replaced by course-library parts on 2026-11-10 to maximise use of `shared_lib`.
Do not place them on the Rev A board.

| Symbol | Replaced by (shared_lib) |
|---|---|
| USB4105-GF-A | `CON_USB_TYPEC_16P` (TYPE-C-3.1-16PIN) |
| MCP73831T-2ACI_OT | `IC_BAT_CHG_4.2V_1A` (TP4056) |
| AO3401A | `Q_PFET_30V_3A_SOT23` (AP2301) |
| B2B-PH-K-S | `Con_2x1_TH_JST_2MM` (S2B-PH-K-S) |

## History

- 2026-11-09: own library created; QFN parts removed in favour of leaded packages
  (PCM5102A + TPA6138A2 instead of ES8388; MCP73831 + AO3401A + AP2112K instead of MCP73871 + TPS63802).
- 2026-11-10: four missing 0603 passives added; USB-C, charger, PFET and battery connector moved to `shared_lib`.
- 2026-11-11: microSD and jack anchoring holes changed from drilled SMT pads to non-plated Multi-Layer pads.
- 2026-11-12: `TestPoint` / `TP_1.5mm` added for the board test points.
