# Reproductor MP3 portátil | Hardware Rev A

PCB propia diseñada en **Altium Designer 26.11.1** para el curso de Diseño Electrónico.
El firmware de la prueba de concepto está en `poc/`.

**Estado (2026-11-15): Rev A entregada** (etiqueta `rev-a`). Revisada por el profesor (concepto positivo), líneas de
datos USB corregidas y salidas de fabricación en `outputs/MP3_entregables/`. ERC: 0 errores. DRC: 0 violaciones, 0 advertencias.
Cómo está conectado todo: [MANUAL_CONEXIONES_RevA.md](docs/MANUAL_CONEXIONES_RevA.md).
El diseño completo está explicado en la [nota de aplicación](../docs/nota-aplicacion/Nota_de_Aplicacion_Reproductor_MP3.pdf).

## Restricciones del curso

| Restricción | Rev A |
|---|---|
| Capas de cobre | 2 (Top y Bottom); Bottom se usa sobre todo como plano de GND |
| Tamaño de la placa | ≤ 100 × 100 mm → contorno final **65 × 90.17 mm** |
| Fabricación | JLCPCB |
| Ensamble | Soldado a mano por los estudiantes |
| Librerías | Primero la `shared_lib` del profesor; piezas propias solo cuando se justifica |

## Arquitectura

| Bloque | Hoja | Componentes principales |
|---|---|---|
| Diagrama de bloques | `00_System_Top` | Entradas de hoja jerárquicas entre todos los bloques |
| Energía | `10_Power_Management` | Cargador TP4056 (R_PROG 4.7 kΩ ≈ 255 mA), selección de fuente con P-MOSFET AP2301 + Schottky (Microchip AN1149), LDO AP2112K-3.3, Li-Po 1S en JST-PH, divisores VBAT/2 y VBUS hacia el ADC |
| MCU | `20_MCU_Core` | ESP32-S3-WROOM-1-N16R8, RC de reset en EN, botones BOOT y RESET, header UART |
| USB | `30_USB_Interface` | USB-C 2.0 (5 V + datos), pull-downs CC de 4.7 kΩ, protección ESD USBLC6-2SC6 |
| Almacenamiento | `40_Storage` | Socket microSD (GCT MEM2075) en modo SPI, pull-ups de 10 kΩ, detección de tarjeta |
| Audio | `50_Audio_Codec` | DAC I²S PCM5102A → filtro 470 Ω / 2.2 nF → amplificador de audífonos TPA6138A2 (G = −1) → jack de 3.5 mm |
| Interfaz de usuario | `60_User_Interface` | 5 botones (activos en bajo, pull-ups de 10 kΩ), módulo OLED SSD1306 128×64 I²C en un header de 4 pines (GND, VCC, SCL, SDA) |

Rieles de alimentación: `VBUS` (5 V del USB), `VBAT` (3.0–4.2 V), `VSYS` (USB o batería), `+3V3` (lógica).

## Origen de los componentes

- **Librería del profesor (`shared_lib`)**: USB-C, TP4056, AP2301, conector de batería JST, diodo Schottky,
  USBLC6, LED, pulsadores, headers y todos los pasivos 0603 genéricos.
- **Librería propia (`MP3_RevA`)**: módulo ESP32-S3, AP2112K, PCM5102A, TPA6138A2, socket microSD,
  jack de audio, cuatro valores 0603 que faltan en la librería del curso (470 Ω, 100 kΩ, 2.2 µF, 2.2 nF)
  y un punto de prueba de un solo pad (`TP`, fuera de la BOM).
  La razón de cada pieza propia está en el [manual de conexiones](docs/MANUAL_CONEXIONES_RevA.md), sección 7.

`shared_lib` no se publica en este repositorio (licencia no confirmada). Los documentos de esquemático y PCB
incluyen todos los símbolos y huellas, así que el proyecto abre sin ella.

## PCB

- Apilado: FR-4 de 1.6 mm, cobre de 1 oz en ambas capas.
- Rellenos de GND en Top y Bottom, unidos con vías.
- Antena del ESP32-S3 fuera del borde de la placa, como indicó el profesor.
- Cuatro agujeros de montaje M3 sin metalizar (3.2 mm). La batería va debajo de la placa.
- El módulo OLED se monta sobre la placa, entre el ESP32 y la cruz de botones.
- Puntos de prueba (`TPn`): pads SMD redondos en Top, colocados desde los esquemáticos en las hojas 10 (rieles,
  GND y divisores del ADC), 20 (EN e IO0) y 50 (audio). Fuera de la BOM.
- Serigrafía de conectores, para el usuario: J1 `BATERIA`, J2 `USB-C`, J3 `MICRO SD`, J4 `AUDIO`,
  J5 `ON/OFF`, J6 `PANTALLA` (OLED, GND–VCC–SCL–SDA) y J7 `UART` (depuración, TX = IO43, RX = IO44).
- Serigrafía de botones: `VOL+`, `VOL-`, `PLAY/PAUSE`, `PREV`, `NEXT`, además de `BOOT` y reset.
  Los cinco botones de usuario tienen el mismo circuito (pull-up de 10 kΩ hacia una entrada), así que su función
  la asigna el firmware. **En la Rev A los nombres de red de los botones no coinciden con la serigrafía**; la
  función correcta de cada GPIO está en el [manual de conexiones](docs/MANUAL_CONEXIONES_RevA.md), sección de botones.
- Logo (dinosaurio, ~19.2 × 24.8 mm) en el Bottom Overlay, generado a partir de
  `altium/MP3_Player/Logos/dino_SXM_25mm_bottom_1px=1mil.bmp` (1 px = 1 mil, ya reflejado para la cara
  inferior). Está dibujado con pistas de serigrafía de 6 mil (0.152 mm, mínimo de JLCPCB), una por cada fila de
  5 píxeles, agrupadas en una sola unión. Está en la zona inferior derecha de la cara Bottom
  (X 2410–3166 mil, Y 1050–2027 mil), a ≥ 12 mil de pads y vías, fuera de las aberturas de máscara y
  a ≥ 1 mm del borde.

Reglas de diseño principales (JLCPCB, 2 capas, con margen para soldar a mano):

| Regla | Valor |
|---|---|
| Separación | 0.20 mm (0.15 mm solo dentro de la huella del USB-C) |
| Ancho de pista | 0.25 mm señales, 0.60 mm alimentación (0.40 mm mínimo) |
| Vías | 0.30 mm de agujero / 0.60 mm de diámetro |
| Separación al contorno | 0.30 mm (0 mm para conectores de borde y el ESP32) |
| Franja mínima de máscara | 0.10 mm (0.13 mm si se pide máscara negra o blanca) |
| Serigrafía | Texto de 1.0 mm de alto, trazo de 0.15 mm |

## Estructura de la carpeta

```text
hardware/
├── README.md
├── .gitignore
├── altium/
│   ├── MP3_Player/                 # Proyecto de Altium: .PrjPcb, 7 hojas .SchDoc, .PcbDoc
│   │   └── Logos/                  # Mapa de bits del logo del Bottom Overlay
│   └── libraries/
│       ├── project/                # Librería propia MP3_RevA (.SchLib, .PcbLib, .LibPkg)
│       └── external/               # Librerías del curso/fabricantes, ignoradas por Git
├── outputs/
│   └── MP3_entregables/            # Salidas de la entrega: GERBERS (zip, con el reporte DRC), BOM, DRAFTSMAN (PDF),
│                                   # 3D (STEP, PDF3D), SHC (esquemático en PDF)
└── docs/
    ├── MANUAL_CONEXIONES_RevA.md   # Manual de conexiones: conectores, mapa de GPIO, bloques, puntos de prueba
    ├── WORKFLOW_REV_A.md           # Plan de trabajo de la Rev A
    └── POWER_BUDGET_REV_A.md       # Presupuesto de corriente y estimado de autonomía
```

## Pendientes

- [x] Revisión de esquemáticos y PCB por el profesor (2026-11-14, positiva)
- [x] Presupuesto de potencia y autonomía: [POWER_BUDGET_REV_A.md](docs/POWER_BUDGET_REV_A.md)
  (≈ 115 mA reproduciendo, ≈ 7.5 h con una celda de 1000 mAh; por confirmar con medidas)
- [x] Salidas de fabricación en `outputs/MP3_entregables/` (Gerbers + taladros, BOM, Draftsman, 3D, esquemático en PDF)
- [x] USB D+/D− conectados a IO20/IO19 (se encontraron sin conectar en la revisión; corregido y salidas regeneradas el 2026-11-15)
- [x] Etiqueta `rev-a` y entrega al profesor (2026-11-15)
- [x] Manual de conexiones: [MANUAL_CONEXIONES_RevA.md](docs/MANUAL_CONEXIONES_RevA.md)
- [x] Tabla de botones corregida en el manual (redes frente a serigrafía, 2026-11-25)
- [ ] Confirmar la polaridad del cable de la batería y el orden de pines del módulo OLED antes de soldar
- No previsto para la Rev A: marcos por bloque en la hoja 60 (las hojas 00–50 sí los tienen)

## Práctica con Git

El diseño de la Rev A se hizo en la rama `hardware/rev-a-altium` (ya eliminada; todo su contenido está en `main`) y se
integró a `main` con pull requests en cada hito; la etiqueta `rev-a` marca el diseño entregado. Los cambios futuros (Rev B) deben hacerse en una rama nueva
desde `main`, sin modificar `poc/`, y conservando la licencia GPLv3.
