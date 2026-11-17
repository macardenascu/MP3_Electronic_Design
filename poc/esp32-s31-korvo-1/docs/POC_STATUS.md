# Estado de la PoC ESP32-S31-Korvo-1

## Alcance

Esta etapa valida el montaje microSD y el inventario inmediato de archivos MP3 en `MUSIC` mediante el BSP de la Korvo. No implementa reproducción. El hardware de prueba usa ESP32-S31, ES8389 y SDMMC; el producto objetivo ESP32-S3, DAC PCM5102A + amplificador de audífonos TPA6138A2 y microSD SPI requiere validación independiente.

El catálogo portable acepta `.mp3` sin distinguir mayúsculas ASCII, ignora directorios y otros archivos, comprueba el acceso de lectura, ordena determinísticamente y admite 256 entradas con nombres de hasta 255 bytes. Ese máximo del catálogo se mide en bytes; `FATFS_MAX_LFN=255` es el límite de caracteres configurado en FATFS y no amplía el búfer del catálogo.

## Evidencia separada por tipo

### Pruebas host anteriores

Las pruebas host compilan `music_catalog.c` con GCC C11 y un shim de `esp_err.h`. Cubren `MUSIC` inexistente o no-directorio, directorio vacío, extensiones `.mp3` mixtas, archivos ajenos, subdirectorios, ordenamiento, nombre largo, capacidad 256 y rechazo de 257. En esta actualización se añade una falla inyectada de `opendir()` después de que `stat()` confirmó el directorio. Verificación actual: GCC C11 con `-Wall -Wextra -Werror`; resultado `music_catalog tests passed`.

### Compilaciones ESP-IDF anteriores

Se conserva como antecedente la compilación registrada de ESP-IDF 6.1 en modo preview, target `esp32s31`, BSP `espressif/esp32_s31_korvo_1` 1.0.1 y GCC RISC-V 15.2.0: 2153/2153 pasos, binario de aplicación de 319360 bytes (`0x4df80`) y aproximadamente 70 % libre en la partición de aplicación. Es evidencia histórica, no una compilación posterior a los cambios de esta actualización.

### Pruebas físicas sobre la placa

El usuario ejecutó el firmware en una ESP32-S31-Korvo-1 V1.1 con microSD insertada. El registro entregado confirma montaje en `/sdcard`, directorio `MUSIC` detectado y tres archivos MP3 listados alfabéticamente. Se conserva el fragmento original:

```text
I (285) main_task: Calling app_main()
[PLAYER] Starting...
[PLATFORM] ESP32-S31-Korvo-1
W (285) ESP32-S31-Korvo-1: Warning: Long filenames on SD card are disabled in menuconfig!
I (295) SD_HOST: src_freq_hz: 500000000
[SD] Mounted successfully at /sdcard
[MUSIC] Directory found
[MUSIC] Tracks: 3
[TRACK 001] Baby_Im_Yours.mp3
[TRACK 002] Formidable.mp3
[TRACK 003] Idilio.mp3
[PLAYER] Ready
I (345) main_task: Returned from app_main()
```

Esto valida SD-01 para ese caso concreto. También se recibieron resultados posteriores del usuario para SD-02 a SD-07. SD-02 (MUSIC vacía), SD-03 (MUSIC ausente) y SD-04 (sin microSD, error `ESP_ERR_TIMEOUT`) se reportaron como PASS. Para SD-05 y SD-06 el usuario confirma el contenido preparado (archivos `.txt` y subdirectorios) y que el catálogo solo mostró MP3 válidos del directorio principal; son PASS por confirmación física del usuario, aunque el UART equivalente al de SD-01 no muestra por sí mismo qué entradas se excluyeron. Para SD-07 se reportó que el UART presentó completo `La_Mujer_que_Bota_Fuego_nombre_mas_largo.mp3`, por lo que el resultado funcional es PASS. No se encontró en el checkout `Pruebas PoC.md` ni los logs adicionales; se distingue aquí la confirmación del usuario de la evidencia UART disponible.

SD-08 (extensiones `.mp3`, `.MP3`, `.Mp3`) queda aplazada por decisión del responsable del proyecto; no es PASS físico. La comparación case-insensitive tiene cobertura host. La advertencia de nombres largos está presente en el primer log. No se dispone del log de la ejecución SD-07 posterior para confirmar si la advertencia desapareció; esto queda como verificación pendiente y no invalida el resultado funcional del nombre largo reportado.

### Verificación de configuración LFN para este cambio

Se inspeccionó `C:\Espressif\v6.1\esp-idf\components\fatfs\Kconfig` de la instalación ESP-IDF 6.1. Allí `FATFS_LFN_HEAP` es una opción oficial de `FATFS_LONG_FILENAMES` («Long filename buffer in heap»). `FATFS_MAX_LFN` depende de que LFN no esté deshabilitado, tiene rango 12–255 y valor predeterminado 255. `sdkconfig.defaults` mantiene el target `CONFIG_IDF_TARGET="esp32s31"` y fija heap y 255. La configuración efectiva disponible en `sdkconfig` y `build_hw_sd/config/sdkconfig.h` también muestra `CONFIG_FATFS_LFN_HEAP=y` y `CONFIG_FATFS_MAX_LFN=255`. La configuración generada confirma esas opciones, pero no se pudo asociar al firmware de la ejecución SD-07 ni comprobar si la advertencia desapareció.

### Compilación actual de firmware

No se pudo iniciar un build nuevo. La activación de la instalación local ESP-IDF 6.1 falla antes de CMake porque falta la herramienta `esp-clangd` (`Tool esp-clangd not found`, WinError 2). No se instaló ni modificó ninguna herramienta global. Por tanto, los valores de `sdkconfig.defaults` se contrastaron con el Kconfig instalado, pero aún no se verificó el `sdkconfig` resultante de un build limpio ni se generó firmware actualizado.

### Pruebas físicas pendientes

La matriz y los tipos de evidencia por escenario están en [SD_VALIDATION.md](SD_VALIDATION.md). El hito de almacenamiento se acepta y cierra dentro del alcance probado SD-01 a SD-07, con SD-08 expresamente aplazada y sin PASS físico. La ausencia del warning LFN en la ejecución posterior sigue pendiente de evidencia.

## Corrección del contrato del catálogo

El contrato documentado es que cualquier error deja el catálogo en cero. Antes, `directory_found` se marcaba tras `stat()` y antes de `opendir()`, con lo que el error de apertura dejaba metadatos parciales. Ahora se marca solo tras abrir correctamente. La prueba host inyecta una falla de apertura para probar el caso de forma determinista; no depende de permisos del usuario del sistema operativo.

## Estado y límites

- **Host:** pruebas ejecutadas y pasadas, incluida regresión de fallo `opendir()`.
- **ESP-IDF:** compilado con ESP-IDF 6.1 en `poc/esp32-s31-korvo-1/build_s31` en modo STORAGE y TONE_BSP; el binario flasheado para la prueba audible de E2.2 es el de `build_s31`. `build_tonebsp` fue una verificación previa de compilación del mismo commit. Ver E2.
- **Hardware:** hito aceptado dentro del alcance SD-01 a SD-07 según log o confirmación del usuario. SD-08 aplazada por decisión del responsable; no es PASS físico. El pendiente LFN de SD-07 se cierra en E2.1: el aviso es un falso positivo del BSP 1.0.1 (ver sección E2).
- No se ha demostrado extracción durante lectura, resistencia a fallas de medio ni funcionamiento del hardware final.
- **E2.1** regresión de almacenamiento: PASS (placa). **E2.2** tono por ruta BSP: PASS (placa, observación auditiva del responsable). La ruta TDM (`audio_korvo.c`, plan B) no está validada y no es necesaria por ahora. No hay lectura WAV, decodificación MP3, OLED, botones, USB MSC o Bluetooth.

## Ruta TDM ES8389 (plan B): compila, sin prueba en placa

La inspección local confirmó ESP-IDF 6.1, BSP `espressif/esp32_s31_korvo_1` 1.0.1 y `esp_codec_dev` 1.5.11. El BSP inicializa I2S0 con `i2s_channel_init_std_mode()` y su helper `bsp_audio_codec_speaker_init()` hereda esa configuración. En el driver ES8389 local, `set_fs()` llama a `es8389_config_fmt(..., ES_I2S_NORMAL)`. Por ello el helper BSP no se usa para la prueba TDM.

ESP-IDF 6.1 ofrece `driver/i2s_tdm.h`. El adaptador local `audio_codec_new_i2s_data()` reconoce un handle TDM y al abrirlo configura reloj, total de slots, máscara y framing Philips. La API pública `esp_codec_dev_write_reg()` deja corregir el registro de formato después de `esp_codec_dev_open()`, que vuelve a seleccionar I2S normal. La prueba configura I2S0 TX en TDM PCM-short de cuatro slots de 16 bits a 16 kHz (BCLK calculado 1.024 MHz), ES8389 en DSP/PCM modo A y DAC S2P desde slot 0. Reaplica PCM-short después de `open()` porque el adaptador 1.5.11 lo cambia a framing TDM Philips. El tono se copia con amplitud reducida a los cuatro slots para que la prueba no dependa de asumir el enrutamiento de slots estéreo DAC no descrito con suficiente detalle en el BSP.

El test solo reproduce por la línea `DSDIN` (GPIO5) del DAC; no configura la entrada adicional `TDMIN` que puede multiplexarse en el pin MCLK del ES8389. Usa el códec en modo esclavo y mantiene MCLK GPIO desconectado según el BSP; el códec deriva su reloj de referencia interna de SCLK conforme a la configuración oficial local. El volumen inicial configurado es 20 % y la amplitud PCM máxima 8 % de escala completa. El driver maneja PA en GPIO7 y debe apagarlo al cerrar el dispositivo.

El firmware ofrece `CONFIG_POC_MODE_STORAGE` (predeterminado) y `CONFIG_POC_MODE_PCM`. La nueva ruta genera una senoide de 440 Hz, 16 kHz, PCM signed 16-bit, tres segundos y rampas lineales de 100 ms. El UART confirma operaciones de software y registros leídos; no puede confirmar que el tono sea audible. El código todavía necesita una compilación local con ESP-IDF y una ejecución física con altavoz antes de aceptar la ruta.

La implementación actual usa las señales del BSP (BCLK GPIO3, WS GPIO4, DOUT GPIO5; sin salida MCLK) y mantiene el canal I2S0 en READY para que `esp_codec_dev` pueda reconfigurar TDM durante `open()`. Después de abrir se fuerza/verifica el formato DSP-A del ES8389 por la interfaz pública de registros y se devuelve el canal a PCM-short antes de transmitir. Se usa un frame de cuatro slots de 16 bits; todos los slots reciben la misma muestra y el DAC S2P se selecciona en slot 0. Esta elección resuelve la discrepancia de formato entre el helper estándar del BSP/driver ES8389 y TDM, pero requiere confirmación física.

`audio_korvo.c` forma parte de `SRCS` y se compila dentro de los builds de E2.1 y E2.2 (aparece `audio_korvo.c.obj`), pero el modo `CONFIG_POC_MODE_PCM` nunca se ha ejecutado en la placa.

Nota histórica: en una sesión anterior (Codex) no se pudo iniciar la compilación porque el export de ESP-IDF 6.1 informó que faltaba `C:\Users\mateo\.espressif\python_env\idf6.1_py3.14_env\Scripts\python.exe`.

## E2 — Salida de audio (ruta BSP)

### E2.1 Regresión de almacenamiento — PASS (placa)

- Build en `poc/esp32-s31-korvo-1/build_s31`, ESP-IDF 6.1 `--preview`, target `esp32s31`: binario `0x4df80`, 70 % libre.
- Log de placa con App version `rev-a-5-g1a0203f`: SD montada, `MUSIC` con 4 pistas (incluido el nombre largo).
- LFN: el aviso "Long filenames on SD card are disabled" es un falso positivo del BSP 1.0.1. `src/bsp_storage.c` comprueba `CONFIG_FATFS_LONG_FILENAMES`, que no existe en nuestro `sdkconfig`; el `sdkconfig` tiene `CONFIG_FATFS_LFN_HEAP=y` y `CONFIG_FATFS_MAX_LFN=255`, y el nombre largo se muestra completo. Con esto se cierra el pendiente de LFN de SD-07.

### E2.2 Tono por ruta BSP — PASS (placa, observación auditiva del responsable)

- Commit `ea8a0d7`. Modo `CONFIG_POC_MODE_TONE_BSP`. Build 2156/2156 sin warnings, binario `0x4a150`, 71 % libre.
- `bsp_audio_codec_speaker_init()` + `esp_codec_dev` 1.5.11, 16 kHz, 16 bits, estéreo, 440 Hz, pico 25 %.
- Escalera de volumen 30/45/60: los tres audibles, volumen creciente, sin distorsión, clics ni pops, tono estable. Altavoz pasivo 4 Ω / 3 W en un conector de la Korvo.
- Log: "Open codec device OK", tones 1/3..3/3, "BSP tone test finished"; duración 353 ms -> 8413 ms (8,06 s) frente a 8 s esperados, coherente con un reloj I2S correcto.
- Prueba host `tone_gen_test`: OK (440 cruces por cero ascendentes en 1 s), ejecutada en Linux gcc 13.3.
- No probado: frecuencia medida con instrumento, música real, ruta PCM5102A del hardware final.
- Decisión: la ruta TDM (`audio_korvo.c`, plan B) no es necesaria por ahora; se conserva como referencia.

## Próxima acción

E3 — reproducir un WAV PCM desde `/sdcard/MUSIC` por la ruta BSP.
