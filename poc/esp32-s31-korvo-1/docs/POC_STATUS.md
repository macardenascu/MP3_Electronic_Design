# Cierre técnico de la primera etapa

## Objetivo

Demostrar el arranque de la aplicación, el montaje de microSD por medio del BSP Korvo y el inventario determinista de archivos `.mp3` en `MUSIC`. Esta etapa no reproduce audio.

## Plataforma y arquitectura

- **Placa de PoC:** ESP32-S31-Korvo-1, BSP oficial `espressif/esp32_s31_korvo_1` 1.0.1.
- **Entorno:** ESP-IDF 6.1, target preliminar `esp32s31` con `--preview`, GCC RISC-V 15.2.0.
- `app_main.c` gestiona el diagnóstico y llama al adaptador de almacenamiento y al catálogo.
- `storage_korvo.c` encapsula las llamadas BSP para montar la SD y obtener su punto de montaje.
- `music_catalog.c` es portable y no depende del BSP. Escanea solo el directorio inmediato `MUSIC`, acepta `.mp3` en cualquier combinación ASCII de mayúsculas, ignora directorios y otros tipos, comprueba que los archivos se puedan abrir/leer y ordena por nombre sin distinguir ASCII mayúsculas, con desempate sensible a mayúsculas.
- El catálogo tiene capacidad fija de 256 nombres de hasta 255 bytes. Si el recorrido o un acceso falla, el escaneo devuelve error y no publica un catálogo parcial.

## Estado por tipo de evidencia

### Implementado

Montaje solicitado al BSP, búsqueda de `MUSIC`, filtro y orden del catálogo, límite de capacidad y mensajes de diagnóstico por UART. Los estados/reproductor y API de audio son solo marcadores; no implementan reproducción.

### Probado en PC

Comando ejecutado desde PowerShell:

```powershell
$env:PATH = 'C:\msys64\usr\bin;' + $env:PATH
gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main main/music_catalog.c tests/host/music_catalog_test.c -o build-host/music_catalog_test.exe
.\build-host\music_catalog_test.exe
```

Resultado: compilación host limpia con `-Werror` y salida `music_catalog tests passed`. Se verifican `MUSIC` inexistente, archivo en lugar de directorio, carpeta vacía, extensiones mixtas, extensiones ajenas, subdirectorio con sufijo `.mp3`, orden determinista, nombre de 244 bytes, capacidad exacta de 256 y rechazo de la entrada 257 con catálogo parcial descartado.

La suite no puede verificar de forma portable que el sistema operativo deniegue la lectura de un archivo: Windows puede ejecutar con permisos elevados y `chmod` no ofrece una garantía equivalente. El escáner sí comprueba `stat`, `fopen`, `fread`, `fclose` y errores de recorrido; falta una prueba host fiable de archivo inaccesible. Tampoco se ha probado una extracción de tarjeta durante el escaneo.

### Compilado para ESP32-S31

El estado local previo contiene `build_new/audio_player_poc.bin` de 319360 bytes (`0x4df80`), compatible con el resultado informado de 2153/2153 pasos y cerca de 70 % libre en la partición de aplicación. El manifiesto de build identifica ESP-IDF 6.1, target `esp32s31` y el compilador RISC-V GCC 15.2.0. Esta compilación no demuestra funcionamiento en placa.

En esta auditoría, la suite host volvió a compilar y pasar. Se intentó repetir `idf.py --preview -B build_new build`, pero el script de activación falló antes de ejecutar IDF: `esp-clangd` produce `WinError 5 Acceso denegado`; por ello no se obtuvo un build nuevo en esta sesión. El binario existente y el build previo permanecen sin modificaciones. No se ejecutaron flash ni monitor.

## Pendiente de validación física

- Montaje FAT32 en la Korvo 1 y ruta real que entrega el BSP.
- Acceso real a archivos/nombres largos y listado UART.
- Errores de montaje, tarjeta ausente o extracción durante el acceso.
- El hardware final ESP32-S3 + ES8388 + microSD SPI requiere validación separada; la Korvo usa ESP32-S31, ES8389 y SDMMC.

## No implementado

Decodificación y reproducción MP3, PCM, WAV, controles, pantalla, USB MSC, Bluetooth y recuperación automática tras pérdida de la SD.

## Siguiente etapa

Validar en hardware la ruta de salida PCM generando un tono PCM, y luego implementar reproducción WAV PCM. Tras validar WAV, abordar la decodificación MP3. La ruta de audio del BSP Korvo y sus características no deben asumirse equivalentes al ES8388 del producto final.

## Cierre

La etapa cierra como PoC de arranque, montaje solicitado e inventario de archivos, con evidencia de host y compilación previa, pero sin validación física. Puede integrarse como una etapa acotada en un repositorio mayor, conservando explícitas estas limitaciones.
