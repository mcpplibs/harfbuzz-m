// The macro mappings alone (import mcpplibs.mwrap.harfbuzz.macros;): each
// constant has its macro's value and type, each computation is usable in a
// constant expression, as the macros are.
import mcpplibs.mwrap.harfbuzz.macros;
namespace macros = mcpplibs::mwrap::harfbuzz::macros;

static_assert(macros::HB_TAG('G', 'S', 'U', 'B') == 0x47535542u);
static_assert(macros::HB_OT_TAG_GSUB == macros::HB_TAG('G', 'S', 'U', 'B'));
static_assert(macros::HB_OT_TAG_VAR_AXIS_WEIGHT == macros::HB_TAG('w', 'g', 'h', 't'));
static_assert(macros::HB_OT_TAG_DEFAULT_SCRIPT == macros::HB_TAG('D', 'F', 'L', 'T'));
static_assert(macros::HB_PAINT_IMAGE_FORMAT_PNG == macros::HB_TAG('p', 'n', 'g', ' '));
static_assert(macros::HB_TAG(-1, 0x1FF, 0, 0) == 0xFFFF0000u);  // each argument masked to 8 bits, as the macro casts
static_assert(macros::HB_TAG_NONE == 0 && macros::HB_TAG_MAX == 0xFFFFFFFFu && macros::HB_TAG_MAX_SIGNED == 0x7FFFFFFFu);
static_assert(macros::HB_COLOR(1, 2, 3, 4) == 0x01020304u);
static_assert(macros::HB_CODEPOINT_INVALID == 0xFFFFFFFFu && macros::HB_SET_VALUE_INVALID == macros::HB_CODEPOINT_INVALID);
static_assert(macros::HB_MAP_VALUE_INVALID == macros::HB_CODEPOINT_INVALID);
static_assert(macros::HB_FEATURE_GLOBAL_START == 0 && macros::HB_FEATURE_GLOBAL_END == static_cast<unsigned>(-1));
static_assert(macros::HB_LANGUAGE_INVALID == nullptr);
static_assert(macros::HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT == 0xFFFDu);
static_assert(macros::HB_UNICODE_MAX == 0x10FFFFu);
static_assert(macros::HB_FONT_NO_VAR_NAMED_INSTANCE == 0xFFFFFFFFu);
static_assert(macros::HB_OT_LAYOUT_NO_SCRIPT_INDEX == 0xFFFFu && macros::HB_OT_LAYOUT_NO_VARIATIONS_INDEX == 0xFFFFFFFFu);
static_assert(macros::HB_OT_MAX_TAGS_PER_SCRIPT == 3u && macros::HB_OT_MAX_TAGS_PER_LANGUAGE == 3u);
static_assert(macros::HB_OT_SHAPE_BUFFER_FORMAT_SERIAL == 1);
static_assert(macros::HB_AAT_LAYOUT_NO_SELECTOR_INDEX == 0xFFFFu);
static_assert(macros::HB_VERSION_MAJOR == 14 && macros::HB_VERSION_MINOR == 3 && macros::HB_VERSION_MICRO == 0);
static_assert(macros::HB_VERSION_STRING[0] == '1' && sizeof(macros::HB_VERSION_STRING) == 7);
static_assert(macros::HB_VERSION_ATLEAST(14, 3, 0) && macros::HB_VERSION_ATLEAST(1, 0, 0) && !macros::HB_VERSION_ATLEAST(14, 4, 0));
// Directions: LTR 4, RTL 5, TTB 6, BTT 7 (hb_direction_t).
static_assert(macros::HB_DIRECTION_IS_VALID(4) && !macros::HB_DIRECTION_IS_VALID(0));
static_assert(macros::HB_DIRECTION_IS_HORIZONTAL(5) && macros::HB_DIRECTION_IS_VERTICAL(7));
static_assert(macros::HB_DIRECTION_IS_FORWARD(6) && macros::HB_DIRECTION_IS_BACKWARD(5));
static_assert(macros::HB_DIRECTION_REVERSE(4) == 5 && macros::HB_DIRECTION_REVERSE(7) == 6);
// Cluster levels: 0 monotone graphemes, 1 monotone characters, 2 characters, 3 graphemes.
static_assert(macros::HB_BUFFER_CLUSTER_LEVEL_IS_MONOTONE(0) && macros::HB_BUFFER_CLUSTER_LEVEL_IS_MONOTONE(1));
static_assert(macros::HB_BUFFER_CLUSTER_LEVEL_IS_GRAPHEMES(3) && !macros::HB_BUFFER_CLUSTER_LEVEL_IS_GRAPHEMES(2));
static_assert(macros::HB_BUFFER_CLUSTER_LEVEL_IS_CHARACTERS(2) && !macros::HB_BUFFER_CLUSTER_LEVEL_IS_CHARACTERS(0));
static_assert(macros::HB_SEGMENT_PROPERTIES_DEFAULT.direction == 0 && macros::HB_SEGMENT_PROPERTIES_DEFAULT.language == nullptr);
static_assert(macros::HB_SEGMENT_PROPERTIES_DEFAULT.script == 0);
static_assert(!macros::HB_DRAW_STATE_DEFAULT.path_open && macros::HB_DRAW_STATE_DEFAULT.current_x == 0.f);

// The deprecated aliases: their replacements' values, and the deprecation.
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
static_assert(macros::HB_OT_MATH_SCRIPT == macros::HB_OT_TAG_MATH_SCRIPT);
static_assert(macros::HB_OT_VAR_NO_AXIS_INDEX == 0xFFFFFFFFu);
static_assert(macros::HB_UNICODE_MAX_DECOMPOSITION_LEN == 19 && macros::HB_UNICODE_COMBINING_CLASS_CCC133 == 133);
static_assert(macros::HB_BUFFER_FLAGS_DEFAULT == 0 && macros::HB_BUFFER_SERIALIZE_FLAGS_DEFAULT == 0);
static_assert(macros::HB_SCRIPT_CANADIAN_ABORIGINAL == macros::HB_TAG('C', 'a', 'n', 's'));
static_assert(macros::HB_AAT_LAYOUT_FEATURE_TYPE_CURISVE_CONNECTION == 2 && macros::HB_MATH_GLYPH_PART_FLAG_EXTENDER == 1);
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

int main() { return 0; }
