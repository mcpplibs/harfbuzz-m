// mcpplibs.mwrap.harfbuzz.macros — the macros of <hb.h>, <hb-ot.h> and
// <hb-aat.h> (a named module cannot export a macro), under their original
// names in mcpplibs::mwrap::harfbuzz::macros:
//
//   - the constants (HB_CODEPOINT_INVALID, HB_TAG_NONE, HB_LANGUAGE_INVALID,
//     HB_FEATURE_GLOBAL_START / _END, HB_OT_TAG_*, HB_OT_LAYOUT_*,
//     HB_VERSION_* …) as `inline constexpr`, each of its macro's type;
//   - the initialisers HB_SEGMENT_PROPERTIES_DEFAULT and HB_DRAW_STATE_DEFAULT
//     as constexpr values of their structures;
//   - the computations HB_TAG, HB_COLOR, HB_DIRECTION_IS_* / _REVERSE,
//     HB_BUFFER_CLUSTER_LEVEL_IS_* and HB_VERSION_ATLEAST as constexpr
//     functions, each converting its arguments as the macro casts them;
//   - the deprecated aliases of hb-deprecated.h / hb-ot-deprecated.h
//     (HB_BUFFER_FLAGS_DEFAULT, HB_OT_VAR_NO_AXIS_INDEX …), [[deprecated]].
//
// hb_color_get_alpha / _red / _green / _blue and hb_glyph_info_get_glyph_flags
// are macros over functions of the same names, exported by the root module.
// HB_UNTAG expands to four arguments, which no function can return: it stays a
// macro, in <mwrap/harfbuzz/macros.hpp>, included before the import (as does
// HB_VERSION_ATLEAST for a test in #if). tools/check_exports.py accounts for
// every macro.
module;
#include <hb.h>
#include <hb-ot.h>
#include <hb-aat.h>

namespace mwrap_harfbuzz_capture {
inline constexpr auto value_HB_CODEPOINT_INVALID = HB_CODEPOINT_INVALID;
inline constexpr auto value_HB_TAG_NONE = HB_TAG_NONE;
inline constexpr auto value_HB_TAG_MAX = HB_TAG_MAX;
inline constexpr auto value_HB_TAG_MAX_SIGNED = HB_TAG_MAX_SIGNED;
inline constexpr auto value_HB_LANGUAGE_INVALID = HB_LANGUAGE_INVALID;
inline constexpr auto value_HB_FEATURE_GLOBAL_START = HB_FEATURE_GLOBAL_START;
inline constexpr auto value_HB_FEATURE_GLOBAL_END = HB_FEATURE_GLOBAL_END;
inline constexpr auto value_HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT = HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT;
inline constexpr auto value_HB_FONT_NO_VAR_NAMED_INSTANCE = HB_FONT_NO_VAR_NAMED_INSTANCE;
inline constexpr auto value_HB_MAP_VALUE_INVALID = HB_MAP_VALUE_INVALID;
inline constexpr auto value_HB_SET_VALUE_INVALID = HB_SET_VALUE_INVALID;
inline constexpr auto value_HB_UNICODE_MAX = HB_UNICODE_MAX;
inline constexpr auto value_HB_PAINT_IMAGE_FORMAT_PNG = HB_PAINT_IMAGE_FORMAT_PNG;
inline constexpr auto value_HB_PAINT_IMAGE_FORMAT_SVG = HB_PAINT_IMAGE_FORMAT_SVG;
inline constexpr auto value_HB_PAINT_IMAGE_FORMAT_BGRA = HB_PAINT_IMAGE_FORMAT_BGRA;
inline constexpr auto value_HB_VERSION_MAJOR = HB_VERSION_MAJOR;
inline constexpr auto value_HB_VERSION_MINOR = HB_VERSION_MINOR;
inline constexpr auto value_HB_VERSION_MICRO = HB_VERSION_MICRO;
inline constexpr auto value_HB_OT_TAG_BASE = HB_OT_TAG_BASE;
inline constexpr auto value_HB_OT_TAG_GDEF = HB_OT_TAG_GDEF;
inline constexpr auto value_HB_OT_TAG_GSUB = HB_OT_TAG_GSUB;
inline constexpr auto value_HB_OT_TAG_GPOS = HB_OT_TAG_GPOS;
inline constexpr auto value_HB_OT_TAG_JSTF = HB_OT_TAG_JSTF;
inline constexpr auto value_HB_OT_TAG_DEFAULT_SCRIPT = HB_OT_TAG_DEFAULT_SCRIPT;
inline constexpr auto value_HB_OT_TAG_DEFAULT_LANGUAGE = HB_OT_TAG_DEFAULT_LANGUAGE;
inline constexpr auto value_HB_OT_MAX_TAGS_PER_SCRIPT = HB_OT_MAX_TAGS_PER_SCRIPT;
inline constexpr auto value_HB_OT_MAX_TAGS_PER_LANGUAGE = HB_OT_MAX_TAGS_PER_LANGUAGE;
inline constexpr auto value_HB_OT_LAYOUT_NO_SCRIPT_INDEX = HB_OT_LAYOUT_NO_SCRIPT_INDEX;
inline constexpr auto value_HB_OT_LAYOUT_NO_FEATURE_INDEX = HB_OT_LAYOUT_NO_FEATURE_INDEX;
inline constexpr auto value_HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX = HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX;
inline constexpr auto value_HB_OT_LAYOUT_NO_VARIATIONS_INDEX = HB_OT_LAYOUT_NO_VARIATIONS_INDEX;
inline constexpr auto value_HB_OT_TAG_MATH = HB_OT_TAG_MATH;
inline constexpr auto value_HB_OT_TAG_MATH_SCRIPT = HB_OT_TAG_MATH_SCRIPT;
inline constexpr auto value_HB_OT_SHAPE_BUFFER_FORMAT_SERIAL = HB_OT_SHAPE_BUFFER_FORMAT_SERIAL;
inline constexpr auto value_HB_OT_TAG_VAR_AXIS_ITALIC = HB_OT_TAG_VAR_AXIS_ITALIC;
inline constexpr auto value_HB_OT_TAG_VAR_AXIS_OPTICAL_SIZE = HB_OT_TAG_VAR_AXIS_OPTICAL_SIZE;
inline constexpr auto value_HB_OT_TAG_VAR_AXIS_SLANT = HB_OT_TAG_VAR_AXIS_SLANT;
inline constexpr auto value_HB_OT_TAG_VAR_AXIS_WIDTH = HB_OT_TAG_VAR_AXIS_WIDTH;
inline constexpr auto value_HB_OT_TAG_VAR_AXIS_WEIGHT = HB_OT_TAG_VAR_AXIS_WEIGHT;
inline constexpr auto value_HB_AAT_LAYOUT_NO_SELECTOR_INDEX = HB_AAT_LAYOUT_NO_SELECTOR_INDEX;
inline constexpr char value_HB_VERSION_STRING[] = HB_VERSION_STRING;
inline constexpr hb_segment_properties_t value_HB_SEGMENT_PROPERTIES_DEFAULT = HB_SEGMENT_PROPERTIES_DEFAULT;
inline constexpr hb_draw_state_t value_HB_DRAW_STATE_DEFAULT = HB_DRAW_STATE_DEFAULT;
inline constexpr auto value_HB_SCRIPT_CANADIAN_ABORIGINAL = HB_SCRIPT_CANADIAN_ABORIGINAL;
inline constexpr auto value_HB_BUFFER_FLAGS_DEFAULT = HB_BUFFER_FLAGS_DEFAULT;
inline constexpr auto value_HB_BUFFER_SERIALIZE_FLAGS_DEFAULT = HB_BUFFER_SERIALIZE_FLAGS_DEFAULT;
inline constexpr auto value_HB_UNICODE_COMBINING_CLASS_CCC133 = HB_UNICODE_COMBINING_CLASS_CCC133;
inline constexpr auto value_HB_UNICODE_MAX_DECOMPOSITION_LEN = HB_UNICODE_MAX_DECOMPOSITION_LEN;
inline constexpr auto value_HB_AAT_LAYOUT_FEATURE_TYPE_CURISVE_CONNECTION = HB_AAT_LAYOUT_FEATURE_TYPE_CURISVE_CONNECTION;
inline constexpr auto value_HB_MATH_GLYPH_PART_FLAG_EXTENDER = HB_MATH_GLYPH_PART_FLAG_EXTENDER;
inline constexpr auto value_HB_OT_MATH_SCRIPT = HB_OT_MATH_SCRIPT;
inline constexpr auto value_HB_OT_VAR_NO_AXIS_INDEX = HB_OT_VAR_NO_AXIS_INDEX;
}  // namespace mwrap_harfbuzz_capture

// Every mapped name, so the definitions below are not rewritten.
#undef HB_CODEPOINT_INVALID
#undef HB_TAG_NONE
#undef HB_TAG_MAX
#undef HB_TAG_MAX_SIGNED
#undef HB_LANGUAGE_INVALID
#undef HB_FEATURE_GLOBAL_START
#undef HB_FEATURE_GLOBAL_END
#undef HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT
#undef HB_FONT_NO_VAR_NAMED_INSTANCE
#undef HB_MAP_VALUE_INVALID
#undef HB_SET_VALUE_INVALID
#undef HB_UNICODE_MAX
#undef HB_PAINT_IMAGE_FORMAT_PNG
#undef HB_PAINT_IMAGE_FORMAT_SVG
#undef HB_PAINT_IMAGE_FORMAT_BGRA
#undef HB_VERSION_MAJOR
#undef HB_VERSION_MINOR
#undef HB_VERSION_MICRO
#undef HB_OT_TAG_BASE
#undef HB_OT_TAG_GDEF
#undef HB_OT_TAG_GSUB
#undef HB_OT_TAG_GPOS
#undef HB_OT_TAG_JSTF
#undef HB_OT_TAG_DEFAULT_SCRIPT
#undef HB_OT_TAG_DEFAULT_LANGUAGE
#undef HB_OT_MAX_TAGS_PER_SCRIPT
#undef HB_OT_MAX_TAGS_PER_LANGUAGE
#undef HB_OT_LAYOUT_NO_SCRIPT_INDEX
#undef HB_OT_LAYOUT_NO_FEATURE_INDEX
#undef HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX
#undef HB_OT_LAYOUT_NO_VARIATIONS_INDEX
#undef HB_OT_TAG_MATH
#undef HB_OT_TAG_MATH_SCRIPT
#undef HB_OT_SHAPE_BUFFER_FORMAT_SERIAL
#undef HB_OT_TAG_VAR_AXIS_ITALIC
#undef HB_OT_TAG_VAR_AXIS_OPTICAL_SIZE
#undef HB_OT_TAG_VAR_AXIS_SLANT
#undef HB_OT_TAG_VAR_AXIS_WIDTH
#undef HB_OT_TAG_VAR_AXIS_WEIGHT
#undef HB_AAT_LAYOUT_NO_SELECTOR_INDEX
#undef HB_VERSION_STRING
#undef HB_SEGMENT_PROPERTIES_DEFAULT
#undef HB_DRAW_STATE_DEFAULT
#undef HB_SCRIPT_CANADIAN_ABORIGINAL
#undef HB_BUFFER_FLAGS_DEFAULT
#undef HB_BUFFER_SERIALIZE_FLAGS_DEFAULT
#undef HB_UNICODE_COMBINING_CLASS_CCC133
#undef HB_UNICODE_MAX_DECOMPOSITION_LEN
#undef HB_AAT_LAYOUT_FEATURE_TYPE_CURISVE_CONNECTION
#undef HB_MATH_GLYPH_PART_FLAG_EXTENDER
#undef HB_OT_MATH_SCRIPT
#undef HB_OT_VAR_NO_AXIS_INDEX
#undef HB_TAG
#undef HB_COLOR
#undef HB_DIRECTION_IS_VALID
#undef HB_DIRECTION_IS_HORIZONTAL
#undef HB_DIRECTION_IS_VERTICAL
#undef HB_DIRECTION_IS_FORWARD
#undef HB_DIRECTION_IS_BACKWARD
#undef HB_DIRECTION_REVERSE
#undef HB_BUFFER_CLUSTER_LEVEL_IS_MONOTONE
#undef HB_BUFFER_CLUSTER_LEVEL_IS_GRAPHEMES
#undef HB_BUFFER_CLUSTER_LEVEL_IS_CHARACTERS
#undef HB_VERSION_ATLEAST

export module mcpplibs.mwrap.harfbuzz.macros;

namespace c = mwrap_harfbuzz_capture;

export namespace mcpplibs::mwrap::harfbuzz::macros {

// ── constants ────────────────────────────────────────────────────────────
inline constexpr auto HB_CODEPOINT_INVALID = c::value_HB_CODEPOINT_INVALID;
inline constexpr auto HB_TAG_NONE = c::value_HB_TAG_NONE;
inline constexpr auto HB_TAG_MAX = c::value_HB_TAG_MAX;
inline constexpr auto HB_TAG_MAX_SIGNED = c::value_HB_TAG_MAX_SIGNED;
inline constexpr auto HB_LANGUAGE_INVALID = c::value_HB_LANGUAGE_INVALID;
inline constexpr auto HB_FEATURE_GLOBAL_START = c::value_HB_FEATURE_GLOBAL_START;
inline constexpr auto HB_FEATURE_GLOBAL_END = c::value_HB_FEATURE_GLOBAL_END;
inline constexpr auto HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT = c::value_HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT;
inline constexpr auto HB_FONT_NO_VAR_NAMED_INSTANCE = c::value_HB_FONT_NO_VAR_NAMED_INSTANCE;
inline constexpr auto HB_MAP_VALUE_INVALID = c::value_HB_MAP_VALUE_INVALID;
inline constexpr auto HB_SET_VALUE_INVALID = c::value_HB_SET_VALUE_INVALID;
inline constexpr auto HB_UNICODE_MAX = c::value_HB_UNICODE_MAX;
inline constexpr auto HB_PAINT_IMAGE_FORMAT_PNG = c::value_HB_PAINT_IMAGE_FORMAT_PNG;
inline constexpr auto HB_PAINT_IMAGE_FORMAT_SVG = c::value_HB_PAINT_IMAGE_FORMAT_SVG;
inline constexpr auto HB_PAINT_IMAGE_FORMAT_BGRA = c::value_HB_PAINT_IMAGE_FORMAT_BGRA;
inline constexpr auto HB_VERSION_MAJOR = c::value_HB_VERSION_MAJOR;
inline constexpr auto HB_VERSION_MINOR = c::value_HB_VERSION_MINOR;
inline constexpr auto HB_VERSION_MICRO = c::value_HB_VERSION_MICRO;
inline constexpr auto HB_OT_TAG_BASE = c::value_HB_OT_TAG_BASE;
inline constexpr auto HB_OT_TAG_GDEF = c::value_HB_OT_TAG_GDEF;
inline constexpr auto HB_OT_TAG_GSUB = c::value_HB_OT_TAG_GSUB;
inline constexpr auto HB_OT_TAG_GPOS = c::value_HB_OT_TAG_GPOS;
inline constexpr auto HB_OT_TAG_JSTF = c::value_HB_OT_TAG_JSTF;
inline constexpr auto HB_OT_TAG_DEFAULT_SCRIPT = c::value_HB_OT_TAG_DEFAULT_SCRIPT;
inline constexpr auto HB_OT_TAG_DEFAULT_LANGUAGE = c::value_HB_OT_TAG_DEFAULT_LANGUAGE;
inline constexpr auto HB_OT_MAX_TAGS_PER_SCRIPT = c::value_HB_OT_MAX_TAGS_PER_SCRIPT;
inline constexpr auto HB_OT_MAX_TAGS_PER_LANGUAGE = c::value_HB_OT_MAX_TAGS_PER_LANGUAGE;
inline constexpr auto HB_OT_LAYOUT_NO_SCRIPT_INDEX = c::value_HB_OT_LAYOUT_NO_SCRIPT_INDEX;
inline constexpr auto HB_OT_LAYOUT_NO_FEATURE_INDEX = c::value_HB_OT_LAYOUT_NO_FEATURE_INDEX;
inline constexpr auto HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX = c::value_HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX;
inline constexpr auto HB_OT_LAYOUT_NO_VARIATIONS_INDEX = c::value_HB_OT_LAYOUT_NO_VARIATIONS_INDEX;
inline constexpr auto HB_OT_TAG_MATH = c::value_HB_OT_TAG_MATH;
inline constexpr auto HB_OT_TAG_MATH_SCRIPT = c::value_HB_OT_TAG_MATH_SCRIPT;
inline constexpr auto HB_OT_SHAPE_BUFFER_FORMAT_SERIAL = c::value_HB_OT_SHAPE_BUFFER_FORMAT_SERIAL;
inline constexpr auto HB_OT_TAG_VAR_AXIS_ITALIC = c::value_HB_OT_TAG_VAR_AXIS_ITALIC;
inline constexpr auto HB_OT_TAG_VAR_AXIS_OPTICAL_SIZE = c::value_HB_OT_TAG_VAR_AXIS_OPTICAL_SIZE;
inline constexpr auto HB_OT_TAG_VAR_AXIS_SLANT = c::value_HB_OT_TAG_VAR_AXIS_SLANT;
inline constexpr auto HB_OT_TAG_VAR_AXIS_WIDTH = c::value_HB_OT_TAG_VAR_AXIS_WIDTH;
inline constexpr auto HB_OT_TAG_VAR_AXIS_WEIGHT = c::value_HB_OT_TAG_VAR_AXIS_WEIGHT;
inline constexpr auto HB_AAT_LAYOUT_NO_SELECTOR_INDEX = c::value_HB_AAT_LAYOUT_NO_SELECTOR_INDEX;
inline constexpr auto& HB_VERSION_STRING = c::value_HB_VERSION_STRING;

// ── initialisers ─────────────────────────────────────────────────────────
inline constexpr hb_segment_properties_t HB_SEGMENT_PROPERTIES_DEFAULT = c::value_HB_SEGMENT_PROPERTIES_DEFAULT;
inline constexpr hb_draw_state_t HB_DRAW_STATE_DEFAULT = c::value_HB_DRAW_STATE_DEFAULT;

// ── computations: each argument converted as the macro casts it ──────────
constexpr hb_tag_t HB_TAG(auto c1, auto c2, auto c3, auto c4) {
    return (hb_tag_t)((((uint32_t)(c1) & 0xFF) << 24) | (((uint32_t)(c2) & 0xFF) << 16) |
                      (((uint32_t)(c3) & 0xFF) << 8) | ((uint32_t)(c4) & 0xFF));
}
constexpr hb_color_t HB_COLOR(auto b, auto g, auto r, auto a) { return (hb_color_t)HB_TAG(b, g, r, a); }

constexpr bool HB_DIRECTION_IS_VALID(auto dir) { return (((unsigned int)(dir)) & ~3U) == 4; }
constexpr bool HB_DIRECTION_IS_HORIZONTAL(auto dir) { return (((unsigned int)(dir)) & ~1U) == 4; }
constexpr bool HB_DIRECTION_IS_VERTICAL(auto dir) { return (((unsigned int)(dir)) & ~1U) == 6; }
constexpr bool HB_DIRECTION_IS_FORWARD(auto dir) { return (((unsigned int)(dir)) & ~2U) == 4; }
constexpr bool HB_DIRECTION_IS_BACKWARD(auto dir) { return (((unsigned int)(dir)) & ~2U) == 5; }
constexpr hb_direction_t HB_DIRECTION_REVERSE(auto dir) { return (hb_direction_t)(((unsigned int)(dir)) ^ 1); }

constexpr bool HB_BUFFER_CLUSTER_LEVEL_IS_MONOTONE(auto level) {
    return (bool)((1u << (unsigned)(level)) &
                  ((1u << HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES) | (1u << HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS)));
}
constexpr bool HB_BUFFER_CLUSTER_LEVEL_IS_GRAPHEMES(auto level) {
    return (bool)((1u << (unsigned)(level)) &
                  ((1u << HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES) | (1u << HB_BUFFER_CLUSTER_LEVEL_GRAPHEMES)));
}
constexpr bool HB_BUFFER_CLUSTER_LEVEL_IS_CHARACTERS(auto level) {
    return (bool)((1u << (unsigned)(level)) &
                  ((1u << HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS) | (1u << HB_BUFFER_CLUSTER_LEVEL_CHARACTERS)));
}

// A test of the headers' version, as the macro is (HB_VERSION_* are this
// package's: 14.3.0). In #if, use the macro of <mwrap/harfbuzz/macros.hpp>.
constexpr bool HB_VERSION_ATLEAST(auto major, auto minor, auto micro) {
    return (major) * 10000 + (minor) * 100 + (micro) <=
           c::value_HB_VERSION_MAJOR * 10000 + c::value_HB_VERSION_MINOR * 100 + c::value_HB_VERSION_MICRO;
}

// ── deprecated aliases (hb-deprecated.h, hb-ot-deprecated.h) ──────────────
[[deprecated("use HB_SCRIPT_CANADIAN_SYLLABICS")]] inline constexpr auto HB_SCRIPT_CANADIAN_ABORIGINAL = c::value_HB_SCRIPT_CANADIAN_ABORIGINAL;
[[deprecated("use HB_BUFFER_FLAG_DEFAULT")]] inline constexpr auto HB_BUFFER_FLAGS_DEFAULT = c::value_HB_BUFFER_FLAGS_DEFAULT;
[[deprecated("use HB_BUFFER_SERIALIZE_FLAG_DEFAULT")]] inline constexpr auto HB_BUFFER_SERIALIZE_FLAGS_DEFAULT = c::value_HB_BUFFER_SERIALIZE_FLAGS_DEFAULT;
[[deprecated("deprecated since HarfBuzz 7.2.0")]] inline constexpr auto HB_UNICODE_COMBINING_CLASS_CCC133 = c::value_HB_UNICODE_COMBINING_CLASS_CCC133;
[[deprecated("deprecated since HarfBuzz 2.0.0")]] inline constexpr auto HB_UNICODE_MAX_DECOMPOSITION_LEN = c::value_HB_UNICODE_MAX_DECOMPOSITION_LEN;
[[deprecated("use HB_AAT_LAYOUT_FEATURE_TYPE_CURSIVE_CONNECTION")]] inline constexpr auto HB_AAT_LAYOUT_FEATURE_TYPE_CURISVE_CONNECTION = c::value_HB_AAT_LAYOUT_FEATURE_TYPE_CURISVE_CONNECTION;
[[deprecated("use HB_OT_MATH_GLYPH_PART_FLAG_EXTENDER")]] inline constexpr auto HB_MATH_GLYPH_PART_FLAG_EXTENDER = c::value_HB_MATH_GLYPH_PART_FLAG_EXTENDER;
[[deprecated("use HB_OT_TAG_MATH_SCRIPT")]] inline constexpr auto HB_OT_MATH_SCRIPT = c::value_HB_OT_MATH_SCRIPT;
[[deprecated("deprecated since HarfBuzz 2.2.0")]] inline constexpr auto HB_OT_VAR_NO_AXIS_INDEX = c::value_HB_OT_VAR_NO_AXIS_INDEX;

}  // namespace mcpplibs::mwrap::harfbuzz::macros
