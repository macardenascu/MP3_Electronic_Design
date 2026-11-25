# Presupuesto de potencia y autonomía de la Rev A (estimado)

Estado (2026-11-14): **estimado con valores típicos de los datasheets, no medido.** Validarlo en la placa ensamblada
(ver "Cómo validar" más abajo) y actualizar este archivo con los valores medidos.

## Resumen del camino de energía

- Con USB, `VSYS` sale de `VBUS` a través del Schottky; el P-MOSFET AP2301 desconecta la batería de la carga
  (Microchip AN1149), así que el TP4056 carga la batería mientras el sistema funciona desde el USB.
- Con batería, `VSYS ≈ VBAT` y el LDO AP2112K-3.3 alimenta `+3V3`. Con un regulador lineal, la corriente de la batería
  es prácticamente igual a la corriente de carga en `+3V3` (más los 55 µA de reposo del LDO).
- Todo en la placa funciona desde `+3V3`; nada se alimenta directamente de `VBAT`, salvo el divisor del ADC.

## Presupuesto de carga en +3V3 (reproduciendo, Wi-Fi y Bluetooth apagados)

| Bloque | Pieza | Típico | Pico / peor caso | Base |
|---|---|---|---|---|
| MCU | ESP32-S3-WROOM-1-N16R8 | 40–60 mA | ~70 mA | Datasheet, modem-sleep (radio apagada): 20–28 mA a 240 MHz con la CPU en reposo; la decodificación MP3 con ambos núcleos y acceso a flash y PSRAM suma a eso (estimado) |
| DAC | PCM5102A | 30 mA | 45 mA | TI SLAS859C, 3.3 V, 48 kHz, 1 kHz a −1 dBFS: DVDD 8 mA + AVDD/CPVDD 22 mA (máx. 13 + 32) |
| Amplificador de audífonos | TPA6138A2 | 14 mA | 25 mA | TI SLOS704B, corriente de alimentación sin carga (máx. 25 mA) |
| Carga de los audífonos | 2 × 32 Ω | ~4 mA | ~20 mA | ~1 mW por canal a nivel de escucha; peor caso cerca del límite de ~40 mW (el firmware limita el volumen a unos −12 dB) |
| Pantalla | Módulo OLED SSD1306 de 0.96" | ~10 mA | ~20 mA | Valor típico de módulo; depende de los píxeles encendidos y del contraste |
| Almacenamiento | microSD en modo SPI | ~10 mA promedio | ~100 mA en ráfaga | Las lecturas son ráfagas cortas a las tasas de bits del MP3; depende de la tarjeta |
| Regulador | AP2112K-3.3 | 0.055 mA | — | Corriente de reposo, Diodes DS39724 |
| Divisores y pull-ups | 390k/390k, pull-ups de 10k | < 0.01 mA | — | Divisor de VBAT ≈ 5 µA a 4.2 V; los pull-ups de los botones solo consumen mientras se pulsan |
| **Total** | | **≈ 110–120 mA** | **≈ 280 mA** | |

Pantalla apagada (o atenuada) y CPU a 160 MHz: **≈ 85–95 mA**.

Comprobaciones frente al diseño:
- AP2112K de 600 mA → el peor caso de ≈ 280 mA deja más de 2× de margen.
- Disipación del LDO con batería: (4.2 V − 3.3 V) × 0.12 A ≈ 0.11 W típico, ≈ 0.25 W en el peor caso. Aceptable para
  un SOT-23-5 con el relleno de cobre a su alrededor.
- Corriente de carga del TP4056 (R_PROG 4.7 kΩ) ≈ 255 mA. Con USB la carga se alimenta de `VBUS`, así que la carga de
  la batería y la reproducción no comparten esos 255 mA.

## Autonomía de la batería

Se toma como capacidad útil el **85 %** de la nominal: el AP2112K deja de regular 3.3 V cuando la batería baja a
unos 3.5 V, y la protección de la celda corta cerca de 3.0 V.

Autonomía ≈ capacidad × 0.85 / corriente.

| Batería (Li-Po 1S, con protección) | Reproduciendo, pantalla encendida (≈ 115 mA) | Pantalla apagada, 160 MHz (≈ 90 mA) | Tiempo de carga a 255 mA |
|---|---|---|---|
| 1000 mAh (p. ej. 803040) | ≈ 7.5 h | ≈ 9.5 h | ≈ 4.5–5 h |
| 1200 mAh (p. ej. 603450) | ≈ 9 h | ≈ 11 h | ≈ 5–5.5 h |
| 2000 mAh (p. ej. 103450) | ≈ 15 h | ≈ 19 h | ≈ 8.5–9 h |

Tiempo de carga ≈ capacidad / 255 mA, más unos 0.5–1 h de la etapa final a voltaje constante.

Recursos del firmware que alargan la autonomía: apagar la OLED tras unos segundos sin uso, correr la CPU a 160 MHz o
menos si la decodificación lo permite, mantener Wi-Fi y Bluetooth apagados, y silenciar el amplificador (`AMP_MUTE_N`)
y el DAC (`DAC_XSMT`) en pausa.

## Cómo validar

1. Poner un multímetro en serie con la batería (escala de mA) y medir: en reposo en el menú, reproduciendo con la
   pantalla encendida, reproduciendo con la pantalla apagada y en pausa.
2. Medir `+3V3` en el pin VCC del header de la OLED mientras reproduce, para confirmar que el LDO sigue regulando a
   medida que la batería se descarga.
3. Hacer una prueba de reproducción completa desde carga llena y anotar el tiempo hasta el corte por batería baja del firmware.
4. Reemplazar los estimados de este archivo por los valores medidos.

## Fuentes

- [TI PCM5102A datasheet (SLAS859C), especificaciones](https://www.ti.com/document-viewer/PCM5102A/datasheet/specifications)
- [TI TPA6138A2 datasheet (SLOS704B), especificaciones](https://www.ti.com/document-viewer/TPA6138A2/datasheet/specifications)
- [ESP32-S3-WROOM-1 datasheet (Espressif)](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- Datasheet del Diodes AP2112 (DS39724), corriente de reposo
