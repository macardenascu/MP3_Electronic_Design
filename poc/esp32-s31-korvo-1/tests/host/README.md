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
  main/core/player_core.c main/core/volume.c tests/host/player_core_test.c \
  -o build-host/player_core_test && ./build-host/player_core_test
```

Test fixtures are created under `build-host/` so they remain inside the project. Permission-denied iteration errors are difficult to reproduce reliably when tests run as an administrator/root. The scanner propagates `opendir`, `stat`, `readdir`, and `closedir` failures.
