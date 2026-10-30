# PoC ESP32-S31-Korvo-1

## Objetivo

Validar en firmware el montaje de microSD proporcionado por el BSP y el inventario determinista de nombres `.mp3` dentro de `MUSIC`. La primera etapa no prueba reproducción de audio.

La Korvo es una plataforma temporal: usa ESP32-S31, códec ES8389 y SDMMC. El hardware objetivo propuesto usa ESP32-S3-WROOM-1-N16R8, ES8388 por I²S y microSD por SPI. Por tanto, esta PoC no valida compatibilidad eléctrica, audio ni almacenamiento del producto final.

## Herramientas y versiones

- ESP-IDF 6.1, con modo preview habilitado para el target `esp32s31`.
- ESP-BSP `espressif/esp32_s31_korvo_1` versión 1.0.1, fijada en `main/idf_component.yml` y `dependencies.lock`.
- Git for Windows y Python/toolchain administrados por la instalación de ESP-IDF.
- Para pruebas host: GCC compatible con C11. En Windows se ha usado GCC de MSYS2.

El script `tools/activate_idf.ps1` activa la instalación local de ESP-IDF en Windows. Sus rutas predeterminadas corresponden a `C:\Espressif`; se pueden reemplazar con las variables `IDF_ROOT`, `IDF_TOOLS_ROOT`, `IDF_PYTHON_SCRIPTS` y `GIT_CMD_PATH` antes de ejecutarlo.

## Archivos

- `main/music_catalog.c/.h`: lógica portable del catálogo, con capacidad fija y ordenamiento determinista.
- `main/storage_korvo.c/.h`: adaptación del montaje SD al BSP.
- `main/app_main.c`: arranque y diagnóstico por consola.
- `tests/host/`: pruebas host del catálogo y shim de errores ESP-IDF.
- `sdkconfig.defaults`, `dependencies.lock`, `CMakeLists.txt`: configuración y reproducibilidad ESP-IDF.
- `docs/POC_STATUS.md`: resultados, límites y evidencia detallada.

## Preparación y pruebas host en Windows (PowerShell)

Desde la raíz del repositorio:

```powershell
Set-Location .\poc\esp32-s31-korvo-1
$env:PATH = 'C:\msys64\usr\bin;' + $env:PATH
New-Item -ItemType Directory -Force build-host | Out-Null
gcc -std=c11 -Wall -Wextra -Werror -DMUSIC_CATALOG_OPENDIR=music_catalog_test_opendir -I tests/host/include -I main main/music_catalog.c tests/host/music_catalog_test.c -o build-host/music_catalog_test.exe
if ($LASTEXITCODE -ne 0) { throw 'Falló la compilación de pruebas host' }
& .\build-host\music_catalog_test.exe
if ($LASTEXITCODE -ne 0) { throw 'Fallaron las pruebas host' }
```

La suite cubre `MUSIC` inexistente, `MUSIC` como archivo, directorio vacío, extensiones `.mp3` con distintas mayúsculas, archivos ajenos, subdirectorios ignorados, ordenamiento, nombre largo, capacidad exacta de 256 y rechazo de la entrada 257. Un fallo no publica un catálogo parcial.

## Activación y compilación ESP32-S31

Abra PowerShell y desde la raíz del repositorio:

```powershell
. .\poc\esp32-s31-korvo-1\tools\activate_idf.ps1
Set-Location .\poc\esp32-s31-korvo-1
idf.py --preview -B build_audit build
```

`build_audit` es un directorio nuevo, ignorado por Git, para evitar reutilizar rutas absolutas guardadas en un build previo. El target se configura como `esp32s31` en `sdkconfig.defaults`; `--preview` habilita ese target experimental. Un build satisfactorio termina con `Project build complete` y genera `build_audit/audio_player_poc.bin`.

El nombre anterior `build_new` permanece válido como referencia histórica, pero para una compilación en otro checkout conviene usar una carpeta nueva. En Windows ESP-IDF puede advertir que ciertas rutas exceden los límites tradicionales; una advertencia aislada no indica por sí sola un fallo. El resultado final de CMake/Ninja determina si la compilación terminó. En una ejecución pasada también hubo rutas con caracteres Unicode que complicaron herramientas auxiliares; mantenga el checkout y los directorios de build en una ruta corta y ASCII cuando sea posible. La instalación de ESP-IDF 6.1 ya está disponible y no es necesario reinstalarla por esos problemas anteriores.

## Resultado esperado y límites

Al ejecutar el firmware en una placa se esperan mensajes de diagnóstico del montaje SD y del catálogo. La compilación por sí sola no demuestra ese comportamiento físico. En esta primera etapa no se decodifica ni reproduce MP3, PCM o WAV; tampoco están implementados interfaz OLED, botones, USB MSC o Bluetooth. Consulte [POC_STATUS.md](docs/POC_STATUS.md) para separar resultados host, compilación y validación física.
