# Manual de conexiones – Reproductor MP3, PCB Rev A

Proyecto Altium: `hardware/altium/MP3_Player/MP3_Player.PrjPcb` · Versión entregada: tag `rev-a` (15 nov. 2026)

Este manual describe cómo está conectada la placa Rev A: qué hace cada bloque, a dónde va cada pin y qué
hay que conectar por fuera para usarla. Las conexiones y valores de este documento corresponden a los
esquemáticos y a la PCB del tag `rev-a`.

---

## 1. Resumen del sistema

```
            USB-C (J2) ──5 V──┬──────────────► Cargador TP4056 (U1) ──► Batería Li-Po (J1)
                │             │                                              │
          D+/D− │             └─► Schottky D1 ─┐        PFET Q1 ◄────────────┘
                │                              ▼          │
         ESD USBLC6 (U4)                     VSYS ◄───────┘
                │                              │
                │                     LDO AP2112K (U2) ◄── EN: J5 (ON/OFF)
                │                              │
                │                            +3V3
                ▼                              │
        ┌───────────────── ESP32-S3-WROOM-1-N16R8 (U3) ─────────────────┐
        │  I²S                 SPI                I²C          GPIO      │
        ▼                       ▼                  ▼             ▼
  DAC PCM5102A (U5)       microSD (J3)       OLED (J6)     5 botones (SW1–SW5)
        │
  Filtro RC 470 Ω / 2.2 nF
        │
  Amplificador TPA6138A2 (U6)
        │
  Jack 3.5 mm (J4) ──► audífonos
```

Placa de 2 capas, 65 × 90.17 mm, FR-4 de 1.6 mm, cobre de 1 oz. La capa Bottom es casi toda un plano de GND.

### Rieles de alimentación

| Riel | Voltaje | De dónde sale | Quién lo usa |
|---|---|---|---|
| `VBUS` | 5 V | USB-C (J2) | Cargador TP4056, Schottky D1, compuerta de Q1, LED de carga, divisor `VBUS_DET`, USBLC6 |
| `VBAT` | 3.0–4.2 V | Batería (J1) | Salida del cargador, drenador de Q1, divisor `BAT_ADC` |
| `VSYS` | ≈ 4.7 V con USB / VBAT sin USB | D1 (con USB) o Q1 (sin USB) | Entrada del LDO U2 y pull-up de `PWR_EN` |
| `+3V3` | 3.3 V | LDO AP2112K (U2) | ESP32, DAC, amplificador, microSD, OLED, pull-ups |
| `GND` | 0 V | — | Común a todo; plano en Bottom |

---

## 2. Conectores para el usuario

| Conector | Serigrafía | Pin | Señal | Notas |
|---|---|---|---|---|
| **J1** | `BATERIA` | 1 | `VBAT` (+) | JST-PH 2.0 mm de 2 pines. **Verificar la polaridad del cable de la batería antes de conectar**: no es estándar entre fabricantes. Usar una Li-Po 1S **con protección**. |
| | | 2 | `GND` (−) | |
| **J2** | `USB-C` | — | `VBUS`, `GND`, `USB_DP`, `USB_DN` | Carga la batería, alimenta la placa y conecta el USB nativo del ESP32 (programación y monitor serie). |
| **J3** | `MICRO SD` | — | Bus SPI | Socket push-push con detección de tarjeta. Las canciones van aquí. |
| **J4** | `AUDIO` | 1 | `GND` (manga) | Jack estéreo de 3.5 mm para audífonos. |
| | | 2 | `OUT_L` (punta) | Canal izquierdo |
| | | 3 | `OUT_R` (anillo) | Canal derecho |
| **J5** | `ON/OFF` | 1 | `PWR_EN` | Header de 2 pines. **Abierto = encendido.** Con un jumper o un interruptor a GND se apaga el regulador de 3.3 V; la batería sigue cargando. |
| | | 2 | `GND` | |
| **J6** | `PANTALLA` | 1 | `GND` | Módulo OLED SSD1306 128×64 I²C. Comprar un módulo con el orden **GND–VCC–SCL–SDA** (algunos rotulan VDD y SCK, es lo mismo). |
| | | 2 | `+3V3` | |
| | | 3 | `OLED_SCL` | Pull-up de 4.7 kΩ en la placa (R14) |
| | | 4 | `OLED_SDA` | Pull-up de 4.7 kΩ en la placa (R13) |
| **J7** | `UART` | 1 | `IO43` (TX del ESP32) | Depuración o programación con un adaptador USB-serial de 3.3 V. Conectar **cruzado**: TX de la placa → RX del adaptador. |
| | | 2 | `IO44` (RX del ESP32) | |
| | | 3 | `GND` | |

### Botones

| Botón | Serigrafía | Señal | GPIO | Función por defecto |
|---|---|---|---|---|
| SW1 | `VOL+` | `BTN_UP_N` | IO15 | Subir volumen / arriba en el menú |
| SW2 | `VOL-` | `BTN_DOWN_N` | IO16 | Bajar volumen / abajo en el menú |
| SW3 | `PLAY/PAUSE` | `BTN_PLAY_N` | IO17 | Reproducir / pausar |
| SW4 | `PREV` | `BTN_PREV_N` | IO18 | Canción anterior |
| SW5 | `NEXT` | `BTN_NEXT_N` | IO21 | Canción siguiente |
| SW6 | `BOOT` | `IO0` | IO0 | Mantener presionado al encender o al soltar RESET para entrar en modo programación |
| SW7 | reset | `EN` | EN | Reinicia el ESP32 |

Los cinco botones de usuario son activos en bajo: cada uno tiene un pull-up de 10 kΩ a `+3V3` (R9–R12, R15)
y al presionarlo la señal va a 0 V. Su función la asigna el firmware, no el hardware.

---

## 3. Mapa de pines del ESP32-S3 (U3)

| Pin del módulo | GPIO | Señal | Dirección | Va a |
|---|---|---|---|---|
| 2 | 3V3 | `+3V3` | — | C7 (100 nF) y C8 (10 µF) |
| 1, 40, 41 (EPAD) | GND | `GND` | — | Plano de GND |
| 3 | EN | `EN` | Entrada | R16 (10 kΩ a `+3V3`), C9 (1 µF a GND), SW7, TP8 |
| 27 | IO0 | `IO0` | Entrada | SW6 (BOOT), TP9 |
| 39 | IO1 | `BAT_ADC` | ADC | Divisor de batería (hoja 10) |
| 38 | IO2 | `VBUS_DET` | ADC | Divisor de USB (hoja 10) |
| 4 | IO4 | `I2S_BCK` | Salida | DAC U5 pin 13 |
| 5 | IO5 | `I2S_LRCK` | Salida | DAC U5 pin 15 |
| 6 | IO6 | `I2S_DIN` | Salida | DAC U5 pin 14 |
| 7 | IO7 | `AMP_MUTE_N` | Salida | Amplificador U6 pin 5 |
| 31 | IO38 | `DAC_XSMT` | Salida | DAC U5 pin 17 |
| 12 | IO8 | `OLED_SDA` | Bidireccional | J6 pin 4 |
| 17 | IO9 | `OLED_SCL` | Salida | J6 pin 3 |
| 18 | IO10 | `SD_CS` | Salida | microSD J3 pin 2 |
| 19 | IO11 | `SD_MOSI` | Salida | microSD J3 pin 3 |
| 20 | IO12 | `SD_SCK` | Salida | microSD J3 pin 5 |
| 21 | IO13 | `SD_MISO` | Entrada | microSD J3 pin 7 |
| 22 | IO14 | `SD_CD_N` | Entrada | microSD J3 pin CD |
| 8 | IO15 | `BTN_UP_N` | Entrada | SW1 |
| 9 | IO16 | `BTN_DOWN_N` | Entrada | SW2 |
| 10 | IO17 | `BTN_PLAY_N` | Entrada | SW3 |
| 11 | IO18 | `BTN_PREV_N` | Entrada | SW4 |
| 23 | IO21 | `BTN_NEXT_N` | Entrada | SW5 |
| 13 | IO19 | `USB_DN` | USB D− | USB-C J2 vía USBLC6 |
| 14 | IO20 | `USB_DP` | USB D+ | USB-C J2 vía USBLC6 |
| 37 | IO43 | TX0 | Salida | J7 pin 1 |
| 36 | IO44 | RX0 | Entrada | J7 pin 2 |

**Pines que no se conectan a propósito**
- IO3, IO45 e IO46 son pines de configuración de arranque (*strapping*); se dejan libres para no alterar el modo de arranque.
- IO35, IO36 e IO37 los usa internamente la PSRAM de 8 MB del módulo N16R8; **no se deben conectar**.
- IO39–IO42, IO47 e IO48 quedan libres como reserva.

La antena del módulo sobresale del borde de la placa para que el cobre no la desafine.

---

## 4. Bloques del circuito

### 4.1 USB-C y protección ESD – hoja `30_USB_Interface`

| Conexión | Componente | Función |
|---|---|---|
| J2 VBUS (A4/B9, B4/A9) → `VBUS` | C10 100 nF a GND | Entrada de 5 V |
| J2 GND (A1/B12, B1/A12) y blindaje → `GND` | — | |
| J2 CC1 (A5) → R17 4.7 kΩ → GND | R17 | Identifica la placa como dispositivo USB (sink). El cargador entrega 5 V. |
| J2 CC2 (B5) → R18 4.7 kΩ → GND | R18 | Una resistencia por cada CC; nunca se unen. |
| J2 DP1/DP2 (A6/B6) → `USB_DP` | — | El conector es reversible: las dos filas se unen. |
| J2 DN1/DN2 (A7/B7) → `USB_DN` | — | |
| J2 SBU1/SBU2 | Sin conectar | No se usan en USB 2.0. |
| U4 USBLC6: pines 1, 6 → `USB_DP`; 3, 4 → `USB_DN`; 5 → `VBUS`; 2 → GND | U4 | Protege D+/D− contra descargas electrostáticas. |

### 4.2 Alimentación – hoja `10_Power_Management`

**Cargador (U1, TP4056)**

| Pin | Conexión | Por qué |
|---|---|---|
| 4 VCC | `VBUS`, C2 10 µF a GND | Entrada de 5 V del USB |
| 5 BAT | `VBAT`, C1 10 µF a GND | Salida hacia la batería (4.2 V máx.) |
| 2 PROG | R3 4.7 kΩ a GND | Corriente de carga ≈ 1200 V / 4.7 kΩ ≈ **255 mA**: suave para baterías de 500 mAh o más |
| 8 CE | `VBUS` | Cargador siempre habilitado cuando hay USB |
| 1 TEMP | GND | Sin termistor (función desactivada) |
| 7 CHRG | Cátodo de D2 (LED); ánodo por R1 1 kΩ a `VBUS` | **LED encendido = cargando**; se apaga al terminar |
| 6 STDBY | Sin conectar | |
| 3 GND | GND | |

**Selección de fuente (load sharing, Microchip AN1149)**

| Componente | Conexión | Función |
|---|---|---|
| D1 Schottky 40 V 1 A | Ánodo `VBUS` → cátodo `VSYS` | Con USB, el sistema se alimenta del USB |
| Q1 AP2301 (P-MOSFET) | G = `VBUS`, S = `VSYS`, D = `VBAT` | Sin USB conduce y conecta la batería al sistema; con USB se apaga y la batería solo carga |
| R2 49.9 kΩ | `VBUS` a GND | Descarga la compuerta al desconectar el USB, para que Q1 conduzca de inmediato |
| C3 10 µF | `VSYS` a GND | Desacople de `VSYS` / entrada del LDO |

**Regulador de 3.3 V (U2, AP2112K-3.3)**

| Pin | Conexión | Por qué |
|---|---|---|
| 1 VIN | `VSYS` (C3) | |
| 5 VOUT | `+3V3`, C4 10 µF y C5 100 nF a GND | Salida de 3.3 V, hasta 600 mA |
| 3 EN | `PWR_EN`: R6 10 kΩ a `VSYS` y J5 pin 1 | Encendido por defecto; J5 a GND lo apaga |
| 2 GND | GND | |
| 4 NC | Sin conectar | |

Se eligió el AP2112K y no el AMS1117 de la librería del curso porque su caída (≈ 250 mV) le permite regular con
la batería; el AMS1117 necesita ≈ 1.1 V de caída y no regularía por debajo de ≈ 4.4 V.

**Medición de batería y de USB**

| Divisor | Conexión | Señal | Valor en el ESP32 |
|---|---|---|---|
| R4 / R8, 390 kΩ / 390 kΩ, C6 100 nF | `VBAT` → nodo → GND | `BAT_ADC` → IO1 | VBAT / 2 (máx. 2.1 V) |
| R5 / R7, 390 kΩ / 390 kΩ | `VBUS` → nodo → GND | `VBUS_DET` → IO2 | 2.5 V con USB, 0 V sin USB |

Resistencias altas para que el divisor de batería consuma solo ≈ 5 µA.

### 4.3 microSD – hoja `40_Storage`

Bus SPI. Alimentación en J3 pin 4 (`+3V3`) con C11 100 nF y C12 10 µF; pin 6 y blindaje (G1–G4) a GND.

| Pin de J3 | Señal | Pull-up 10 kΩ a `+3V3` | GPIO |
|---|---|---|---|
| 2 DAT3/CS | `SD_CS` | R19 | IO10 |
| 3 CMD/MOSI | `SD_MOSI` | R20 | IO11 |
| 5 CLK | `SD_SCK` | — | IO12 |
| 7 DAT0/MISO | `SD_MISO` | R21 | IO13 |
| 8 DAT1 | — | R23 | (no se usa en SPI) |
| 1 DAT2 | — | R24 | (no se usa en SPI) |
| CD | `SD_CD_N` | R22 | IO14 |

El contacto CD se cierra al blindaje (GND) al insertar la tarjeta: **`SD_CD_N` = 0 V significa tarjeta presente.**

### 4.4 Audio – hoja `50_Audio_Codec`

Cadena: ESP32 (I²S) → **DAC PCM5102A** → filtro RC → **amplificador TPA6138A2** → jack.
Los dos chips generan internamente un voltaje negativo (bomba de carga), así que la salida queda centrada en
0 V y no necesita capacitores grandes de acople.

**DAC (U5, PCM5102A)**

| Pin | Conexión | Por qué |
|---|---|---|
| 20 DVDD, 8 AVDD, 1 CPVDD | `+3V3`, cada uno con 100 nF + 10 µF (C14–C19) | Alimentación digital, analógica y de la bomba de carga |
| 19 DGND, 9 AGND, 3 CPGND | GND | |
| 13 BCK / 15 LRCK / 14 DIN | `I2S_BCK` / `I2S_LRCK` / `I2S_DIN` | Audio digital desde el ESP32 |
| 12 SCK | GND | El DAC genera su reloj interno a partir de BCK; no hace falta MCLK |
| 16 FMT, 11 FLT, 10 DEMP | GND | Formato I²S, filtro normal, sin de-énfasis |
| 17 XSMT | `DAC_XSMT`, R32 10 kΩ a GND | Arranca en silencio; el firmware lo activa (IO38 en alto) |
| 2 CAPP – 4 CAPM | C23 2.2 µF entre ellos | Capacitor de la bomba de carga |
| 5 VNEG | C25 2.2 µF a GND | Voltaje negativo interno |
| 18 LDOO | C26 10 µF + C27 100 nF a GND | Regulador interno |
| 6 OUTL / 7 OUTR | R26 / R27 470 Ω → `DAC_L` / `DAC_R`, con C13 / C20 2.2 nF a GND | Filtro paso bajo (≈ 154 kHz) que quita el ruido de alta frecuencia del DAC |

**Amplificador (U6, TPA6138A2)**

| Pin | Conexión | Por qué |
|---|---|---|
| 9 VDD | `+3V3`, C21 100 nF + C22 1 µF | |
| 4, 10 GND | GND | |
| 8 CP – 7 CN | C24 1 µF | Capacitor de la bomba de carga |
| 6 VSS | C28 1 µF a GND | Voltaje negativo interno |
| 13 −INL | `DAC_L` por R29 10 kΩ; realimentación R25 10 kΩ desde OUTL | Configuración inversora, **ganancia −1** |
| 2 −INR | `DAC_R` por R30 10 kΩ; realimentación R28 10 kΩ desde OUTR | Igual para el canal derecho |
| 14 +INL, 1 +INR | GND | Referencia de las entradas |
| 12 OUTL / 3 OUTR | `OUT_L` / `OUT_R` → J4 punta / anillo | Salida a los audífonos |
| 5 MUTE_N | `AMP_MUTE_N`, R31 100 kΩ a GND, TP16 | Arranca en silencio; el firmware lo activa (IO7 en alto). Evita el "pop" al encender |
| 11 UVP | Sin conectar | Pull-up interno: protección de bajo voltaje en su valor por defecto |

Potencia máxima ≈ 25–40 mW por canal en audífonos de 32 Ω. El firmware limita el volumen a ≈ −12 dB para no
saturar el amplificador alimentado a 3.3 V.

### 4.5 Interfaz de usuario – hoja `60_User_Interface`

- Botones SW1–SW5: un terminal a GND y el otro a su señal `BTN_*_N`, con pull-up de 10 kΩ (sección 2).
- OLED en J6: `GND`, `+3V3`, `OLED_SCL` y `OLED_SDA`, con pull-ups I²C de 4.7 kΩ (R13, R14).

### 4.6 Núcleo del ESP32 – hoja `20_MCU_Core`

- Reset: R16 10 kΩ a `+3V3` y C9 1 µF a GND en `EN` dan un arranque limpio; SW7 lleva `EN` a GND.
- Boot: SW6 lleva `IO0` a GND (el pin tiene pull-up interno).
- UART de depuración en J7 (sección 2).

---

## 5. Puntos de prueba

Pads SMD de cobre expuesto en la cara Top, para medir con multímetro u osciloscopio.

| TP | Señal | Valor esperado |
|---|---|---|
| TP1 | `VBUS` | 5 V con USB conectado |
| TP2 | `VBAT` | 3.0–4.2 V según la carga de la batería |
| TP3 | `VBUS_DET` | ≈ 2.5 V con USB, 0 V sin USB |
| TP5 | `VSYS` | ≈ 4.7 V con USB; ≈ VBAT sin USB |
| TP6 | `+3V3` | 3.3 V |
| TP7 | `GND` | Referencia (zona de alimentación) |
| TP8 | `EN` | 3.3 V en funcionamiento; 0 V con RESET presionado |
| TP9 | `IO0` | 3.3 V; 0 V con BOOT presionado |
| TP16 | `AMP_MUTE_N` | 0 V en silencio, 3.3 V reproduciendo |
| TP17 | `GND` | Referencia (zona de audio) |

---

## 6. Primer encendido

1. **Sin batería ni módulos**, conectar el USB-C. Medir TP1 (5 V), TP5 (≈ 4.7 V) y TP6 (3.3 V).
   El LED de carga puede parpadear sin batería; es normal en el TP4056.
2. Medir TP3 ≈ 2.5 V (detección de USB).
3. Desconectar el USB. **Confirmar la polaridad** del cable de la batería con el multímetro y conectarla en J1
   (`BATERIA`, pin 1 = +). Medir TP2 y TP6: 3.3 V con la batería sola.
4. Conectar el USB otra vez: el LED de carga debe encenderse.
5. Conectar el módulo OLED en J6 respetando el orden GND–VCC–SCL–SDA, insertar la microSD y conectar los
   audífonos.
6. Programar el ESP32 por el USB-C. Si no entra en modo programación: mantener **BOOT**, presionar y soltar
   **RESET**, y soltar BOOT. Alternativa: adaptador USB-serial de 3.3 V en J7.

---

## 7. Origen de los componentes

Criterio del curso: usar primero la librería del profesor (`shared_lib`) y justificar cada pieza propia.

**De `shared_lib`:** USB-C (`CON_USB_TYPEC_16P`), USBLC6-2SC6, cargador TP4056, P-MOSFET AP2301, Schottky,
conector JST-PH de batería, LED, pulsadores, headers y los pasivos 0603 genéricos.

**Propios (`MP3_RevA`) y por qué**

| Pieza | Justificación |
|---|---|
| ESP32-S3-WROOM-1-N16R8 | La alternativa del curso (ESP32-C3-WROOM-02) deja 13 GPIO libres y el reproductor usa 19 señales además del USB. El S3 da margen, doble núcleo y 8 MB de PSRAM para el buffer de audio. |
| AP2112K-3.3 | El AMS1117 del curso no regula con batería (caída ≈ 1.1 V). El AP2112K tiene ≈ 250 mV de caída, pin EN y ≈ 55 µA de consumo en reposo. |
| PCM5102A | La librería del curso no tiene DAC de audio. |
| TPA6138A2 | La librería del curso no tiene amplificador de audífonos. |
| GCT MEM2075 | La librería del curso no tiene socket microSD. |
| Same Sky SJ-3523-SMT | La librería del curso no tiene jack de audio. |
| 470 Ω, 100 kΩ, 2.2 µF, 2.2 nF (0603) | Valores que no existen en la librería del curso; son copias de sus plantillas 0603 con la misma huella. |
| Test point `TP` | Pad SMD propio para los puntos de prueba. |

---

## 8. Advertencias

- El TP4056 **no protege la batería** contra sobredescarga ni cortocircuito: usar una Li-Po con circuito de
  protección incorporado.
- La polaridad de los cables JST-PH varía entre fabricantes: medir antes de conectar J1.
- El orden de pines de los módulos OLED varía: conectar solo uno que coincida con GND–VCC–SCL–SDA.
- No conectar nada a IO35–IO37 (PSRAM interna).
- El consumo estimado y la autonomía están en [POWER_BUDGET_REV_A.md](POWER_BUDGET_REV_A.md).
