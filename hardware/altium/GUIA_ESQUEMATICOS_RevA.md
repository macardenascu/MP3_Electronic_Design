# Guía paso a paso: esquemáticos MP3 Rev A

Versión 2026-11-10 (rev 2). Proyecto `hardware/altium/MP3_Player/MP3_Player.PrjPcb`.

Esta guía dice **qué colocar, de qué librería y cómo conectarlo**, hoja por hoja. Las conexiones las haces tú.
Los valores marcados **(verificar)** salen del circuito típico de cada datasheet; confírmalos antes de cerrar la hoja.

### Cambios en esta versión (rev 2)

Se aplicó el criterio de **usar primero los componentes de la librería del profesor (`shared_lib`)** y justificar cada pieza propia (sección 0.4).

| # | Antes (MP3_RevA) | Ahora (`shared_lib`) | Hoja |
|---|---|---|---|
| 1 | `USB4105-GF-A` | `CON_USB_TYPEC_16P` (TYPE-C-3.1-16PIN) | 30 |
| 2 | `MCP73831T-2ACI_OT` | `IC_BAT_CHG_4.2V_1A` (TP4056) | 10 |
| 3 | `AO3401A` | `Q_PFET_30V_3A_SOT23` (AP2301) | 10 |
| 4 | `B2B-PH-K-S` | `Con_2x1_TH_JST_2MM` (S2B-PH-K-S) | 10 |
| 5 | Faltaban 470 Ω, 100 kΩ, 2,2 µF y 2,2 nF | Creados en `MP3_RevA` a partir de las plantillas 0603 del profesor | 50 |

Se mantienen el **ESP32-S3** y el **AP2112K**. La justificación está en la sección 0.4.

Tiempo estimado si sigues el orden: 30 USB (30 min) → 10 Energía (1 h) → 20 MCU (1 h) → 40 microSD (30 min)
→ 50 Audio (1 h 30) → 60 UI (30 min) → 00 Top (30 min).

---

## 0. Antes de empezar (10 min)

### 0.1 Comandos de Altium que vas a usar todo el tiempo

| Qué | Cómo |
|---|---|
| Colocar componente | Panel **Components** → elige librería → doble clic en la pieza (o arrástrala). `Espacio` rota, `X`/`Y` espeja |
| Cable | `Ctrl+W` (Place → Wire). Clic en el extremo del pin, clic en el destino. `Esc` termina |
| Power port | Place → Power Port. En *Properties*, **Name** = `GND`, `+3V3`, `VBUS`… y **Style** = Power Ground para GND, Bar/Arrow para los rieles |
| Net label (local) | Place → Net Label. Debe tocar el cable |
| Port (sale de la hoja) | Place → Port. **Name** = nombre de la señal, **I/O Type** = Input / Output / Bidirectional |
| No ERC | Place → Directives → Generic No ERC (la X verde) sobre el extremo del pin |
| Cambiar valor/parámetro | Selecciona la pieza → panel **Properties** |
| Compilar y revisar | Project → **Validate PCB Project** → panel **Messages** |

### 0.2 Reglas del proyecto

1. **Rieles con Power Port:** `GND`, `VBUS`, `VBAT`, `VSYS` y `+3V3`. Son globales, así que conectan entre hojas sin hacer nada más.
2. **Señales entre hojas con Port** en la hoja hija y Sheet Entry en `00_System_Top` (sección 7). Un Net Label **no** sale de su hoja.
3. **Designadores:** deja `R?`, `C?`, `U?`. Al final usa Tools → Annotation → Annotate Schematics.
4. **Pines sin usar:** ponles No ERC.
5. **Librerías:**
   - `shared_lib` (profesor): USB-C, TP4056, AP2301, JST de batería, R, C, LED, Schottky, USBLC6, pulsadores, headers.
   - `MP3_RevA` (propia): ESP32-S3, AP2112K, PCM5102A, TPA6138A2, microSD, jack y 4 pasivos 0603 que no existían (470 Ω, 100 kΩ, 2,2 µF, 2,2 nF).
6. Guarda (`Ctrl+S`) y corre **Validate** al terminar cada hoja.

### 0.3 Piezas genéricas de `shared_lib` que vas a necesitar

| Valor | Nombre en la librería |
|---|---|
| 100 nF | `Cap_0.1uF_0603_50V` |
| 1 µF | `Cap_1uF_0603_50V` |
| 10 µF | `Cap_10uF_0603_10V` |
| 2,2 nF | `Cap_2.2nF_0603_50V` (**MP3_RevA**) |
| 2,2 µF | `Cap_2.2uF_0603_10V` (**MP3_RevA**) |
| 470 Ω / 100 kΩ | `Res_470R_0603_1/10W` / `Res_100k_0603_1/10W` (**MP3_RevA**) |
| 1 kΩ / 3,3 kΩ / 4,7 kΩ / 10 kΩ / 49,9 kΩ / 390 kΩ | `Res_1k_…`, `Res_3k3_…`, `Res_4k7_…`, `Res_10k_…`, `Res_49k9_…`, `Res_390k_0603_1/10W` |
| LED rojo | `LED_0805_RED` (pin 1 = ánodo, pin 2 = cátodo) |
| Schottky | `D_Schottky_40V_1A_SOD_123F` (pin 1 = cátodo, pin 2 = ánodo) |
| USBLC6 | `TVS_5.25V_2CH_SOT_23_6` |
| Header 2 / 3 / 4 pines | `H_Male_2x1_TH_100mil`, `H_Male_3x1_…`, `H_Male_4x1_…` |

Los 4 pasivos de `MP3_RevA` son copias de las plantillas 0603 del profesor (`Res_1k_0603_1/10W`, `Cap_1uF_0603_50V`, `Cap_1nF_0603_50V`), con la misma huella `0603_res` / `0603_cap` y el mismo modelo 3D. Sus datos de fabricante están en `TBD` hasta elegir número de pieza.

Si un valor no existe en ninguna librería, pon el más cercano y cámbiale el parámetro **Comment/Value**. Anótalo para el BOM.

### 0.4 Origen de cada componente y justificación de los propios

Criterio: si `shared_lib` tiene una pieza que cumple la función, se usa esa. Una pieza propia solo entra si la del profesor no cumple un requisito medible o si no existe.

**Componentes del profesor (`shared_lib`) que usa el diseño**

| Función | Pieza en `shared_lib` | Fabricante / MPN | Hoja |
|---|---|---|---|
| Conector USB-C | `CON_USB_TYPEC_16P` | TYPE-C-3.1-16PIN | 30 |
| Protección ESD de D+/D− | `TVS_5.25V_2CH_SOT_23_6` | ST USBLC6-2SC6 | 30 |
| Cargador Li-ion | `IC_BAT_CHG_4.2V_1A` | TP4056 (SOIC-8) | 10 |
| Switch del load sharing | `Q_PFET_30V_3A_SOT23` | AP2301 (SOT-23) | 10 |
| Diodo VBUS → VSYS | `D_Schottky_40V_1A_SOD_123F` | PSBD1DF40V1H | 10 |
| Conector de batería | `Con_2x1_TH_JST_2MM` | JST S2B-PH-K-S | 10 |
| Pulsadores | `Sw_PUSH_4.5x4.5x5.0xmm` | — | 20, 60 |
| Headers (OLED, UART, encendido) | `H_Male_*x1_TH_100mil` | — | 10, 20, 60 |
| LED | `LED_0805_RED` | — | 10 |
| Resistencias y capacitores 0603 | `Res_*_0603_1/10W`, `Cap_*_0603_*` | varios | todas |

**Componentes propios (`MP3_RevA`) y por qué**

| Pieza | Alternativa en `shared_lib` | Justificación |
|---|---|---|
| **ESP32-S3-WROOM-1-N16R8** | `MCU_ESP32-C3-WROOM-02-N4` | El C3-WROOM-02 expone 15 GPIO y 2 se van en el USB (IO18/IO19), así que quedan **13**. El reproductor necesita **19 señales** además del USB (I²S ×3, microSD ×5, botones ×5, OLED ×2, ADC ×2, mute ×2; ver hoja 20). Para que cupiera habría que meter los botones en una escalera resistiva sobre un ADC, unir los dos mutes, quitar la detección de tarjeta y usar los pines de arranque IO2/IO8/IO9 como señales, sin ningún pin libre de reserva. El S3 deja margen y además trae doble núcleo y 8 MB de PSRAM para el buffer de audio. |
| **AP2112K-3.3** (LDO) | `LDO_3V3_1A_FIXED` (AMS1117-3.3) | El AMS1117 necesita ≈ 1,1 V de caída (dropout) a 1 A, así que pide VIN ≥ 4,4 V para dar 3,3 V. La batería Li-ion entrega 3,0–4,2 V, de modo que **con batería no regula**. El AP2112K tiene ≈ 250 mV de dropout a 600 mA, trae pin **EN** (lo usamos para `PWR_EN`) y consume ≈ 55 µA en reposo, contra ≈ 5 mA del AMS1117. |
| **PCM5102A** (DAC I²S) | No existe | `shared_lib` no tiene DAC de audio. |
| **TPA6138A2** (amplificador de audífonos) | No existe | `shared_lib` no tiene amplificador de audio. |
| **MEM2075** (microSD) | No existe | `shared_lib` no tiene zócalo microSD. |
| **SJ-3523-SMT** (jack 3,5 mm) | No existe | `shared_lib` no tiene jack de audio. |
| **470 Ω, 100 kΩ, 2,2 µF, 2,2 nF** (0603) | No existen esos valores | Son copias de las plantillas del profesor con otro valor. Usan su misma huella y su modelo 3D. |

Las piezas de `MP3_RevA` que ya no se usan (`USB4105-GF-A`, `MCP73831T-2ACI_OT`, `AO3401A`, `B2B-PH-K-S`) se quedan en la librería, pero no las coloques.

---

## 1. Hoja `30_USB_Interface`

**Qué hace:** recibe 5 V del USB-C (`VBUS`), le dice al cargador que es un dispositivo (resistencias CC) y protege D+/D− con el USBLC6.

### Piezas

| Cant. | Pieza | Librería |
|---|---|---|
| 1 | `CON_USB_TYPEC_16P` (J?) — **cambiado en rev 2** | shared_lib |
| 1 | `TVS_5.25V_2CH_SOT_23_6` (U?) | shared_lib |
| 2 | `Res_4k7_0603_1/10W` | shared_lib |
| 1 | `Cap_0.1uF_0603_50V` | shared_lib |

Si ya habías colocado el `USB4105-GF-A`, bórralo y pon el del profesor. Los pines tienen los mismos números, así que las conexiones son casi iguales.

### Pasos

1. Coloca J? a la izquierda de la hoja y U? a la derecha.
2. **VBUS:** une con cable los pines **A4/B9** y **B4/A9** (los dos se llaman VBUS) y ponles un Power Port `VBUS`.
3. **GND:** une **A1/B12** y **B1/A12** (GND) y el pin **0 (SHLD)**, que es el blindaje, y ponles un Power Port `GND`.
   En la huella `CON_USB_C_16P_RA` los 4 pads del blindaje se llaman "0", así que este único pin los conecta a todos.
4. **CC1:** pin **A5 (CC1)** → resistencia de 4,7 kΩ → `GND`.
5. **CC2:** pin **B5 (CC2)** → **otra** resistencia de 4,7 kΩ → `GND`. Nunca unas CC1 con CC2.
   (La norma pide 5,1 kΩ ±20 %, es decir 4,08–6,12 kΩ, y 4,7 kΩ entra en ese rango.)
6. **D+:** une **A6 (DP1)** con **B6 (DP2)** y ponle un Net Label `USB_DP`.
7. **D−:** une **A7 (DN1)** con **B7 (DN2)** y ponle un Net Label `USB_DN`.
8. **SBU:** No ERC en **A8 (SBU1)** y **B8 (SBU2)**.
9. **USBLC6:**
   - pin **1** y pin **6** → `USB_DP`
   - pin **3** y pin **4** → `USB_DN`
   - pin **5** → `VBUS`, con 100 nF a `GND`
   - pin **2** → `GND`
10. Pon un **Port** `USB_DP` y un Port `USB_DN`, los dos *Bidirectional*, en las nets del lado del USBLC6.
11. Validate. No deben quedar pines del USB sin conectar ni sin No ERC.

---

## 2. Hoja `10_Power_Management`

**Qué hace:** carga la batería (TP4056), decide de dónde se alimenta el sistema (`VSYS` desde USB o desde la batería, según Microchip AN1149), genera `+3V3` (AP2112K) y mide la batería y la presencia de USB.

### Piezas

| Cant. | Pieza | Para qué |
|---|---|---|
| 1 | `IC_BAT_CHG_4.2V_1A` (TP4056, shared_lib) — **cambiado en rev 2** | Cargador |
| 1 | `Con_2x1_TH_JST_2MM` (S2B-PH-K-S, shared_lib) — **cambiado en rev 2** | Conector de batería |
| 1 | `Q_PFET_30V_3A_SOT23` (AP2301, shared_lib) — **cambiado en rev 2** | Switch del load sharing |
| 1 | `AP2112K-3.3TRG1` (MP3_RevA) | Regulador de 3,3 V |
| 1 | `D_Schottky_40V_1A_SOD_123F` | VBUS → VSYS |
| 1 | `LED_0805_RED` | LED de carga |
| 4 | `Cap_10uF_0603_10V` | Entrada y salida del cargador y del LDO |
| 2 | `Cap_0.1uF_0603_50V` | Salida del LDO y nodo del ADC |
| 1 | `Res_4k7` | PROG (≈ 255 mA) |
| 1 | `Res_1k` | LED |
| 1 | `Res_49k9` | Pull-down de la compuerta |
| 1 | `Res_10k` | Pull-up de EN |
| 4 | `Res_390k` | Dos divisores |
| 1 | `H_Male_2x1_TH_100mil` | Interruptor de encendido (provisional) |

Organiza la hoja en 4 bloques de izquierda a derecha: **cargador → load sharing → LDO → divisores**.

### Bloque A: cargador TP4056

Ten abierto el datasheet del TP4056 (NanJing Top Power) en la figura de aplicación típica.

1. Coloca U? (`IC_BAT_CHG_4.2V_1A`).
2. Pin **4 (VCC)** → `VBUS`. Pon un 10 µF entre `VBUS` y `GND` junto al pin.
3. Pin **3 (GND)** → `GND`.
4. Pin **5 (BAT)** → `VBAT`. Pon un 10 µF entre `VBAT` y `GND`.
5. Pin **2 (PROG)** → resistencia de 4,7 kΩ → `GND`. La corriente es I ≈ 1200 V / R_PROG:
   - **4,7 kΩ → ≈ 255 mA**, la recomendada: carga a ≤ 0,5 C una batería de 500 mAh o más.
   - 3,3 kΩ → ≈ 364 mA, si la batería es de 750 mAh o más.
   - 1,2 kΩ → 1 A, que es el máximo del chip. No la uses, porque se calienta mucho.
6. Pin **8 (CE)** → `VBUS`. Así el cargador queda siempre habilitado.
7. Pin **1 (TEMP)** → `GND`. Así se desactiva la función de termistor (el datasheet lo indica cuando no se usa NTC).
8. Pin **7 (CHRG, activo en bajo)** → **cátodo** del LED (pin 2). El **ánodo** (pin 1) va a una resistencia de 1 kΩ y de ahí a `VBUS`.
   CHRG es de drenador abierto: mientras carga baja a 0 V y el LED se enciende. Cuando termina, se apaga.
9. Pin **6 (STDBY, activo en bajo)** → No ERC. Opcional: un segundo LED igual al de CHRG para indicar "carga completa".
10. **Batería J? (`Con_2x1_TH_JST_2MM`):** pin **1** → `VBAT`, pin **2** → `GND`.
    Pon una nota con *Place → Text* que diga "verificar polaridad del cable".

### Bloque B: load sharing (AN1149)

11. Coloca Q? (`Q_PFET_30V_3A_SOT23`, AP2301, canal P). Tiene el mismo pinout que el AO3401A:
   - pin **1 (G)** → `VBUS`
   - pin **3 (D)** → `VBAT`
   - pin **2 (S)** → `VSYS`
12. Resistencia de 49,9 kΩ entre `VBUS` y `GND`. Cuando desconectas el USB, baja la compuerta a 0 V.
13. Diodo Schottky: pin **2 (ánodo)** → `VBUS`, pin **1 (cátodo)** → `VSYS`.

Así funciona: con USB conectado, la compuerta está a 5 V, el PFET se apaga y `VSYS` sale de VBUS por el diodo. Sin USB, la compuerta baja a 0 V, el PFET conduce y `VSYS` sale de la batería.

### Bloque C: LDO AP2112K

14. Coloca U? (AP2112K):
    - pin **1 (VIN)** → `VSYS`, con 10 µF a `GND`
    - pin **2 (GND)** → `GND`
    - pin **5 (VOUT)** → `+3V3`, con 10 µF y 100 nF a `GND`
    - pin **4 (NC)** → No ERC
15. Pin **3 (EN)** → Net Label `PWR_EN`.
16. Resistencia de 10 kΩ de `PWR_EN` a `VSYS`.
17. Header de 2 pines: pin 1 → `PWR_EN`, pin 2 → `GND`. Si pones un jumper o un interruptor ahí, el sistema se apaga y la batería sigue cargando.

### Bloque D: divisores para el ESP32

18. **Batería:** `VBAT` → 390 kΩ → nodo → 390 kΩ → `GND`, y 100 nF del nodo a `GND`. En el nodo pon un **Port** `BAT_ADC` (*Output*). Da VBAT/2, como máximo 2,1 V.
19. **USB presente:** `VBUS` → 390 kΩ → nodo → 390 kΩ → `GND`. En el nodo pon un **Port** `VBUS_DET` (*Output*). Da 2,5 V con USB y 0 V sin USB.
20. Validate.

---

## 3. Hoja `20_MCU_Core`

**Qué hace:** el ESP32-S3 recibe todas las señales de las otras hojas.

La tabla de GPIO es una **propuesta**: evita los pines de arranque (IO0, IO3, IO45, IO46) y los IO35–IO37, que usa la PSRAM. Revísala contra el datasheet de Espressif v1.8, tabla 3-1.

### Piezas

| Cant. | Pieza |
|---|---|
| 1 | `ESP32-S3-WROOM-1-N16R8` (MP3_RevA) |
| 1 | `Cap_10uF_0603_10V` |
| 1 | `Cap_0.1uF_0603_50V` |
| 1 | `Cap_1uF_0603_50V` (EN) |
| 1 | `Res_10k` (EN) |
| 2 | Pulsador `Sw_PUSH_4.5x4.5x5.0xmm` (BOOT y RESET) |
| 1 | `H_Male_3x1_TH_100mil` (UART, opcional) |

### Pasos

1. **Alimentación:**
   - pin **2 (3V3)** → `+3V3`, con 10 µF y 100 nF a `GND`
   - pines **1**, **40** y **41 (EPAD)** → `GND`
2. **Reset:** pin **3 (EN)** → 10 kΩ a `+3V3` + 1 µF a `GND`. Opcional: pulsador RESET de EN a `GND`.
3. **Boot:** pin **27 (IO0)** → pulsador BOOT a `GND`. El pin ya tiene pull-up interno.
4. **Pines de arranque** **15 (IO3)**, **16 (IO46)** y **26 (IO45)**: No ERC.
5. **PSRAM:** pines **28**, **29** y **30** (IO35–IO37): No ERC. **Nunca** los conectes.
6. **UART (opcional):** pin **37 (TXD0)** y pin **36 (RXD0)** al header de 3 pines, junto con GND.
7. Pon un **Port** en cada pin según esta tabla. La dirección es la que tiene la señal vista desde el MCU:

| Port | GPIO | Pin del símbolo | I/O Type |
|---|---|---|---|
| `USB_DN` | IO19 | 13 | Bidirectional |
| `USB_DP` | IO20 | 14 | Bidirectional |
| `BAT_ADC` | IO1 | 39 | Input |
| `VBUS_DET` | IO2 | 38 | Input |
| `I2S_BCK` | IO4 | 4 | Output |
| `I2S_LRCK` | IO5 | 5 | Output |
| `I2S_DIN` | IO6 | 6 | Output |
| `AMP_MUTE_N` | IO7 | 7 | Output |
| `OLED_SDA` | IO8 | 12 | Bidirectional |
| `OLED_SCL` | IO9 | 17 | Output |
| `SD_CS` | IO10 | 18 | Output |
| `SD_MOSI` | IO11 | 19 | Output |
| `SD_SCK` | IO12 | 20 | Output |
| `SD_MISO` | IO13 | 21 | Input |
| `SD_CD_N` | IO14 | 22 | Input |
| `BTN_UP_N` | IO15 | 8 | Input |
| `BTN_DOWN_N` | IO16 | 9 | Input |
| `BTN_PLAY_N` | IO17 | 10 | Input |
| `BTN_PREV_N` | IO18 | 11 | Input |
| `BTN_NEXT_N` | IO21 | 23 | Input |
| `DAC_XSMT` | IO38 | 31 | Output |

8. A todos los pines IO que no usaste ponles No ERC.
9. Validate.

Consejo: pon los Ports en columna a la derecha del símbolo, cada uno con un cable corto a su pin, en el mismo orden de la tabla. Así se revisa más rápido.

---

## 4. Hoja `40_Storage` (microSD por SPI)

### Piezas

| Cant. | Pieza |
|---|---|
| 1 | `MEM2075-00-140-01-A` (MP3_RevA) |
| 6 | `Res_10k_0603_1/10W` |
| 1 | `Cap_10uF_0603_10V` |
| 1 | `Cap_0.1uF_0603_50V` |

### Pasos

1. Pin **4 (VDD)** → `+3V3`, con 10 µF y 100 nF a `GND`.
2. Pin **6 (VSS)** → `GND`. Une **G1**, **G2**, **G3** y **G4** (el blindaje) → `GND`.
3. Pin **2 (DAT3/CS)** → Port `SD_CS` (*Input*), con 10 kΩ a `+3V3`.
4. Pin **3 (CMD/MOSI)** → Port `SD_MOSI` (*Input*), con 10 kΩ a `+3V3`.
5. Pin **5 (CLK)** → Port `SD_SCK` (*Input*). Este no lleva pull-up.
6. Pin **7 (DAT0/MISO)** → Port `SD_MISO` (*Output*), con 10 kΩ a `+3V3`.
7. Pines **8 (DAT1)** y **1 (DAT2)** → 10 kΩ cada uno a `+3V3`. No se usan en SPI, pero no deben quedar flotando.
8. Pin **CD** → Port `SD_CD_N` (*Output*), con 10 kΩ a `+3V3`. Cuando metes la tarjeta, el contacto se cierra al blindaje (GND) y la señal baja a 0.
9. Validate.

---

## 5. Hoja `50_Audio_Codec`

**Qué hace:** ESP32 (I²S) → DAC PCM5102A → filtro RC → amplificador TPA6138A2 → jack. Los dos chips trabajan centrados en tierra, así que no hacen falta capacitores grandes de salida.

Ten abiertos **TI SLAS859C** (PCM5102A) y **SLOS704B** (TPA6138A2) en la figura de aplicación típica.

### Piezas (aprox.)

| Cant. | Pieza |
|---|---|
| 1 | `PCM5102APWR` (MP3_RevA) |
| 1 | `TPA6138A2PWR` (MP3_RevA) |
| 1 | `SJ-3523-SMT` (MP3_RevA) |
| ~5 | 10 µF |
| ~5 | 100 nF |
| ~5 | `Cap_1uF_0603_50V` / `Cap_2.2uF_0603_10V` (MP3_RevA) **(verificar)** |
| 2 | `Cap_2.2nF_0603_50V` (MP3_RevA, filtro) |
| 2 | `Res_470R_0603_1/10W` (MP3_RevA, filtro) |
| 4 | R de ganancia |
| 2 | `Res_10k_0603_1/10W` (shared_lib) / `Res_100k_0603_1/10W` (MP3_RevA) (pull-downs de mute) |

### Bloque A: DAC PCM5102A

1. Alimentación: pines **20 (DVDD)**, **8 (AVDD)** y **1 (CPVDD)** → `+3V3`. **Cada uno** con su 10 µF y su 100 nF a `GND`, pegados al pin.
2. Pines **19 (DGND)**, **9 (AGND)** y **3 (CPGND)** → `GND`.
3. I²S:
   - pin **13 (BCK)** → Port `I2S_BCK` (*Input*)
   - pin **15 (LRCK)** → Port `I2S_LRCK` (*Input*)
   - pin **14 (DIN)** → Port `I2S_DIN` (*Input*)
4. Pin **12 (SCK)** → `GND`. Así el DAC genera su reloj a partir de BCK y el ESP32 no necesita MCLK.
5. Configuración por pines:
   - **16 (FMT)** → `GND` (formato I²S)
   - **11 (FLT)** → `GND` (filtro normal)
   - **10 (DEMP)** → `GND` (sin de-énfasis)
6. Pin **17 (XSMT)** → Port `DAC_XSMT` (*Input*), con 10 kΩ a `GND` para que arranque en mute.
7. Bomba de carga y LDO interno:
   - entre pin **2 (CAPP)** y pin **4 (CAPM)**: 2,2 µF **(verificar)**
   - pin **5 (VNEG)** → 2,2 µF a `GND` **(verificar)**
   - pin **18 (LDOO)** → 1 µF a `GND` **(verificar)**
8. Salidas: pin **6 (OUTL)** → 470 Ω → nodo `DAC_L`, con 2,2 nF de `DAC_L` a `GND`. Haz lo mismo con el pin **7 (OUTR)** → `DAC_R` **(verificar valores)**.

### Bloque B: amplificador TPA6138A2

9. Pin **9 (VDD)** → `+3V3`, con 1 µF y 100 nF a `GND` **(verificar)**. Pines **4** y **10 (GND)** → `GND`.
10. Bomba de carga: 1 µF entre pin **8 (CP)** y pin **7 (CN)**, y pin **6 (VSS)** → 1 µF a `GND` **(verificar)**.
11. Canal izquierdo (inversor):
    - `DAC_L` → R_in → pin **13 (−INL)**
    - R_f del pin **12 (OUTL)** al pin **13**
    - pin **14 (+INL)** → `GND`, según la figura de aplicación
12. Canal derecho: igual, con `DAC_R` → R_in → pin **2 (−INR)**, R_f del pin **3 (OUTR)** al pin **2**, y pin **1 (+INR)** → `GND`.
13. **Ganancia:** G = −R_f / R_in. El DAC entrega 2,1 Vrms (≈ 3 V de pico) y con 3,3 V el amplificador puede recortar. Empieza con R_in = 20 kΩ y R_f = 10 kΩ (G = −0,5) **(verificar)**.
14. Pin **5 (MUTE_N)** → Port `AMP_MUTE_N` (*Input*), con 100 kΩ a `GND` para que arranque en mute **(verificar)**.
15. Pin **11 (UVP)**: revisa en el datasheet el divisor que fija el umbral de bajo voltaje, o cómo dejarlo si no lo usas **(verificar)**.

### Bloque C: jack

16. Jack J?:
    - pin **2 (punta)** → pin 12 (OUTL) del amplificador
    - pin **3 (anillo)** → pin 3 (OUTR)
    - pin **1 (manga)** → `GND`
17. Validate.

---

## 6. Hoja `60_User_Interface`

Los 5 pulsadores ya están colocados. La huella está verificada: pads 1,1,2,2.

### Pasos

1. Pin **2** de cada pulsador → `GND`.
2. Pin **1** de cada pulsador → su Port (*Output*):

| Pulsador | Port |
|---|---|
| Arriba / vol + | `BTN_UP_N` |
| Abajo / vol − | `BTN_DOWN_N` |
| Play / pausa | `BTN_PLAY_N` |
| Anterior | `BTN_PREV_N` |
| Siguiente | `BTN_NEXT_N` |

3. Cada señal lleva 10 kΩ a `+3V3` (o usa los pull-ups internos del ESP32 y quítalos). Opcional: 100 nF a `GND` como antirrebote.
4. **OLED SSD1306:** header `H_Male_4x1_TH_100mil` con `GND`, `+3V3`, `OLED_SCL` (Port *Input*) y `OLED_SDA` (Port *Bidirectional*).
   **Revisa el orden de los pines en tu módulo**, porque cambia según el fabricante. Si el módulo no trae pull-ups I²C, pon 4,7 kΩ a `+3V3` en SDA y en SCL.
5. Validate.

---

## 7. Hoja `00_System_Top`

Esta hoja se hace al final, cuando todas las hojas hijas ya tienen sus Ports.

1. Clic derecho sobre cada sheet symbol → **Sheet Symbol Actions → Synchronize Sheet Entries and Ports**. Altium crea una Sheet Entry por cada Port de esa hoja.
2. Ordena las entries: las entradas a la izquierda del símbolo y las salidas a la derecha.
3. Une con **cable** las entries que tienen el mismo nombre:

| Señal | Desde | Hasta |
|---|---|---|
| `USB_DP`, `USB_DN` | U_30_USB_Interface | U_20_MCU_Core |
| `BAT_ADC`, `VBUS_DET` | U_10_Power_Management | U_20_MCU_Core |
| `I2S_BCK`, `I2S_LRCK`, `I2S_DIN`, `DAC_XSMT`, `AMP_MUTE_N` | U_20_MCU_Core | U_50_Audio_Codec |
| `SD_CS`, `SD_MOSI`, `SD_SCK` | U_20_MCU_Core | U_40_Storage |
| `SD_MISO`, `SD_CD_N` | U_40_Storage | U_20_MCU_Core |
| `BTN_*_N` (5) | U_60_User_Interface | U_20_MCU_Core |
| `OLED_SDA`, `OLED_SCL` | U_20_MCU_Core | U_60_User_Interface |

4. Si hay muchas señales, puedes poner un Net Label en el cable de cada entry en lugar de tirar el cable completo. En la hoja top eso es válido, porque las entries están en la misma hoja.
5. Validate.

---

## 8. Cierre: anotación, ERC y commit

- [ ] Tools → Annotation → **Annotate Schematics** → *Update Changes List* → *Accept Changes*.
- [ ] Project → **Validate PCB Project**: 0 errores. Revisa uno por uno los avisos que queden.
- [ ] No quedan pines flotando sin No ERC: SBU, NC del LDO, STDBY del TP4056, IO35–IO37, GPIO libres.
- [ ] Están confirmados los valores **(verificar)** de audio.
- [ ] Están confirmadas la polaridad de la batería y el orden de pines del OLED.
- [ ] Decidiste la corriente de carga del TP4056 según tu batería (4,7 kΩ ≈ 255 mA o 3,3 kΩ ≈ 364 mA).
- [ ] No quedó colocada ninguna pieza retirada en rev 2 (`USB4105`, `MCP73831`, `AO3401A`, `B2B-PH-K-S`).
- [ ] Commit en `hardware/rev-a-altium`.

### Errores típicos de Validate y qué significan

| Mensaje | Causa habitual |
|---|---|
| *Floating Net Label* / *Net label not connected* | El label no toca el cable. Muévelo hasta que el punto caliente quede sobre el cable |
| *Unconnected pin* | Falta el cable o el No ERC |
| *Nets with only one pin* | Un Port o label sin pareja: revisa que el nombre esté escrito igual en las dos hojas |
| *Duplicate net names* / *Port not linked to sheet entry* | Falta sincronizar las entries en `00_System_Top` |
| *Pin type conflict* (Output–Output) | Dos ports Output en la misma net: corrige la dirección de uno de ellos |
| *Duplicate designators* | Falta anotar |
