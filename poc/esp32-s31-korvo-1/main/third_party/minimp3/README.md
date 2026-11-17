# minimp3 (vendored, unmodified)

- Source: https://github.com/lieff/minimp3 (branch `master`)
- Commit: `6238eb88e03afc2e7064f3002ed3b8d5d3716706` (author Lion, 2026-11-17T21:31:25Z, "fix the ID3v1 branch of the mp3d_match_frame() end-of-stream check")
- Downloaded: 2026-11-17, from `https://raw.githubusercontent.com/lieff/minimp3/<commit>/`
- Files: `minimp3.h`, `LICENSE` (byte-identical to upstream)
  - `minimp3.h` SHA-256 `422a6cdcd278586af7fd1eb53e430baef75ababe62a774eccb174f7902c7dad9`
  - `LICENSE` SHA-256 `6a1ee543e5282cd9061881edf462e6fdab181f328da71fc2c9a6950a80e94d01`
- License: CC0 1.0 Universal (public domain dedication), see `LICENSE`.

The implementation is compiled only in `main/decoder/mp3_decoder.c`
(`MINIMP3_ONLY_MP3` + `MINIMP3_IMPLEMENTATION`). Do not edit these files; update by
replacing them from a new upstream commit and recording it here.
