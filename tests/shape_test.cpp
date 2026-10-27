// Shaping through the module: Latin (ligatures, kerning, features), Arabic
// (joining forms, marks, right-to-left) and Devanagari (reordering,
// conjuncts) with the glyphs and advances an independent HarfBuzz build
// (uharfbuzz 0.57.0, HarfBuzz 15.0.0) gives for the same fonts (its clusters
// are character indices, these UTF-8 byte offsets); buffers, their properties, cluster
// levels, serialisation both ways, comparison, normalisation, the message
// callback and shape plans — spelled as hb.h documents them.
#include <gtest/gtest.h>

import std;
import mcpplibs.mwrap.harfbuzz;

using namespace mcpplibs::mwrap::harfbuzz::macros;  // HB_TAG, HB_FEATURE_GLOBAL_START …

namespace {

std::string font_path(std::string_view name) {
    const std::filesystem::path here = std::filesystem::path(std::source_location::current().file_name()).parent_path();
    const std::filesystem::path beside = here / "fonts" / name;
    return (std::filesystem::exists(beside) ? beside : std::filesystem::path("tests/fonts") / name).string();
}

struct Font {
    hb_blob_t* blob;
    hb_face_t* face;
    hb_font_t* font;
    explicit Font(std::string_view name)
        : blob(hb_blob_create_from_file_or_fail(font_path(name).c_str())),
          face(hb_face_create(blob, 0)),
          font(hb_font_create(face)) {}
    ~Font() {
        hb_font_destroy(font);
        hb_face_destroy(face);
        hb_blob_destroy(blob);
    }
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
};

std::string serialize(hb_buffer_t* buffer, hb_font_t* font,
                      hb_buffer_serialize_format_t format = HB_BUFFER_SERIALIZE_FORMAT_TEXT,
                      hb_buffer_serialize_flags_t flags = HB_BUFFER_SERIALIZE_FLAG_DEFAULT) {
    std::string out;
    const unsigned n = hb_buffer_get_length(buffer);
    for (unsigned start = 0; start < n;) {
        char buf[4096];
        unsigned consumed = 0;
        start += hb_buffer_serialize(buffer, start, n, buf, sizeof buf, &consumed, font, format, flags);
        if (!consumed) break;
        out += buf;
    }
    return out;
}

std::string shape(hb_font_t* font, std::string_view text, std::span<const hb_feature_t> features = {}) {
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, text.data(), static_cast<int>(text.size()), 0, static_cast<int>(text.size()));
    hb_buffer_guess_segment_properties(buffer);
    hb_shape(font, buffer, features.data(), static_cast<unsigned>(features.size()));
    std::string out = serialize(buffer, font);
    hb_buffer_destroy(buffer);
    return out;
}

}  // namespace

TEST(Shape, Latin) {
    Font f("NotoSans-Latin.ttf");
    ASSERT_GT(hb_face_get_glyph_count(f.face), 300u);
    // f_f_i: the ligature, at the cluster of its first letter; AVA: kerned.
    EXPECT_EQ(shape(f.font, "office AVA"), "[o=0+605|f_f_i=1+946|c=4+480|e=5+564|space=6+260|A=7+599|V=8+560|A=9+639]");
    EXPECT_EQ(shape(f.font, "Wave"), "[W=0+910|a=1+561|v=2+508|e=3+564]");
    // Features: liga off everywhere, kern off for the first two letters only.
    hb_feature_t features[2];
    ASSERT_TRUE(hb_feature_from_string("-liga", -1, &features[0]));
    ASSERT_TRUE(hb_feature_from_string("kern[0:2]=0", -1, &features[1]));
    EXPECT_EQ(features[0].tag, HB_TAG('l', 'i', 'g', 'a'));
    EXPECT_EQ(features[0].start, HB_FEATURE_GLOBAL_START);
    EXPECT_EQ(features[0].end, HB_FEATURE_GLOBAL_END);
    EXPECT_EQ(features[1].end, 2u);
    EXPECT_EQ(shape(f.font, "office", features), "[o=0+605|f=1+344|f=2+344|i=3+258|c=4+480|e=5+564]");
    char text[64];
    hb_feature_to_string(&features[1], text, sizeof text);
    EXPECT_STREQ(text, "-kern[:2]");
}

TEST(Shape, Arabic) {
    Font f("NotoSansArabic-Regular.ttf");
    // سلام: right to left, initial / medial lam-alef ligature parts / final.
    EXPECT_EQ(shape(f.font, "سلام"), "[uni0645=6+484|uni0627.fina.rlig.2=4+379|uni0644.medi.rlig=2+220|uni0633.init=0+784]");
    // مرحبا: a dot below, positioned by GPOS mark attachment.
    EXPECT_EQ(shape(f.font, "مرحبا"),
              "[uni0627.fina=8+291|dotbelowar=6@94,-3+0|uni066E.medi=6+343|uni062D.init=4+589|uni0631.fina=2@-40,0+356|uni0645.init=0+525]");
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, "سلام", -1, 0, -1);
    hb_buffer_guess_segment_properties(buffer);
    EXPECT_EQ(hb_buffer_get_direction(buffer), HB_DIRECTION_RTL);
    EXPECT_EQ(hb_buffer_get_script(buffer), HB_SCRIPT_ARABIC);
    EXPECT_TRUE(HB_DIRECTION_IS_BACKWARD(hb_buffer_get_direction(buffer)));
    hb_buffer_destroy(buffer);
}

TEST(Shape, Devanagari) {
    Font f("NotoSansDevanagari-Regular.ttf");
    // The vowel sign i reorders before its consonant cluster; क्ष is a conjunct.
    EXPECT_EQ(shape(f.font, "क्षि"), "[uni093F.10=0+259|uni0915094D0937=0+717]");
    EXPECT_EQ(shape(f.font, "हिन्दी"), "[uni093F.03=0+259|uni0939=0+531|uni0928094D=6+309|uni0926=12+531|uni0940=12+259]");
    // A dotted circle for a mark with no base, unless the buffer says not to.
    EXPECT_EQ(shape(f.font, "ि"), "[uni093F=0+259|uni25CC=0+510]");
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, "ि", -1, 0, -1);
    hb_buffer_guess_segment_properties(buffer);
    hb_buffer_set_flags(buffer, HB_BUFFER_FLAG_DO_NOT_INSERT_DOTTED_CIRCLE);
    hb_shape(f.font, buffer, nullptr, 0);
    EXPECT_EQ(hb_buffer_get_length(buffer), 1u);
    hb_buffer_destroy(buffer);
}

TEST(Shape, ShapersAndPlans) {
    Font f("NotoSans-Latin.ttf");
    const char** shapers = hb_shape_list_shapers();
    std::vector<std::string_view> names;
    for (const char** s = shapers; *s; ++s) names.emplace_back(*s);
    EXPECT_EQ(names, (std::vector<std::string_view>{"ot", "fallback"}));
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, "fi", -1, 0, -1);
    hb_buffer_guess_segment_properties(buffer);
    const char* fallback[] = {"fallback", nullptr};
    ASSERT_TRUE(hb_shape_full(f.font, buffer, nullptr, 0, fallback));
    EXPECT_EQ(serialize(buffer, f.font), "[f=0+344|i=1+258]");  // no GSUB: no ligature
    hb_segment_properties_t props = HB_SEGMENT_PROPERTIES_DEFAULT;
    props.direction = HB_DIRECTION_LTR;
    props.script = HB_SCRIPT_LATIN;
    props.language = hb_language_from_string("en", -1);
    hb_shape_plan_t* plan = hb_shape_plan_create_cached(f.face, &props, nullptr, 0, nullptr);
    EXPECT_STREQ(hb_shape_plan_get_shaper(plan), "ot");
    hb_set_t* lookups = hb_set_create();
    hb_ot_shape_plan_collect_lookups(plan, HB_OT_TAG_GSUB, lookups);
    EXPECT_GT(hb_set_get_population(lookups), 0u);
    hb_buffer_clear_contents(buffer);
    hb_buffer_add_utf8(buffer, "fi", -1, 0, -1);
    hb_buffer_set_segment_properties(buffer, &props);
    ASSERT_TRUE(hb_shape_plan_execute(plan, f.font, buffer, nullptr, 0));
    EXPECT_EQ(serialize(buffer, f.font), "[fi=0+602]");
    hb_set_destroy(lookups);
    hb_shape_plan_destroy(plan);
    hb_buffer_destroy(buffer);
}

TEST(Buffer, ContentsAndProperties) {
    hb_buffer_t* buffer = hb_buffer_create();
    ASSERT_TRUE(hb_buffer_allocation_successful(buffer));
    EXPECT_EQ(hb_buffer_get_content_type(buffer), HB_BUFFER_CONTENT_TYPE_INVALID);
    const std::u16string utf16 = u"aé\U0001F600";
    hb_buffer_add_utf16(buffer, reinterpret_cast<const std::uint16_t*>(utf16.data()), static_cast<int>(utf16.size()), 0, -1);
    EXPECT_EQ(hb_buffer_get_content_type(buffer), HB_BUFFER_CONTENT_TYPE_UNICODE);
    const std::uint32_t utf32[] = {0x5B57};
    hb_buffer_add_utf32(buffer, utf32, 1, 0, 1);
    const std::uint8_t latin1[] = {0xE9};
    hb_buffer_add_latin1(buffer, latin1, 1, 0, 1);
    const hb_codepoint_t cps[] = {0x10FFFF};
    hb_buffer_add_codepoints(buffer, cps, 1, 0, 1);
    hb_buffer_add(buffer, 0x41, 9);
    unsigned n = 0;
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buffer, &n);
    ASSERT_EQ(n, 7u);
    const hb_codepoint_t expected[] = {0x61, 0xE9, 0x1F600, 0x5B57, 0xE9, 0x10FFFF, 0x41};
    for (unsigned i = 0; i < n; i++) EXPECT_EQ(info[i].codepoint, expected[i]) << i;
    EXPECT_EQ(info[2].cluster, 2u);  // UTF-16 code unit offset
    EXPECT_EQ(info[6].cluster, 9u);
    hb_buffer_reverse(buffer);
    EXPECT_EQ(hb_buffer_get_glyph_infos(buffer, nullptr)[0].codepoint, 0x41u);
    hb_buffer_reverse_range(buffer, 0, 2);
    EXPECT_EQ(hb_buffer_get_glyph_infos(buffer, nullptr)[0].codepoint, 0x10FFFFu);
    // Invalid UTF-8 becomes the replacement code point, U+FFFD by default.
    hb_buffer_reset(buffer);
    EXPECT_EQ(hb_buffer_get_replacement_codepoint(buffer), HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT);
    hb_buffer_set_replacement_codepoint(buffer, '?');
    hb_buffer_add_utf8(buffer, "a\xff" "b", -1, 0, -1);
    info = hb_buffer_get_glyph_infos(buffer, &n);
    ASSERT_EQ(n, 3u);
    EXPECT_EQ(info[1].codepoint, static_cast<hb_codepoint_t>('?'));
    // Properties and their defaults.
    hb_buffer_set_language(buffer, hb_language_from_string("zh-Hant", -1));
    EXPECT_STREQ(hb_language_to_string(hb_buffer_get_language(buffer)), "zh-hant");
    hb_buffer_set_cluster_level(buffer, HB_BUFFER_CLUSTER_LEVEL_CHARACTERS);
    EXPECT_EQ(hb_buffer_get_cluster_level(buffer), HB_BUFFER_CLUSTER_LEVEL_CHARACTERS);
    EXPECT_TRUE(HB_BUFFER_CLUSTER_LEVEL_IS_CHARACTERS(hb_buffer_get_cluster_level(buffer)));
    EXPECT_FALSE(HB_BUFFER_CLUSTER_LEVEL_IS_MONOTONE(hb_buffer_get_cluster_level(buffer)));
    hb_buffer_set_invisible_glyph(buffer, 3);
    EXPECT_EQ(hb_buffer_get_invisible_glyph(buffer), 3u);
    hb_buffer_set_not_found_glyph(buffer, 1);
    EXPECT_EQ(hb_buffer_get_not_found_glyph(buffer), 1u);
    hb_buffer_set_not_found_variation_selector_glyph(buffer, 2);
    EXPECT_EQ(hb_buffer_get_not_found_variation_selector_glyph(buffer), 2u);
    hb_buffer_set_random_state(buffer, 42);
    EXPECT_EQ(hb_buffer_get_random_state(buffer), 42u);
    hb_buffer_set_flags(buffer, static_cast<hb_buffer_flags_t>(HB_BUFFER_FLAG_BOT | HB_BUFFER_FLAG_EOT));
    EXPECT_EQ(hb_buffer_get_flags(buffer), HB_BUFFER_FLAG_BOT | HB_BUFFER_FLAG_EOT);
    EXPECT_EQ(hb_buffer_get_unicode_funcs(buffer), hb_unicode_funcs_get_default());
    hb_buffer_t* similar = hb_buffer_create_similar(buffer);
    EXPECT_EQ(hb_buffer_get_flags(similar), hb_buffer_get_flags(buffer));
    EXPECT_EQ(hb_buffer_get_length(similar), 0u);
    hb_buffer_append(similar, buffer, 0, 2);
    EXPECT_EQ(hb_buffer_get_length(similar), 2u);
    EXPECT_TRUE(hb_buffer_set_length(similar, 1));
    EXPECT_TRUE(hb_buffer_pre_allocate(similar, 1024));
    hb_segment_properties_t a, b;
    hb_buffer_set_direction(buffer, HB_DIRECTION_TTB);
    hb_buffer_get_segment_properties(buffer, &a);
    hb_buffer_get_segment_properties(buffer, &b);
    EXPECT_TRUE(hb_segment_properties_equal(&a, &b));
    EXPECT_EQ(hb_segment_properties_hash(&a), hb_segment_properties_hash(&b));
    hb_segment_properties_t c = HB_SEGMENT_PROPERTIES_DEFAULT;
    hb_segment_properties_overlay(&c, &a);
    EXPECT_EQ(c.direction, HB_DIRECTION_TTB);
    EXPECT_TRUE(HB_DIRECTION_IS_VERTICAL(c.direction));
    hb_buffer_destroy(similar);
    hb_buffer_destroy(buffer);
    EXPECT_EQ(hb_buffer_get_length(hb_buffer_get_empty()), 0u);
}

TEST(Buffer, ClusterLevels) {
    Font f("NotoSansDevanagari-Regular.ttf");
    auto clusters = [&](hb_buffer_cluster_level_t level) {
        hb_buffer_t* buffer = hb_buffer_create();
        hb_buffer_add_utf8(buffer, "हिन्दी", -1, 0, -1);
        hb_buffer_guess_segment_properties(buffer);
        hb_buffer_set_cluster_level(buffer, level);
        hb_shape(f.font, buffer, nullptr, 0);
        std::string out = serialize(buffer, f.font, HB_BUFFER_SERIALIZE_FORMAT_TEXT,
                                    static_cast<hb_buffer_serialize_flags_t>(HB_BUFFER_SERIALIZE_FLAG_NO_POSITIONS |
                                                                             HB_BUFFER_SERIALIZE_FLAG_NO_GLYPH_NAMES));
        hb_buffer_destroy(buffer);
        return out;
    };
    // UTF-8 byte offsets of the clusters: हि at 0, न्द at 6, ी at 15.
    EXPECT_EQ(clusters(HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES), "[412=0|78=0|135=6|63=12|23=12]");
    EXPECT_EQ(clusters(HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS), "[412=0|78=0|135=6|63=12|23=15]");
    EXPECT_EQ(clusters(HB_BUFFER_CLUSTER_LEVEL_CHARACTERS), "[412=3|78=0|135=6|63=12|23=15]");
    EXPECT_EQ(clusters(HB_BUFFER_CLUSTER_LEVEL_GRAPHEMES), "[412=0|78=0|135=6|63=12|23=12]");
}

TEST(Buffer, SerialiseDeserialiseDiff) {
    Font f("NotoSans-Latin.ttf");
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, "Af", -1, 0, -1);
    hb_buffer_guess_segment_properties(buffer);
    hb_shape(f.font, buffer, nullptr, 0);
    const std::string json = serialize(buffer, f.font, HB_BUFFER_SERIALIZE_FORMAT_JSON);
    EXPECT_EQ(json, R"([{"g":"A","cl":0,"dx":0,"dy":0,"ax":639,"ay":0},{"g":"f","cl":1,"dx":0,"dy":0,"ax":344,"ay":0}])");
    const std::string extents = serialize(buffer, f.font, HB_BUFFER_SERIALIZE_FORMAT_TEXT,
                                          static_cast<hb_buffer_serialize_flags_t>(HB_BUFFER_SERIALIZE_FLAG_GLYPH_EXTENTS |
                                                                                   HB_BUFFER_SERIALIZE_FLAG_GLYPH_FLAGS));
    EXPECT_EQ(extents, "[A=0+639<0,717,638,-717>|f=1+344<15,765,372,-765>]");
    EXPECT_EQ(hb_buffer_serialize_format_from_string("json", -1), HB_BUFFER_SERIALIZE_FORMAT_JSON);
    EXPECT_STREQ(hb_buffer_serialize_format_to_string(HB_BUFFER_SERIALIZE_FORMAT_TEXT), "text");
    std::vector<std::string_view> formats;
    for (const char** p = hb_buffer_serialize_list_formats(); *p; ++p) formats.emplace_back(*p);
    EXPECT_EQ(formats, (std::vector<std::string_view>{"text", "json"}));
    // Back from both formats: the same glyphs, positions and clusters.
    for (const std::string& text : {json, serialize(buffer, f.font)}) {
        hb_buffer_t* copy = hb_buffer_create();
        const char* end = nullptr;
        const auto format = text.front() == '[' && text[1] == '{' ? HB_BUFFER_SERIALIZE_FORMAT_JSON : HB_BUFFER_SERIALIZE_FORMAT_TEXT;
        ASSERT_TRUE(hb_buffer_deserialize_glyphs(copy, text.c_str(), static_cast<int>(text.size()), &end, f.font, format));
        EXPECT_EQ(end, text.c_str() + text.size());
        EXPECT_EQ(hb_buffer_diff(copy, buffer, HB_CODEPOINT_INVALID, 0), HB_BUFFER_DIFF_FLAG_EQUAL);
        hb_buffer_destroy(copy);
    }
    // Unicode buffers serialise too.
    hb_buffer_t* text = hb_buffer_create();
    hb_buffer_add_utf8(text, "Af", -1, 0, -1);
    char buf[128];
    unsigned consumed = 0;
    hb_buffer_serialize_unicode(text, 0, 2, buf, sizeof buf, &consumed, HB_BUFFER_SERIALIZE_FORMAT_TEXT,
                                HB_BUFFER_SERIALIZE_FLAG_DEFAULT);
    EXPECT_STREQ(buf, "<U+0041=0|U+0066=1>");
    hb_buffer_t* parsed = hb_buffer_create();
    ASSERT_TRUE(hb_buffer_deserialize_unicode(parsed, buf, -1, nullptr, HB_BUFFER_SERIALIZE_FORMAT_TEXT));
    EXPECT_EQ(hb_buffer_get_length(parsed), 2u);
    // A difference is reported, by kind.
    hb_buffer_get_glyph_infos(parsed, nullptr)[1].codepoint = 'g';
    EXPECT_TRUE(hb_buffer_diff(parsed, text, HB_CODEPOINT_INVALID, 0) & HB_BUFFER_DIFF_FLAG_CODEPOINT_MISMATCH);
    hb_buffer_destroy(parsed);
    hb_buffer_destroy(text);
    hb_buffer_destroy(buffer);
}

TEST(Buffer, GlyphFlagsNormaliseAndMessages) {
    Font f("NotoSansArabic-Regular.ttf");
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, "سلام", -1, 0, -1);
    hb_buffer_guess_segment_properties(buffer);
    hb_buffer_set_flags(buffer, static_cast<hb_buffer_flags_t>(HB_BUFFER_FLAG_PRODUCE_UNSAFE_TO_CONCAT |
                                                               HB_BUFFER_FLAG_PRODUCE_SAFE_TO_INSERT_TATWEEL | HB_BUFFER_FLAG_VERIFY));
    std::vector<std::string> messages;
    hb_buffer_set_message_func(
        buffer,
        [](hb_buffer_t*, hb_font_t*, const char* message, void* data) -> hb_bool_t {
            static_cast<std::vector<std::string>*>(data)->emplace_back(message);
            return true;
        },
        &messages, nullptr);
    ASSERT_TRUE(hb_shape_full(f.font, buffer, nullptr, 0, nullptr));
    EXPECT_FALSE(messages.empty());
    EXPECT_TRUE(std::ranges::any_of(messages, [](const std::string& m) { return m.starts_with("start lookup"); }));
    hb_buffer_set_message_func(buffer, nullptr, nullptr, nullptr);
    unsigned n = 0;
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buffer, &n);
    // Joined letters: unsafe to break inside the word, tatweel may go between.
    ASSERT_EQ(n, 4u);
    EXPECT_EQ(hb_glyph_info_get_glyph_flags(&info[0]), HB_GLYPH_FLAG_UNSAFE_TO_CONCAT);
    EXPECT_EQ(hb_glyph_info_get_glyph_flags(&info[1]), HB_GLYPH_FLAG_UNSAFE_TO_BREAK | HB_GLYPH_FLAG_UNSAFE_TO_CONCAT);
    EXPECT_EQ(hb_glyph_info_get_glyph_flags(&info[2]), HB_GLYPH_FLAG_DEFINED);  // tatweel may go after the medial lam
    EXPECT_EQ(hb_glyph_info_get_glyph_flags(&info[3]), HB_GLYPH_FLAG_UNSAFE_TO_CONCAT);
    EXPECT_TRUE(hb_buffer_has_positions(buffer));
    hb_buffer_normalize_glyphs(buffer);
    hb_buffer_reverse_clusters(buffer);
    hb_buffer_changed(buffer);
    EXPECT_EQ(hb_buffer_get_length(buffer), 4u);
    hb_buffer_destroy(buffer);
}

TEST(Buffer, UserData) {
    static hb_user_data_key_t key;
    static int destroyed = 0;
    hb_buffer_t* buffer = hb_buffer_create();
    int value = 7;
    ASSERT_TRUE(hb_buffer_set_user_data(buffer, &key, &value, [](void*) { ++destroyed; }, true));
    EXPECT_EQ(hb_buffer_get_user_data(buffer, &key), &value);
    hb_buffer_t* again = hb_buffer_reference(buffer);
    hb_buffer_destroy(again);
    EXPECT_EQ(destroyed, 0);
    hb_buffer_destroy(buffer);
    EXPECT_EQ(destroyed, 1);
}
