# Campaña física de almacenamiento SD

Todas las pruebas usan ESP32-S31-Korvo-1 V1.1, el firmware de esta PoC, FAT32 y microSD mediante SDMMC. Apaga y desconecta la placa antes de insertar o retirar la tarjeta. No formatees la tarjeta; conserva respaldo del contenido y evita modificar canciones existentes. Para cada escenario, guarda el log UART completo y anota ID, fecha, firmware/commit, tarjeta y resultado observado.

## Preparación y comandos

Desde PowerShell en la raíz del repositorio, activa la instalación existente y compila en una carpeta nueva (no borra builds previos):

```powershell
. .\poc\esp32-s31-korvo-1\tools\activate_idf.ps1
Set-Location .\poc\esp32-s31-korvo-1
idf.py --preview -B build_lfn build
if ($LASTEXITCODE -ne 0) { throw 'Falló la compilación ESP-IDF' }
if ((Select-String -Path build_lfn\config\sdkconfig.h -Pattern 'CONFIG_FATFS_LFN_HEAP 1','CONFIG_FATFS_MAX_LFN 255').Count -ne 2) { throw 'La configuración generada no confirma FATFS LFN heap y máximo 255' }
idf.py --preview -B build_lfn flash monitor
```

Verifica que el comando de configuración muestre ambas definiciones antes de flashear. Si el puerto no se detecta, añade `-p COMx` a `idf.py`. Sal del monitor con `Ctrl+]`. No se requiere formatear la tarjeta.

Para escenarios con un contenido distinto, apaga y desconecta la Korvo antes de retirar la microSD. Cambia únicamente los archivos de prueba que tú hayas preparado; vuelve a insertar la tarjeta y enciende la placa. No borres ni renombres archivos del usuario.

## Matriz

Para todos los escenarios, PASS requiere que el comportamiento observado coincida con lo descrito, sin reset inesperado ni error fatal. FAIL incluye nombres truncados/alias 8.3, entradas equivocadas, montaje incorrecto o fallo distinto del esperado. El log por sí solo debe permitir identificar escenario y firmware.

| ID | Preparación segura | Comportamiento y UART esperado | PASS / FAIL | Evidencia actual |
|---|---|---|---|---|
| SD-01 | `MUSIC/` con `Baby_Im_Yours.mp3`, `Formidable.mp3`, `Idilio.mp3`. | `[SD] Mounted successfully at /sdcard`, `[MUSIC] Directory found`, `Tracks: 3`; tres nombres completos en orden alfabético y `[PLAYER] Ready`. | PASS si monta y lista esos tres nombres; FAIL ante error o lista distinta. | **PASS, log UART disponible** en `POC_STATUS.md`. |
| SD-02 | `MUSIC/` vacía. | Montaje correcto; `[MUSIC] Directory found`, `[MUSIC] Tracks: 0`, `[PLAYER] Ready`. Sin error fatal. | PASS si cero canciones y listo; FAIL si fatal o conteo incorrecto. | **PASS, confirmado por el usuario**; el log no está en el checkout. |
| SD-03 | La tarjeta no contiene directorio `MUSIC`. | Montaje correcto; `[MUSIC] Directory not found: MUSIC` y mensaje de modo diagnóstico. | PASS si reporta directorio no encontrado, sin reset; FAIL si se interpreta como directorio vacío o falla el montaje. | **PASS, confirmado por el usuario**; el log no está en el checkout. |
| SD-04 | Apagar y desconectar la placa; retirar la microSD y volver a encender sin tarjeta. | Error de montaje `ESP_ERR_TIMEOUT` controlado; permanece en modo diagnóstico. | PASS si el fallo se informa y el firmware queda en modo diagnóstico; FAIL si se bloquea, resetea o reporta montaje exitoso. | **PASS, confirmado por el usuario**; se reportó `ESP_ERR_TIMEOUT`, log no disponible aquí. |
| SD-05 | `MUSIC/` con MP3 válidos y archivos `.txt`. | Montaje correcto; solo se listan MP3 válidos. El texto del listado puede ser equivalente a SD-01. | PASS si archivos ajenos se excluyen; FAIL si aparecen en catálogo. | **PASS, validación física confirmada por el usuario**: tarjeta preparada con `.txt`; el listado observado solo mostró MP3. El UART no enumera los archivos excluidos. |
| SD-06 | `MUSIC/` con MP3 directos y subdirectorios que contienen otros archivos. | Solo aparecen archivos MP3 del nivel inmediato. | PASS si no recorre subdirectorios; FAIL si incluye archivos anidados. | **PASS, validación física confirmada por el usuario**: tarjeta preparada con subdirectorios; el listado observado solo mostró MP3 directos. El UART no enumera archivos anidados. |
| SD-07 | `MUSIC/` con MP3 normales y `La_Mujer_que_Bota_Fuego_nombre_mas_largo.mp3`. | Nombre LFN completo, sin alias 8.3 ni corte; listado correcto. | PASS funcional si el nombre coincide completo; la advertencia de build se evalúa aparte. | **PASS funcional reportado por el usuario**; el log comunicado muestra el nombre completo. La advertencia LFN en esa ejecución posterior no se puede verificar con el log disponible. |
| SD-08 | Archivos `prueba_minusculas.mp3`, `prueba_mayusculas.MP3`, `prueba_mixta.Mp3` en `MUSIC/`. | `Tracks: 3`; los tres nombres aparecen. | PASS si admite las tres variantes; FAIL si omite cualquiera. | **Pendiente, única prueba física funcional restante.** |

## Registro de resultados

No marcar una fila pendiente como PASS sin adjuntar el log UART real y las condiciones de ejecución. Registrar también cualquier diferencia literal de mensajes o comportamiento; no inferir causa en FATFS o hardware sin evidencia. Inserción y retirada de tarjeta siempre con la placa apagada.

El hito de almacenamiento se acepta dentro del alcance de SD-01 a SD-07, según evidencia UART disponible o confirmación explícita del usuario, con el tipo indicado por fila. SD-08 se aplaza por decisión del responsable: no se marca PASS físico; la comparación de extensiones sin distinguir mayúsculas sí está cubierta por host. La ausencia de la advertencia LFN en el monitor posterior también sigue sin verificar. No se prueba extracción durante lectura, tarjetas dañadas, recuperación, reproducción ni hardware objetivo ESP32-S3/ES8388/SPI.
