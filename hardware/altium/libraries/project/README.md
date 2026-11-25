# MP3_RevA — librería propia de Altium

Componentes que la librería del curso (`shared_lib.IntLib`) no tiene, o no tiene en una forma utilizable.
Todo lo demás en la placa viene de la librería del curso (ver `hardware/README.md`).
La razón de cada pieza propia está en [MANUAL_CONEXIONES_RevA.md](../../../docs/MANUAL_CONEXIONES_RevA.md), sección 7.

Archivos: `MP3_RevA.SchLib`, `MP3_RevA.PcbLib`, `MP3_RevA.LibPkg` (ábralo y ejecute
*Project → Compile Integrated Library* para obtener `MP3_RevA.IntLib`).

**Estado de cada pieza: pinout y huella tomados del datasheet indicado abajo, y verificado
pin del símbolo ↔ pad de la huella (todos coinciden). No fabricado ni probado.**
Cada huella de CI o conector tiene incrustado el modelo STEP del fabricante.

## Piezas usadas en la placa Rev A

| Símbolo | Huella | Fuente (revisión) | Notas |
|---|---|---|---|
| ESP32-S3-WROOM-1-N16R8 | ESP32-S3-WROOM-1 | Datasheet de Espressif v1.8, Tabla 3-1, Fig. 11-1 | EPAD = un pad (pin 41), revisado contra la huella del datasheet; vías de GND añadidas en la placa. IO35–IO37 los usa la PSRAM. La antena sobresale del borde de la placa. |
| AP2112K-3.3TRG1 | SOT-23-5 | Diodes DS39724 Rev. 2-2, distribución de pads sugerida SOT25 | LDO de 600 mA. Elegido en lugar del AMS1117 del curso por su caída de voltaje con batería. |
| PCM5102APWR | TSSOP-20_4.4x6.5mm_P0.65mm | TI SLAS859C, tabla de pines y layout PW0020A | DAC I²S, control solo por pines. Los componentes de soporte siguen la Figura 33. |
| TPA6138A2PWR | TSSOP-14_4.4x5mm_P0.65mm | TI SLOS704B, tabla de pines y layout PW0014A | Salidas centradas en tierra, sin capacitores de acople. UVP sin conectar (pull-up interno). |
| MEM2075-00-140-01-A | MicroSD_GCT_MEM2075-00-140-01-A | Plano de GCT rev A1 | El interruptor de detección de tarjeta cierra contra la carcasa. NP1/NP2 son agujeros Multi-Layer sin metalizar (corregido el 2026-11-11). |
| SJ-3523-SMT | Jack_3.5mm_SameSky_SJ-3523-SMT | Datasheet de Same Sky 04/15/2025 (rev 1.07) | Pin 1 manga, 2 punta (L), 3 anillo (R). El pin 4 no existe en esta variante. NP1/NP2 son agujeros Multi-Layer sin metalizar (corregido el 2026-11-11). |
| TP | TP | Diseño propio | Un pad SMD redondo en Top, con abertura de máscara. Fuera de la BOM. Se usa para todos los puntos de prueba de la placa. |

### Pasivos 0603 que faltan en la librería del curso

Copias de las plantillas del curso (`Res_1k_0603_1/10W`, `Cap_1uF_0603_50V`, `Cap_1nF_0603_50V`) con la misma
huella `0603_res` / `0603_cap` y el mismo modelo 3D. Los datos de fabricante quedan como `TBD` hasta elegir una referencia.

| Símbolo | Valor | Uso |
|---|---|---|
| Res_470R_0603_1/10W | 470 Ω | Filtro RC a la salida del DAC |
| Res_100k_0603_1/10W | 100 kΩ | Pull-down del MUTE del amplificador |
| Cap_2.2uF_0603_10V | 2.2 µF | Bomba de carga del DAC (CAPP–CAPM, VNEG) |
| Cap_2.2nF_0603_50V | 2.2 nF | Filtro RC a la salida del DAC |

## Piezas que siguen en la librería pero no se usan (retiradas en la rev 2 del esquemático)

Reemplazadas por piezas de la librería del curso el 2026-11-10 para aprovechar al máximo `shared_lib`.
No colocarlas en la placa Rev A.

| Símbolo | Reemplazado por (shared_lib) |
|---|---|
| USB4105-GF-A | `CON_USB_TYPEC_16P` (TYPE-C-3.1-16PIN) |
| MCP73831T-2ACI_OT | `IC_BAT_CHG_4.2V_1A` (TP4056) |
| AO3401A | `Q_PFET_30V_3A_SOT23` (AP2301) |
| B2B-PH-K-S | `Con_2x1_TH_JST_2MM` (S2B-PH-K-S) |

## Historial

- 2026-11-09: se crea la librería propia; se quitan las piezas QFN en favor de encapsulados con patas
  (PCM5102A + TPA6138A2 en lugar del ES8388; MCP73831 + AO3401A + AP2112K en lugar de MCP73871 + TPS63802).
- 2026-11-10: se agregan los cuatro pasivos 0603 que faltaban; USB-C, cargador, PFET y conector de batería pasan a `shared_lib`.
- 2026-11-11: los agujeros de anclaje de la microSD y del jack pasan de pads SMT taladrados a pads Multi-Layer sin metalizar.
- 2026-11-12: se agregan el símbolo y la huella `TP` para los puntos de prueba de la placa.
