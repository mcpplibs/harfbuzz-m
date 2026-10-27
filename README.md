# harfbuzz

> HarfBuzz, the OpenType text shaping engine, through C++23 modules.

**English** · [简体中文](README.zh.md) · [Upstream](https://github.com/harfbuzz/harfbuzz) · [Releases](https://github.com/mcpplibs/harfbuzz-m/releases) · [mcpp](https://github.com/mcpp-community/mcpp) · [Index](https://github.com/mcpplibs/mcpp-index)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![mcpp-community](https://img.shields.io/badge/mcpp--community-mwrap-blue)](https://github.com/mcpp-community)
[![CI](https://github.com/mcpplibs/harfbuzz-m/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/mcpplibs/harfbuzz-m/actions/workflows/ci.yml)

The upstream C API is unchanged: `hb_buffer_t`, `hb_shape`, `hb_font_create`,
`HB_SCRIPT_ARABIC` are the upstream globals of `<hb.h>`, `<hb-ot.h>`,
`<hb-aat.h>` and `<hb-ft.h>`; `hb::shared_ptr` and `hb::unique_ptr` are those
of `<hb-cplusplus.hh>`. The library is `compat.harfbuzz` 14.3.0 with its
FreeType backend. Macros are mapped to constants and constexpr functions.
HarfBuzz's own shaping suites pass on every CI run, on a port of upstream's
`hb-shape` written against the module.

## Modules

- `import mcpplibs.mwrap.harfbuzz;`: the whole API (533 functions, 142 types, 805 enumerators, `namespace hb`), re-exporting the two below
- `import mcpplibs.mwrap.harfbuzz.ft;`: `hb_ft_font_create`, `hb_ft_font_set_funcs` … over FreeType's `FT_Face`
- `import mcpplibs.mwrap.harfbuzz.macros;`: `HB_TAG`, `HB_OT_TAG_GSUB`, `HB_LANGUAGE_INVALID` … in `mcpplibs::mwrap::harfbuzz::macros`

> More Modules API -> [Usage and API](docs/usage.md)

## Getting started

```sh
mcpp new banner --template mcpplibs.mwrap.harfbuzz@14.3.0
cd banner
mcpp run
mcpp run -- /path/to/font.ttf "Text" -liga,-kern
```

The default template shapes a line with a small font it carries (ligatures
and kerning included), prints the glyph run, and draws the line in the
terminal from the glyph outlines. Given a font file, it shapes any text.

```toml
[dependencies]
"mcpplibs.mwrap.harfbuzz" = "14.3.0"
```

```cpp
import std;
import mcpplibs.mwrap.harfbuzz;
using namespace mcpplibs::mwrap::harfbuzz::macros;  // HB_TAG, HB_LANGUAGE_INVALID …

int main(int argc, char** argv) {
    hb_face_t* face = hb_face_create_from_file_or_fail(argv[1], 0);
    if (!face) return 1;
    hb_font_t* font = hb_font_create(face);
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, "office", -1, 0, -1);
    hb_buffer_guess_segment_properties(buffer);
    hb_feature_t kern_off{HB_TAG('k', 'e', 'r', 'n'), 0, HB_FEATURE_GLOBAL_START, HB_FEATURE_GLOBAL_END};
    hb_shape(font, buffer, &kern_off, 1);
    unsigned n = 0;
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buffer, &n);
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buffer, &n);
    for (unsigned i = 0; i < n; ++i)
        std::println("glyph {} cluster {} advance {}", info[i].codepoint, info[i].cluster, pos[i].x_advance);
    hb_buffer_destroy(buffer);
    hb_font_destroy(font);
    hb_face_destroy(face);
}
```

## Macros and FreeType

A named module cannot export a macro. Constants (`HB_OT_TAG_*`,
`HB_CODEPOINT_INVALID`, `HB_FEATURE_GLOBAL_END`, `HB_VERSION_*` …), the
initialisers `HB_SEGMENT_PROPERTIES_DEFAULT` / `HB_DRAW_STATE_DEFAULT` and the
computations (`HB_TAG`, `HB_COLOR`, `HB_DIRECTION_IS_*`, `HB_VERSION_ATLEAST`
…) are in `mcpplibs::mwrap::harfbuzz::macros`. `HB_UNTAG` (four arguments of a
call) and `#if HB_VERSION_ATLEAST(…)` need `<mwrap/harfbuzz/macros.hpp>`,
included before the import. `FT_Face` is FreeType's: import
`mcpplibs.mwrap.freetype` or include `<ft2build.h>`; either names the same
type. Details: [docs/usage.md](docs/usage.md).

## Examples

- [examples/hb-shape](examples/hb-shape): upstream's `hb-shape` command-line shaper (the same options, output and
  `--batch` mode), ported to C++23 against the module; [tools/run_conformance.py](tools/run_conformance.py) runs
  HarfBuzz's `test/shape` suites on it.

## Build and testing

Linux, macOS and Windows CI build the modules and run the tests: shaping
Latin, Arabic and Devanagari against an independent HarfBuzz build's results,
buffers and serialisation, sets and maps, Unicode functions, draw and paint
functions, OpenType and AAT queries, the FreeType integration, the C++
helpers, the macros and the header mixed with the import. The export audit
([tools/check_exports.py](tools/check_exports.py)) checks that the generated
surface is fresh, every declaration exported and every macro mapped or listed
with its reason. Upstream's `in-house`, `aots` and `text-rendering-tests`
shaping suites run with both the OpenType and the FreeType font functions. A
separate job scaffolds the default template as an installed package and runs
it.

## License

[Apache-2.0](LICENSE) for the wrapper, by mcpp-community; HarfBuzz remains
under its "Old MIT" license ([upstream/harfbuzz/COPYING](upstream/harfbuzz/COPYING)).
The test fonts keep their own licenses ([tests/fonts](tests/fonts/README.md)).
