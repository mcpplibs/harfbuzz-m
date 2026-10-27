# Using mcpplibs.mwrap.harfbuzz

`import mcpplibs.mwrap.harfbuzz;` declares HarfBuzz 14.3.0's API as its headers do. Types, enumerators and functions are the upstream globals, the very same entities as the headers', so a translation unit may include a header too. The library is `compat.harfbuzz`: upstream's amalgamation `src/harfbuzz.cc`, built with the FreeType backend over `compat.freetype` 2.13.3.

## Modules

| Module | What it holds |
|---|---|
| `mcpplibs.mwrap.harfbuzz` | everything `<hb.h>`, `<hb-ot.h>` and `<hb-aat.h>` declare: 142 types (typedef names and the struct/union tags `_hb_var_int_t`, `hb_language_impl_t` …), 805 enumerators (`HB_SCRIPT_*`, `HB_BUFFER_FLAG_*`, `HB_OT_NAME_ID_*` …), 533 functions (blob, buffer, common, draw, face, font, map, paint, set, shape, shape-plan, style, unicode, version, the `hb_ot_*` and `hb_aat_*` APIs, the deprecated ones); `namespace hb` of `<hb-cplusplus.hh>`; re-exports the two below |
| `mcpplibs.mwrap.harfbuzz.ft` | the 16 functions of `<hb-ft.h>`: `hb_ft_face_create*`, `hb_ft_font_create*`, `hb_ft_font_set_funcs`, `hb_ft_font_lock_face` … |
| `mcpplibs.mwrap.harfbuzz.macros` | the macros, in `mcpplibs::mwrap::harfbuzz::macros` |

`using namespace mcpplibs::mwrap::harfbuzz::macros;` makes code read as it does with the headers.

## The C++ helpers

`hb::shared_ptr<T>` and `hb::unique_ptr<T>` hold any HarfBuzz object through its `reference` / `destroy` pair (`hb::vtable<T>`, defined for every object type of this library); `hb::is_shared_ptr` / `hb::is_unique_ptr` recognise them, and `std::hash` is specialised for both. `hb::shared_ptr<hb_font_t> font{hb_font_create(face)};` converts to `hb_font_t*` wherever the C API takes one.

## Macros

A named module cannot export a macro. `mcpplibs::mwrap::harfbuzz::macros` holds them under their original names:

- **Constants** as `inline constexpr`, each with its macro's type: `HB_CODEPOINT_INVALID`, `HB_TAG_NONE` / `_MAX` / `_MAX_SIGNED`, `HB_LANGUAGE_INVALID` (a null `hb_language_t`), `HB_FEATURE_GLOBAL_START` / `_END`, `HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT`, `HB_FONT_NO_VAR_NAMED_INSTANCE`, `HB_MAP_VALUE_INVALID`, `HB_SET_VALUE_INVALID`, `HB_UNICODE_MAX`, `HB_PAINT_IMAGE_FORMAT_*`, `HB_OT_TAG_*` (`GSUB`, `GPOS`, `GDEF`, `BASE`, `JSTF`, `MATH`, the default script and language, the variation axes), `HB_OT_LAYOUT_*` indices, `HB_OT_MAX_TAGS_PER_*`, `HB_OT_SHAPE_BUFFER_FORMAT_SERIAL`, `HB_AAT_LAYOUT_NO_SELECTOR_INDEX`, `HB_VERSION_MAJOR` / `_MINOR` / `_MICRO` / `_STRING`.
- **Initialisers** as constexpr values of their structures: `HB_SEGMENT_PROPERTIES_DEFAULT`, `HB_DRAW_STATE_DEFAULT`.
- **Computations** as constexpr functions, usable in constant expressions; each converts its arguments as the macro casts them: `HB_TAG`, `HB_COLOR`, `HB_DIRECTION_IS_VALID` / `_HORIZONTAL` / `_VERTICAL` / `_FORWARD` / `_BACKWARD`, `HB_DIRECTION_REVERSE`, `HB_BUFFER_CLUSTER_LEVEL_IS_MONOTONE` / `_GRAPHEMES` / `_CHARACTERS`, `HB_VERSION_ATLEAST`.
- **Deprecated aliases** of `hb-deprecated.h` / `hb-ot-deprecated.h` (`HB_BUFFER_FLAGS_DEFAULT`, `HB_SCRIPT_CANADIAN_ABORIGINAL`, `HB_OT_VAR_NO_AXIS_INDEX`, `HB_OT_MATH_SCRIPT` …), marked `[[deprecated]]` with their replacement.

`hb_color_get_alpha` / `_red` / `_green` / `_blue` and `hb_glyph_info_get_glyph_flags` are macros over functions of the same names; the functions are exported.

### What stays a macro

These stay macros in `<mwrap/harfbuzz/macros.hpp>`, which is `<hb.h>`, `<hb-ot.h>` and `<hb-aat.h>`:

| Macro | Why |
|---|---|
| `HB_UNTAG` | expands to four comma-separated arguments, for a printf-style call |
| `HB_VERSION_ATLEAST` in `#if` | a preprocessor test (the constexpr function serves everywhere else) |

Include it before the import. While a macro is defined it expands even inside a qualified name, so `macros::HB_TAG_NONE` is not reachable there; use the plain macro (see [tests/header_mix_test.cpp](../tests/header_mix_test.cpp)). The annotations `HB_EXTERN`, `HB_BEGIN_DECLS` / `HB_END_DECLS`, `HB_DEPRECATED` / `HB_DEPRECATED_FOR`, the umbrellas' `HB_*_H_IN`, the include guards and `HB_DEFINE_VTABLE` (the helpers' vtable definitions, all made already) are not mapped. [tools/check_exports.py](../tools/check_exports.py) accounts for all 112 macros.

## FreeType

`compat.harfbuzz` is built with FreeType, so `<hb-ft.h>` is part of the library: `mcpplibs.mwrap.harfbuzz.ft` exports it, and the `ft` font functions are available by name (`hb_font_set_funcs_using(font, "ft")`, `hb_face_create_from_file_or_fail_using(path, 0, "ft")`). FreeType's own types (`FT_Face`, `FT_Library` …) are FreeType's entities: import `mcpplibs.mwrap.freetype`, or include `<ft2build.h>` and `FT_FREETYPE_H`. Declarations from a global module fragment are the same entities whichever module or header brings them, so the `FT_Face` of either is the one `hb_ft_font_create` takes. This package depends on `compat.freetype` directly, the same release `compat.harfbuzz` is built with; it does not depend on another wrapper.

## Not exported

| Header or declaration | Why |
|---|---|
| `hb-subset.h`, `hb-subset-serialize.h`, `hb-subset-depend.h` | libharfbuzz-subset (`src/harfbuzz-subset.cc`), which `compat.harfbuzz` does not build |
| `hb-raster.h`, `hb-vector.h`, `hb-gpu.h` | separate libraries (libharfbuzz-raster, -vector, -gpu) not built |
| `hb-cairo.h`, `hb-icu.h`, `hb-glib.h`, `hb-gobject.h`, `hb-gobject-structs.h` | integrations with cairo, ICU, GLib and GObject, not built |
| `hb-coretext.h`, `hb-directwrite.h`, `hb-gdi.h`, `hb-uniscribe.h`, `hb-graphite2.h`, `hb-fontations.h` | platform and external shaping backends, not enabled |
| `hb-wasm-api.h` | the experimental WebAssembly shaper: not installed upstream, not enabled |
| `hb_shape_justify` | declared and compiled only with `HB_EXPERIMENTAL_API`, which `compat.harfbuzz` does not define |

## Checks

- `mcpp test`: shaping Latin, Arabic and Devanagari with the glyphs and advances an independent HarfBuzz build (uharfbuzz) gives for the same fonts; buffers, cluster levels, serialisation and diffs; blobs, faces, fonts, variations and custom font functions; draw and paint functions; OpenType layout, colour, metrics, name, var, math and meta; AAT; style; sets, maps and Unicode functions; the FreeType integration; the C++ helpers; the constants; the side header mixed with the import. Fonts: [tests/fonts](../tests/fonts/README.md).
- `python3 tools/check_exports.py`: `src/harfbuzz.cppm` and `src/ft.cppm` are what [tools/gen_exports.py](../tools/gen_exports.py) writes from the pinned headers; every declaration is exported, every public header wrapped or listed, every macro mapped or listed with its reason.
- `python3 tools/run_conformance.py`: upstream's `test/shape/run-tests.py` with the `.tests` files of `in-house`, `aots` and `text-rendering-tests` (as their `meson.build` lists them), on [examples/hb-shape](../examples/hb-shape), with the `ot` and the `ft` font functions: 7,549 pass (7,577 on macOS, where some of the system fonts the suites name are present). Skipped, as upstream skips them: lines for shapers this build does not have (CoreText, DirectWrite, Uniscribe …) and macOS system fonts that are absent or of another version.
- `python3 tools/test_template.py`: the default template, scaffolded from the package as installed, built and run.
