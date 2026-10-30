# Estado de la PoC ESP32-S31-Korvo-1

## Alcance

Esta etapa valida el montaje microSD y el inventario inmediato de archivos MP3 en `MUSIC` mediante el BSP de la Korvo. No implementa reproducción. El hardware de prueba usa ESP32-S31, ES8389 y SDMMC; el producto objetivo ESP32-S3, ES8388 y microSD SPI requiere validación independiente.

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
- **ESP-IDF:** configuración LFN modificada; falta compilar este estado con ESP-IDF y verificar `sdkconfig` resultante.
- **Hardware:** hito aceptado dentro del alcance SD-01 a SD-07 según log o confirmación del usuario. SD-08 aplazada por decisión del responsable; no es PASS físico. No se conserva en este checkout un log UART posterior que confirme la ausencia del warning LFN.
- No se ha demostrado extracción durante lectura, resistencia a fallas de medio ni funcionamiento del hardware final.
- No hay decodificación ni reproducción MP3, PCM o WAV; no hay OLED, botones, USB MSC o Bluetooth.

## Próxima acción

SD-08 queda fuera del cierre actual por decisión del responsable; la prueba host mantiene cobertura del filtrado case-insensitive. Como verificación futura opcional, registrar un arranque posterior con LFN para comprobar la ausencia del warning. El siguiente hito es la salida de audio PCM, sin que esto equivalga a reproducción WAV/MP3.
