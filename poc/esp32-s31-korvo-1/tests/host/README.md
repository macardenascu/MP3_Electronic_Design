# Host tests

This compiles the exact portable `main/music_catalog.c` implementation with a tiny ESP error-code shim. It covers missing and non-directory MUSIC paths, empty MUSIC directories, mixed-case extensions, ignored files/directories, deterministic sorting and a long supported filename. The scanner probes each candidate with open/close and reports failures rather than listing inaccessible files.

On a POSIX host with a C11 compiler and CMake:

```sh
cmake -S tests/host -B build-host
cmake --build build-host
./build-host/music_catalog_test
```

The test was run on Windows through MSYS2 GCC with:

```sh
mkdir -p build-host/tmp
export TMPDIR="$PWD/build-host/tmp"
gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main \
  -DMUSIC_CATALOG_OPENDIR=music_catalog_test_opendir \
  main/music_catalog.c tests/host/music_catalog_test.c \
  -o build-host/music_catalog_test
./build-host/music_catalog_test
```

## Player core (main/core)

`main/core/` holds the portable C11 player logic: `volume`, `mute_sequencer`, `power_policy` and `player_core`. It returns ordered commands and never touches hardware or allocates memory. Each module has its own test:

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

## Korvo audio capabilities (main/korvo_audio_caps.c)

`korvo_audio_caps_test` checks the sample rates the Korvo BSP speaker path can clock (ES8389 without MCLK, 16 bits: 8000, 16000, 44100, 48000 Hz) and that others such as 22050 and 11025 Hz are rejected:

```sh
gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main \
  main/korvo_audio_caps.c tests/host/korvo_audio_caps_test.c \
  -o build-host/korvo_audio_caps_test && ./build-host/korvo_audio_caps_test
```

## MP3 decoder (main/decoder)

`mp3_decoder_test` covers the ID3v2 syncsafe size (including the footer flag), an empty file, a tag larger than the input buffer, a truncated tag, random data (`ESP_ERR_NOT_SUPPORTED`) and a failing read callback (`ESP_FAIL`). It does not decode real audio.

`mp3_to_wav` is a host tool that decodes a real MP3 with the same wrapper and writes a 16-bit WAV with the same channels and sample rate, printing `frames`, `sample_rate`, `channels`, `samples_per_ch` and `skipped_bytes`:

```sh
gcc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main/decoder -I main/third_party/minimp3 \
  main/decoder/mp3_decoder.c tests/host/mp3_to_wav.c -lm -o build-host/mp3_to_wav
./build-host/mp3_to_wav input.mp3 output.wav
```

Test fixtures are created under `build-host/` so they remain inside the project. Permission-denied iteration errors are difficult to reproduce reliably when tests run as an administrator/root. The scanner propagates `opendir`, `stat`, `readdir`, and `closedir` failures.
