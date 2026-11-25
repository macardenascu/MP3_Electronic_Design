# font8x8 (derivada, datos de los glifos sin modificar)

- Origen: https://github.com/dhepper/font8x8, archivo `font8x8_basic.h` (rama `master`)
- Commit: `8e279d2d864e79128e96188a6b9526cfa3fbfef9` (Daniel Hepper, 2019-07-08, "Merge pull request #4 from WillGreen/master")
- Descargado: 2026-11-18 desde `https://raw.githubusercontent.com/dhepper/font8x8/<commit>/font8x8_basic.h`
- SHA-256 del `font8x8_basic.h` original: `49d8df366296b203ca3211bc0672cf2a762135bf12710735b6292756b19dffd5`
- Licencia: **dominio público** (así lo indican la cabecera del archivo y el README original; basada en el
  font8x8.h de Marcel Sondaar y en las fuentes VGA de IBM de dominio público).

`main/core/font8x8.h` se genera a partir de ese archivo. Los únicos cambios son: la declaración
`char font8x8_basic[128][8]` pasa a `static const unsigned char font8x8_basic[128][8]`, se añaden una guarda de
inclusión y un comentario corto de procedencia, y los finales de línea son LF. Los 128 glifos son idénticos byte a byte.
Solo `main/core/ui_render.c` lo incluye; los demás módulos usan `ui_font_glyph()`.
