// hb-common, hb-set, hb-map and hb-unicode through the module: tags,
// directions, scripts, languages, features, versions; set and map algebra and
// iteration; the Unicode Character Database functions, and Unicode functions
// replaced one at a time over the default ones.
#include <gtest/gtest.h>

import std;
import mcpplibs.mwrap.harfbuzz;

using namespace mcpplibs::mwrap::harfbuzz::macros;

TEST(Common, TagsDirectionsScriptsLanguages) {
    EXPECT_EQ(hb_tag_from_string("liga", -1), HB_TAG('l', 'i', 'g', 'a'));
    EXPECT_EQ(hb_tag_from_string("ab", -1), HB_TAG('a', 'b', ' ', ' '));
    char s[4];
    hb_tag_to_string(HB_TAG('k', 'e', 'r', 'n'), s);
    EXPECT_EQ(std::string_view(s, 4), "kern");
    EXPECT_EQ(hb_direction_from_string("rtl", -1), HB_DIRECTION_RTL);
    EXPECT_STREQ(hb_direction_to_string(HB_DIRECTION_TTB), "ttb");
    EXPECT_EQ(HB_DIRECTION_REVERSE(HB_DIRECTION_LTR), HB_DIRECTION_RTL);
    EXPECT_EQ(hb_script_from_iso15924_tag(HB_TAG('A', 'r', 'a', 'b')), HB_SCRIPT_ARABIC);
    EXPECT_EQ(hb_script_from_string("Deva", -1), HB_SCRIPT_DEVANAGARI);
    EXPECT_EQ(hb_script_to_iso15924_tag(HB_SCRIPT_LATIN), HB_TAG('L', 'a', 't', 'n'));
    EXPECT_EQ(hb_script_get_horizontal_direction(HB_SCRIPT_HEBREW), HB_DIRECTION_RTL);
    EXPECT_EQ(hb_script_get_horizontal_direction(HB_SCRIPT_LATIN), HB_DIRECTION_LTR);
    hb_language_t en = hb_language_from_string("EN_us", -1);
    EXPECT_STREQ(hb_language_to_string(en), "en-us");
    EXPECT_EQ(en, hb_language_from_string("en-US", -1));  // interned: one pointer per language
    EXPECT_TRUE(hb_language_matches(hb_language_from_string("en", -1), en));
    EXPECT_NE(hb_language_get_default(), HB_LANGUAGE_INVALID);
    EXPECT_EQ(hb_language_from_string("", 0), HB_LANGUAGE_INVALID);
    hb_feature_t feature;
    ASSERT_TRUE(hb_feature_from_string("aalt[3:5]=2", -1, &feature));
    EXPECT_EQ(feature.value, 2u);
    EXPECT_EQ(feature.start, 3u);
    EXPECT_EQ(feature.end, 5u);
    EXPECT_FALSE(hb_feature_from_string("=", -1, &feature));
    unsigned major = 0, minor = 0, micro = 0;
    hb_version(&major, &minor, &micro);
    EXPECT_EQ(major, 14u);
    EXPECT_EQ(minor, 3u);
    EXPECT_STREQ(hb_version_string(), "14.3.0");
    EXPECT_TRUE(hb_version_atleast(14, 3, 0));
    EXPECT_FALSE(hb_version_atleast(14, 3, 1));
    EXPECT_STREQ(hb_version_string(), HB_VERSION_STRING);
    EXPECT_EQ(hb_color_get_alpha(HB_COLOR(1, 2, 3, 4)), 4);
}

TEST(Set, AlgebraAndIteration) {
    hb_set_t* a = hb_set_create();
    hb_set_add_range(a, 10, 19);
    hb_set_add(a, 100);
    hb_set_del(a, 15);
    EXPECT_EQ(hb_set_get_population(a), 10u);
    EXPECT_TRUE(hb_set_has(a, 14));
    EXPECT_FALSE(hb_set_has(a, 15));
    EXPECT_EQ(hb_set_get_min(a), 10u);
    EXPECT_EQ(hb_set_get_max(a), 100u);
    hb_codepoint_t first = HB_SET_VALUE_INVALID, last = HB_SET_VALUE_INVALID;
    std::vector<std::pair<hb_codepoint_t, hb_codepoint_t>> ranges;
    while (hb_set_next_range(a, &first, &last)) ranges.emplace_back(first, last);
    EXPECT_EQ(ranges, (std::vector<std::pair<hb_codepoint_t, hb_codepoint_t>>{{10, 14}, {16, 19}, {100, 100}}));
    hb_codepoint_t c = HB_SET_VALUE_INVALID;
    ASSERT_TRUE(hb_set_previous(a, &c));
    EXPECT_EQ(c, 100u);
    hb_codepoint_t out[4];
    EXPECT_EQ(hb_set_next_many(a, HB_SET_VALUE_INVALID, out, 4), 4u);
    EXPECT_EQ(out[3], 13u);
    hb_set_t* b = hb_set_copy(a);
    EXPECT_TRUE(hb_set_is_equal(a, b));
    EXPECT_EQ(hb_set_hash(a), hb_set_hash(b));
    hb_set_clear(b);
    hb_set_add_range(b, 18, 30);
    hb_set_t* u = hb_set_copy(a);
    hb_set_union(u, b);
    EXPECT_EQ(hb_set_get_population(u), 21u);  // 10–14, 16–30, 100
    hb_set_t* i = hb_set_copy(a);
    hb_set_intersect(i, b);
    EXPECT_EQ(hb_set_get_population(i), 2u);
    hb_set_t* d = hb_set_copy(a);
    hb_set_subtract(d, b);
    EXPECT_EQ(hb_set_get_population(d), 8u);
    hb_set_t* x = hb_set_copy(a);
    hb_set_symmetric_difference(x, b);
    EXPECT_EQ(hb_set_get_population(x), 19u);  // 10 + 13 - 2 * {18, 19}
    EXPECT_TRUE(hb_set_is_subset(i, a));
    hb_set_invert(i);
    EXPECT_TRUE(hb_set_is_inverted(i));
    EXPECT_FALSE(hb_set_has(i, 18));
    EXPECT_TRUE(hb_set_has(i, 0x10FFFF));
    hb_set_clear(i);
    EXPECT_TRUE(hb_set_is_empty(i));
    hb_set_add_sorted_array(i, out, 4);
    EXPECT_EQ(hb_set_get_population(i), 4u);
    hb_set_del_range(i, 10, 11);
    EXPECT_EQ(hb_set_get_min(i), 12u);
    for (hb_set_t* s : {a, b, u, i, d, x}) hb_set_destroy(s);
    EXPECT_TRUE(hb_set_is_empty(hb_set_get_empty()));
}

TEST(Map, SetGetIterate) {
    hb_map_t* m = hb_map_create();
    hb_map_set(m, 1, 10);
    hb_map_set(m, 2, 20);
    hb_map_set(m, 3, 30);
    hb_map_del(m, 2);
    EXPECT_EQ(hb_map_get_population(m), 2u);
    EXPECT_EQ(hb_map_get(m, 3), 30u);
    EXPECT_EQ(hb_map_get(m, 2), HB_MAP_VALUE_INVALID);
    EXPECT_TRUE(hb_map_has(m, 1));
    std::map<hb_codepoint_t, hb_codepoint_t> seen;
    int idx = -1;
    hb_codepoint_t key = 0, value = 0;
    while (hb_map_next(m, &idx, &key, &value)) seen[key] = value;
    EXPECT_EQ(seen, (std::map<hb_codepoint_t, hb_codepoint_t>{{1, 10}, {3, 30}}));
    hb_set_t* keys = hb_set_create();
    hb_set_t* values = hb_set_create();
    hb_map_keys(m, keys);
    hb_map_values(m, values);
    EXPECT_TRUE(hb_set_has(keys, 3) && hb_set_has(values, 30));
    hb_map_t* copy = hb_map_copy(m);
    EXPECT_TRUE(hb_map_is_equal(m, copy));
    EXPECT_EQ(hb_map_hash(m), hb_map_hash(copy));
    hb_map_t* other = hb_map_create();
    hb_map_set(other, 4, 40);
    hb_map_update(copy, other);
    EXPECT_EQ(hb_map_get_population(copy), 3u);
    hb_map_clear(copy);
    EXPECT_TRUE(hb_map_is_empty(copy));
    EXPECT_TRUE(hb_map_allocation_successful(m));
    hb_set_destroy(values);
    hb_set_destroy(keys);
    hb_map_destroy(other);
    hb_map_destroy(copy);
    hb_map_destroy(m);
}

TEST(Unicode, DefaultFunctions) {
    hb_unicode_funcs_t* u = hb_unicode_funcs_get_default();
    EXPECT_TRUE(hb_unicode_funcs_is_immutable(u));
    EXPECT_EQ(hb_unicode_general_category(u, 'a'), HB_UNICODE_GENERAL_CATEGORY_LOWERCASE_LETTER);
    EXPECT_EQ(hb_unicode_general_category(u, 0x0301), HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK);
    EXPECT_EQ(hb_unicode_combining_class(u, 0x0301), HB_UNICODE_COMBINING_CLASS_ABOVE);
    EXPECT_EQ(hb_unicode_mirroring(u, '('), static_cast<hb_codepoint_t>(')'));
    EXPECT_EQ(hb_unicode_script(u, 0x0915), HB_SCRIPT_DEVANAGARI);
    EXPECT_EQ(hb_unicode_script(u, 0x0627), HB_SCRIPT_ARABIC);
    EXPECT_EQ(hb_unicode_script(u, ' '), HB_SCRIPT_COMMON);
    hb_codepoint_t ab = 0;
    ASSERT_TRUE(hb_unicode_compose(u, 'e', 0x0301, &ab));
    EXPECT_EQ(ab, 0xE9u);
    hb_codepoint_t a = 0, b = 0;
    ASSERT_TRUE(hb_unicode_decompose(u, 0xE9, &a, &b));
    EXPECT_EQ(a, static_cast<hb_codepoint_t>('e'));
    EXPECT_EQ(b, 0x0301u);
    EXPECT_GE(HB_UNICODE_MAX, 0x10FFFFu);
}

TEST(Unicode, CustomFunctionsOverTheDefault) {
    hb_unicode_funcs_t* u = hb_unicode_funcs_create(hb_unicode_funcs_get_default());
    EXPECT_EQ(hb_unicode_funcs_get_parent(u), hb_unicode_funcs_get_default());
    hb_unicode_funcs_set_script_func(
        u, [](hb_unicode_funcs_t*, hb_codepoint_t cp, void*) { return cp == 'x' ? HB_SCRIPT_GREEK : HB_SCRIPT_LATIN; }, nullptr,
        nullptr);
    hb_unicode_funcs_make_immutable(u);
    EXPECT_EQ(hb_unicode_script(u, 'x'), HB_SCRIPT_GREEK);
    EXPECT_EQ(hb_unicode_general_category(u, 'x'), HB_UNICODE_GENERAL_CATEGORY_LOWERCASE_LETTER);  // the parent's
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_set_unicode_funcs(buffer, u);
    hb_buffer_add_utf8(buffer, "x", -1, 0, -1);
    hb_buffer_guess_segment_properties(buffer);
    EXPECT_EQ(hb_buffer_get_script(buffer), HB_SCRIPT_GREEK);
    hb_buffer_destroy(buffer);
    hb_unicode_funcs_destroy(u);
    EXPECT_NE(hb_unicode_funcs_get_empty(), nullptr);
}
