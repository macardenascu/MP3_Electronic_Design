# Pruebas en el computador (host)

Estas pruebas compilan exactamente la implementación portable de `main/music_catalog.c` con un pequeño reemplazo de los códigos de error de ESP-IDF. Cubren rutas `MUSIC` inexistentes o que no son directorio, directorios `MUSIC` vacíos, extensiones con mayúsculas mezcladas, archivos y directorios ignorados, ordenamiento determinista y un nombre largo admitido. El catálogo prueba cada candidato con open/close y reporta los fallos en lugar de listar archivos inaccesibles.

En un computador POSIX con un compilador C11 y CMake:

```sh
cmake -S tests/host -B build-host
cmake --build build-host
./build-host/music_catalog_test
```

La prueba se ejecutó en Windows con GCC de MSYS2 así:

```sh
mkdir -p build-host/tmp
export TMPDIR="$PWD/build-host/tmp"
gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main \
  -DMUSIC_CATALOG_OPENDIR=music_catalog_test_opendir \
  main/music_catalog.c tests/host/music_catalog_test.c \
  -o build-host/music_catalog_test
./build-host/music_catalog_test
```

## Núcleo del reproductor (main/core)

`main/core/` contiene la lógica portable del reproductor en C11: `volume`, `mute_sequencer`, `power_policy` y `player_core`. Devuelve órdenes en secuencia y nunca toca el hardware ni reserva memoria. Cada módulo tiene su propia prueba:

```sh
for t in volume mute_sequencer power_policy; do
  gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main/core \
    main/core/$t.c tests/host/${t}_test.c -o build-host/${t}_test && ./build-host/${t}_test
done
gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main/core \
  main/core/tone_gen.c tests/host/tone_gen_test.c -lm \
  -o build-host/tone_gen_test && ./build-host/tone_gen_test
gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main/core \
  main/core/wav_parser.c tests/host/wav_parser_test.c \
  -o build-host/wav_parser_test && ./build-host/wav_parser_test
gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main/decoder -I main/third_party/minimp3 \
  main/decoder/mp3_decoder.c tests/host/mp3_decoder_test.c -lm \
  -o build-host/mp3_decoder_test && ./build-host/mp3_decoder_test
gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main/core \
  main/core/player_core.c main/core/volume.c tests/host/player_core_test.c \
  -o build-host/player_core_test && ./build-host/player_core_test
```

## Capacidades de audio de la Korvo (main/korvo_audio_caps.c)

`korvo_audio_caps_test` comprueba las frecuencias de muestreo que puede generar la ruta de altavoz del BSP de la Korvo (ES8389 sin MCLK, 16 bits: 8000, 16000, 44100 y 48000 Hz) y que otras, como 22050 y 11025 Hz, se rechazan:

```sh
gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main \
  main/korvo_audio_caps.c tests/host/korvo_audio_caps_test.c \
  -o build-host/korvo_audio_caps_test && ./build-host/korvo_audio_caps_test
```

## Piezas del reproductor (E5/E7)

`korvo_volume_test` comprueba la conversión de dB a porcentaje (−60 dB → 0 %, −12 dB → 60 %, lineal, redondeada y con saturación). `battery_sim_test` encadena la batería simulada (3700 mV, −20 mV por tick) con `power_policy` (corte en 3500 mV, histéresis de 100 mV, 5 lecturas), imprime el tick en que se dispara LOW_BATTERY y comprueba que se dispara una sola vez:

```sh
gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main \
  main/korvo_volume.c tests/host/korvo_volume_test.c \
  -o build-host/korvo_volume_test && ./build-host/korvo_volume_test
gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main/core \
  main/core/battery_sim.c main/core/power_policy.c tests/host/battery_sim_test.c \
  -o build-host/battery_sim_test && ./build-host/battery_sim_test
```

## Interfaz (E6/E8)

`ui_model_test` cubre el framebuffer de 128x64 con el formato del SSD1306 y el recorte, el desplazamiento del nombre (posición a 0, 1000, 1300 y 1600 ms, y reinicio de cada vuelta), nombres cortos centrados y sin extensión, el formato `mm:ss`, la barra de volumen en mínimo, medio y máximo, el mensaje de 2 s, el apagado de la pantalla a los 10 s, que la primera pulsación con la pantalla apagada se descarta, que un mensaje enciende la pantalla y que todos los mensajes del reproductor caben en 16 caracteres. `ui_preview` escribe `ui_<caso>.pbm` (P4, 128x64) para: canción corta reproduciendo, nombre largo en t=0 y t=2000 ms, pausa, volumen máximo, "No soportado", "Bateria baja" y catálogo vacío:

```sh
gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main/core \
  main/core/ui_render.c main/core/ui_model.c tests/host/ui_model_test.c \
  -o build-host/ui_model_test && ./build-host/ui_model_test
gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main/core \
  main/core/ui_render.c main/core/ui_model.c tests/host/ui_preview.c -o build-host/ui_preview
(cd build-host && ./ui_preview)
```

La fuente de 8x8 es de dominio público; ver `main/core/FONT_LICENSE.md`.

## Decodificador MP3 (main/decoder)

`mp3_decoder_test` cubre el tamaño *syncsafe* de ID3v2 (incluido el indicador de pie), un archivo vacío, una etiqueta más grande que el búfer de entrada, una etiqueta truncada, datos aleatorios (`ESP_ERR_NOT_SUPPORTED`) y una función de lectura que falla (`ESP_FAIL`). No decodifica audio real.

`mp3_to_wav` es una herramienta para el computador que decodifica un MP3 real con el mismo envoltorio y escribe un WAV de 16 bits con los mismos canales y frecuencia de muestreo. Imprime `frames`, `sample_rate`, `channels`, `samples_per_ch` y `skipped_bytes`:

```sh
gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main/decoder -I main/third_party/minimp3 \
  main/decoder/mp3_decoder.c tests/host/mp3_to_wav.c -lm -o build-host/mp3_to_wav
./build-host/mp3_to_wav input.mp3 output.wav
```

Los archivos de prueba se crean dentro de `build-host/`, así que quedan dentro del proyecto. Los errores de permiso denegado al recorrer directorios son difíciles de reproducir de forma fiable cuando las pruebas corren como administrador/root. El catálogo propaga los fallos de `opendir`, `stat`, `readdir` y `closedir`.
