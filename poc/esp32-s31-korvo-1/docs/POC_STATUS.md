# Estado de la PoC ESP32-S31-Korvo-1

## Alcance

La primera etapa es una PoC de montaje de almacenamiento e inventario de música. Solicita el montaje SD al BSP Korvo y recorre el directorio inmediato `MUSIC` para identificar archivos `.mp3`. No es una prueba de reproducción.

La lógica portable está en `music_catalog.c`; `storage_korvo.c` encapsula el BSP y `app_main.c` presenta diagnósticos por consola. El catálogo tiene capacidad fija de 256 nombres de hasta 255 bytes, acepta extensiones ASCII `.mp3` sin distinguir mayúsculas, ignora directorios y otros tipos, comprueba acceso a archivos y ordena por nombre de forma determinista. Ante error descarta toda la salida del escaneo.

La Korvo 1 utiliza ESP32-S31, códec ES8389 y SDMMC. No valida la arquitectura final propuesta de ESP32-S3-WROOM-1-N16R8, ES8388 por I²S y microSD por SPI.

## Estado por evidencia

### Implementado

- Llamada al montaje SD de BSP y obtención de su punto de montaje.
- Búsqueda de `MUSIC`, filtrado y ordenamiento de archivos, comprobación de errores y límite fijo de 256 entradas.
- Mensajes de diagnóstico por UART.

La API de audio y algunos tipos de reproductor en el código son marcadores; no implementan reproducción.

### Probado en PC

Comando PowerShell ejecutado desde `poc/esp32-s31-korvo-1`:

```powershell
$env:PATH = 'C:\msys64\usr\bin;' + $env:PATH
New-Item -ItemType Directory -Force build-host | Out-Null
gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main main/music_catalog.c tests/host/music_catalog_test.c -o build-host/music_catalog_test.exe
if ($LASTEXITCODE -ne 0) { throw 'Falló la compilación host' }
& .\build-host\music_catalog_test.exe
if ($LASTEXITCODE -ne 0) { throw 'Fallaron las pruebas host' }
```

La suite debe terminar con `music_catalog tests passed`. Cubre directorio inexistente, `MUSIC` como archivo, directorio vacío, extensiones mixtas, archivos no MP3, subdirectorios, ordenamiento, nombre largo, capacidad exacta de 256 y rechazo de la entrada 257. La regresión de capacidad comprueba que un escaneo fallido no deje `directory_found` activado ni entradas parciales.

Las pruebas de permisos denegados no son portables cuando el proceso corre con privilegios; tampoco simulan extracción de tarjeta durante un escaneo.

### Compilado para ESP32-S31

La evidencia registrada en la etapa previa incluye:

- ESP-IDF 6.1, modo preview, target `esp32s31`, BSP `espressif/esp32_s31_korvo_1` 1.0.1 y GCC RISC-V 15.2.0.
- Compilación posterior al cambio de nombre del repositorio: 2153/2153 pasos completados.
- Binario de aplicación de 319360 bytes (`0x4df80`) y aproximadamente 70 % de espacio disponible en la partición de aplicación.

**Verificación realizada en esta auditoría:** las pruebas host se recompilaron y pasaron, incluida la nueva regresión de salida limpia ante error. No se pudo iniciar un build ESP-IDF nuevo: la activación falló antes de CMake porque `idf_tools.py` no pudo acceder a `esp-clangd` (`WinError 5: Acceso denegado`). También se intentó invocar Ninja directamente sobre un build previo; CMake no pudo regenerarlo porque el entorno `IDF_PATH` no estaba activo. No se modificaron instalaciones globales ni se intentó instalar herramientas. El binario local previamente generado y su evidencia histórica no se alteraron. No se ejecutaron flash ni monitor.

Históricamente, una auditoría anterior tampoco pudo activar ESP-IDF por el error de acceso a `esp-clangd`. Posteriormente sí se obtuvieron compilaciones satisfactorias desde PowerShell, incluida la compilación de 2153/2153 pasos posterior al cambio de nombre del repositorio. En esta auditoría reapareció el bloqueo de activación; por eso el resultado histórico se conserva, pero no se afirma una compilación nueva de los cambios actuales.

### Pendiente de validación física

- Montaje de tarjeta FAT32 en la Korvo y ruta concreta del BSP.
- Acceso y listado real por UART, incluyendo nombres largos.
- Comportamiento con tarjeta ausente, errores de montaje y extracción durante acceso.
- Hardware final y sus interfaces, que requieren validación independiente.

### No implementado

- Decodificación o reproducción MP3, PCM o WAV.
- Pantalla OLED, controles físicos, USB MSC y Bluetooth.
- Recuperación automática tras pérdida de tarjeta.

## Próximo hito

Validar físicamente el montaje SD y el inventario de archivos en ESP32-S31-Korvo-1. Las etapas de audio quedan fuera de esta primera PoC y deben comenzar solo después de cerrar esa validación.
