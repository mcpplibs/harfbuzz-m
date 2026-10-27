// The raw headers and the import in one translation unit (the side header
// first, the canonical order): the module's names are the very same entities;
// the macro a function cannot be (HB_UNTAG) and the preprocessor test
// HB_VERSION_ATLEAST work beside them; the headers' own values equal the
// mapped constants. While a header's macro is defined it expands even inside
// a qualified name, so `macros::HB_TAG_NONE` is only reachable with the macro
// set aside.
#include <mwrap/harfbuzz/macros.hpp>
#include <gtest/gtest.h>

import std;
import mcpplibs.mwrap.harfbuzz;

#if !HB_VERSION_ATLEAST(14, 0, 0)
#error "the side header is HarfBuzz 14's"
#endif

namespace {
constexpr hb_tag_t raw_gsub = HB_OT_TAG_GSUB, raw_none = HB_TAG_NONE;
constexpr unsigned raw_end = HB_FEATURE_GLOBAL_END;
}  // namespace

#pragma push_macro("HB_OT_TAG_GSUB")
#pragma push_macro("HB_TAG_NONE")
#pragma push_macro("HB_FEATURE_GLOBAL_END")
#pragma push_macro("HB_TAG")
#undef HB_OT_TAG_GSUB
#undef HB_TAG_NONE
#undef HB_FEATURE_GLOBAL_END
#undef HB_TAG
static_assert(mcpplibs::mwrap::harfbuzz::macros::HB_OT_TAG_GSUB == raw_gsub);
static_assert(mcpplibs::mwrap::harfbuzz::macros::HB_TAG_NONE == raw_none);
static_assert(mcpplibs::mwrap::harfbuzz::macros::HB_FEATURE_GLOBAL_END == raw_end);
static_assert(mcpplibs::mwrap::harfbuzz::macros::HB_TAG('a', 'b', 'c', 'd') == 0x61626364u);
#pragma pop_macro("HB_TAG")
#pragma pop_macro("HB_FEATURE_GLOBAL_END")
#pragma pop_macro("HB_TAG_NONE")
#pragma pop_macro("HB_OT_TAG_GSUB")

TEST(HeaderMix, SameEntitiesAndValues) {
    static_assert(std::is_same_v<decltype(&::hb_buffer_create), hb_buffer_t* (*)()>);
    static_assert(std::is_same_v<hb_glyph_info_t, ::hb_glyph_info_t>);
    static_assert(std::is_same_v<decltype(HB_SCRIPT_ARABIC), hb_script_t>);
    // HB_UNTAG: four arguments of a call.
    const std::string tag = std::format("{:c}{:c}{:c}{:c}", HB_UNTAG(HB_TAG('k', 'e', 'r', 'n')));
    EXPECT_EQ(tag, "kern");
    // The header's macro over the function of the same name, and the function.
    const hb_color_t c = HB_COLOR(1, 2, 3, 4);
    EXPECT_EQ(hb_color_get_red(c), 3);
    EXPECT_EQ((hb_color_get_red)(c), 3);
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, "a", -1, 0, -1);
    hb_segment_properties_t props = HB_SEGMENT_PROPERTIES_DEFAULT;  // the header's initialiser
    EXPECT_EQ(props.direction, HB_DIRECTION_INVALID);
    hb_buffer_destroy(buffer);
}
