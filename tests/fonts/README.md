# Test fonts

The fonts `mcpp test` shapes, draws and queries. They are made by
[tools/make_test_fonts.py](../../tools/make_test_fonts.py) from pinned sources,
checked against their SHA-256; [fonts.json](fonts.json) records each output's
source, reduction and checksum. They stay in the repository: the release
archive leaves them out (`.gitattributes`, `export-ignore`).

| File | Source | Reduction | License |
|---|---|---|---|
| `NotoSans-Latin.ttf` | Noto Sans (variable, `wdth`/`wght`), [google/fonts](https://github.com/google/fonts/tree/a4c0b21f839d7ac65a7d8b49eb81bddd110774a3/ofl/notosans) | subset to Basic Latin, Latin-1 and U+2026, variations kept | SIL OFL 1.1, [OFL-NotoSans.txt](OFL-NotoSans.txt) |
| `NotoSansArabic-Regular.ttf` | Noto Sans Arabic, [google/fonts](https://github.com/google/fonts/tree/a4c0b21f839d7ac65a7d8b49eb81bddd110774a3/ofl/notosansarabic) | instanced at wght 400 / wdth 100, subset to U+0621–0655, U+0660–066D, U+200C–200F, U+25CC | SIL OFL 1.1, [OFL-NotoSansArabic.txt](OFL-NotoSansArabic.txt) |
| `NotoSansDevanagari-Regular.ttf` | Noto Sans Devanagari, [google/fonts](https://github.com/google/fonts/tree/a4c0b21f839d7ac65a7d8b49eb81bddd110774a3/ofl/notosansdevanagari) | instanced at wght 400 / wdth 100, subset to U+0900–0970, U+200C–200D, U+25CC | SIL OFL 1.1, [OFL-NotoSansDevanagari.txt](OFL-NotoSansDevanagari.txt) |
| `chromacheck-colr.ttf` | ChromaCheck COLR by Roel Nieskens and Google, as HarfBuzz 14.3.0 ships it in `test/api/fonts` | none | MIT, [LICENSE-ChromaCheck.md](LICENSE-ChromaCheck.md) |

The subsets keep every OpenType layout feature, the name table and the glyph
names (fontTools 4.67.0 `subset` with `--layout-features=* --name-IDs=*
--glyph-names --notdef-outline`). The Noto fonts are Copyright 2022 The Noto
Project Authors; no Reserved Font Name is declared.
