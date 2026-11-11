# MP3_RevA — own Altium library

Components the course library (`shared_lib.IntLib`) does not have. Generic parts
(R, C, LED, push button, Schottky diode, USBLC6-2SC6, pin headers) keep coming from the course library.

Files: `MP3_RevA.SchLib`, `MP3_RevA.PcbLib`, `MP3_RevA.LibPkg` (open it and run
*Project → Compile Integrated Library* to get `MP3_RevA.IntLib`).

**Status of every part: pinout and land pattern taken from the datasheet below and
cross-checked symbol pin ↔ footprint pad (all match). Not built or tested.**
Every footprint has an embedded manufacturer STEP model. Footprints were simplified to the course style (2026-11-09).
USB-C pad B5 (CC2) was restored on 2026-11-09 at x = +1.75 mm, mirroring B8.

## Revision 2 (2026-11-09): leaded packages preferred

QFN parts were removed. The architecture changed to:
- Audio: PCM5102A I²S DAC + TPA6138A2 headphone amplifier (replaces ES8388).
- Power: MCP73831 charger + AO3401A load-sharing switch (Microchip AN1149) + AP2112K 3.3 V LDO
  (replaces MCP73871 + TPS63802; the XFL4015 inductor and the NTC are no longer needed).

| Symbol | Footprint | Source (revision) | Notes |
|---|---|---|---|
| ESP32-S3-WROOM-1-N16R8 | ESP32-S3-WROOM-1 | Espressif datasheet v1.8, Table 3-1, Fig. 11-1 | Manufacturer STEP model embedded. Antenna keep-out included. EPAD = one 3.7 mm pad (pin 41). IO35–IO37 used by the PSRAM. |
| PCM5102APWR | TSSOP-20_4.4x6.5mm_P0.65mm | TI SLAS859C, pin table and PW0020A board layout | I²S DAC, hardware control only. |
| TPA6138A2PWR | TSSOP-14_4.4x5mm_P0.65mm | TI SLOS704B, pin table and PW0014A board layout | Ground-centred outputs, no coupling capacitors. |
| MCP73831T-2ACI_OT | SOT-23-5 | Microchip DS21984A (2005) | Orderable MPN `MCP73831T-2ACI/OT`. Check the latest revision (DS20001984). |
| AP2112K-3.3TRG1 | SOT-23-5 | Diodes DS39724 Rev. 2-2, SOT25 suggested pad layout | 600 mA LDO. Dropout limits use of the last part of the battery. |
| AO3401A | SOT-23-3 | AOS AO3401A Rev 3.1 | Pin order 1 = G, 2 = S, 3 = D read from the package drawing. Land derived from the SOT25 pattern. |
| USB4105-GF-A | USB_C_GCT_USB4105-GF-A | GCT drawing rev B4 | Mechanical 1 line marks the PCB edge (y = −2.60 mm). |
| MEM2075-00-140-01-A | MicroSD_GCT_MEM2075-00-140-01-A | GCT drawing rev A1 | Card-detect switch closes to the shell. Keep-out under the card not drawn yet. |
| SJ-3523-SMT | Jack_3.5mm_SameSky_SJ-3523-SMT | Same Sky datasheet 04/15/2025 (rev 1.07) | Pin 4 not populated on this variant. |
| B2B-PH-K-S | JST_PH_B2B-PH-K-S_1x02_P2.00mm | JST PH catalog | Battery cable polarity is not standard. |
