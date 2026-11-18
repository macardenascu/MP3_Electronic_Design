# font8x8 (derived, glyph data unmodified)

- Source: https://github.com/dhepper/font8x8, file `font8x8_basic.h` (branch `master`)
- Commit: `8e279d2d864e79128e96188a6b9526cfa3fbfef9` (Daniel Hepper, 2019-07-08, "Merge pull request #4 from WillGreen/master")
- Downloaded: 2026-11-18 from `https://raw.githubusercontent.com/dhepper/font8x8/<commit>/font8x8_basic.h`
- Upstream `font8x8_basic.h` SHA-256: `49d8df366296b203ca3211bc0672cf2a762135bf12710735b6292756b19dffd5`
- License: **Public Domain** (stated in the file header and the upstream README; based on Marcel Sondaar's
  font8x8.h and the public-domain IBM VGA fonts).

`main/core/font8x8.h` is generated from that file. The only changes are: the declaration
`char font8x8_basic[128][8]` becomes `static const unsigned char font8x8_basic[128][8]`, an include guard
and a short provenance comment are added, and line endings are LF. The 128 glyphs are byte-identical.
Only `main/core/ui_render.c` includes it; other modules use `ui_font_glyph()`.
