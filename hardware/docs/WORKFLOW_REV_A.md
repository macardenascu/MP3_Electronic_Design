# Rev A academic PCB | Work plan

Status: **Native files integrated on `hardware/rev-a-altium`; Altium reopen and library-link validation pending**.

## G0 | Repo and Altium setup
- [x] Project visible in Altium: `MP3_Player.PrjPcb`
- [x] Local sheets visible: `00_System_Top.SchDoc`, `10_Power_Management.SchDoc`, `60_User_Interface.SchDoc`
- [x] Local board visible: `MP3_Player.PcbDoc`
- [x] Professor's integrated library visible in Components panel
- [x] Confirm the project and three schematic sheets plus board are present in `hardware/altium/project/`
- [ ] Reopen `hardware/altium/project/MP3_Player.PrjPcb` in Altium and verify all document and library links
- [ ] Verify library redistribution rights before publishing external library binaries
- [ ] Commit native project files, not only screenshots

## G1 | Library and component evidence
- [ ] Create BOM/part selection register with MPN, role, manufacturer/datasheet, supply rail, package, footprint source, and review status
- [ ] Verify symbol pin numbers against datasheet and footprint pads
- [ ] Verify USB-C, jack, microSD, ESP32-S3, charger, buck-boost, codec, and OLED footprints
- [ ] Log unresolved components; never assume an IntLib part is correct by origin alone

## G2 | System architecture and floorplan
- [ ] Freeze academic MUST requirements with faculty approval for any change
- [ ] Build interface and preliminary GPIO tables (USB, I2S, SPI, I2C, button, power)
- [ ] Estimate mechanical constraints and place component outline previews within 100 × 100 mm
- [ ] Allocate two layers only: Top and Bottom; plan return currents and antenna keepout
- [ ] Maintain separate wired audio and optional Bluetooth feasibility decisions

## G3 | Schematic by verified blocks
- [ ] Power: USB VBUS, battery protection, MCP73871, TPS63802, power states, power budget
- [ ] MCU core, strapping, reset, USB and recovery paths
- [ ] Storage: SPI microSD
- [ ] Audio: ES8388 I2S, power rails, output loading and ESD
- [ ] User interface: five buttons, OLED, voltage/pull-up compatibility
- [ ] Top level: documented hierarchical ports and inter-sheet interfaces
- [ ] ERC, interface review, and pin-to-pad audit

## G4 | Board design
- [ ] Manufacturer and fabrication rules specified; two copper layers verified
- [ ] Component placement/board outline reviewed before routing
- [ ] Route power and critical nets first, manage continuous ground return
- [ ] Check analog audio and switching converter separation
- [ ] DRC and 3D visual review; do not confuse DRC pass with functional validation

## G5 | Academic deliverables
- [ ] Native Altium project and own libraries
- [ ] PDFs of schematic and board-layer views; 3D captures if applicable
- [ ] BOM, net/interface and design decision notes
- [ ] DRC/ERC reports and explicit exceptions/limitations
- [ ] Fabrication outputs if required by course, and board-size evidence
- [ ] Revision-tagged Git checkpoint

## Current known risks
- 2-layer signal return and audio noise versus a switching regulator.
- Battery safety, charger current and power-path protection require datasheet-backed design.
- ESP32-S3 module pin availability and USB pins must be checked before final GPIO assignment.
- ES8388 output and supplies cannot be inferred from generic DAC diagrams.
- Instructor's IntLib does not contain all application-specific parts; their footprints are not yet verified.
- The project file references its three schematic sheets and board by relative filenames; those files are present beside the project. Library links still require manual validation in Altium. The instructor library remains local and excluded from Git pending redistribution permission.
- Physical validation and 8-hour autonomy are design goals, not demonstrated results.

## Review discipline
Need → numerical/electrical requirement → alternatives → verified selection → Altium implementation → ERC/DRC checks → status (approved, provisional or pending).
Do not edit the separate PoC firmware during this workflow.
