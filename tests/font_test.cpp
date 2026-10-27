// Blobs, faces and fonts through the module, checked against what fontTools
// reads from the same files: tables, cmap, metrics, names, variations and
// named instances; custom font functions; glyph outlines through draw
// functions; COLR/CPAL colour glyphs through paint functions and hb-ot-color;
// OpenType layout, metrics, name, var, math and meta queries; AAT; style.
#include <gtest/gtest.h>

import std;
import mcpplibs.mwrap.harfbuzz;

using namespace mcpplibs::mwrap::harfbuzz::macros;  // HB_TAG, HB_OT_TAG_GSUB, HB_LANGUAGE_INVALID …

namespace {

std::string font_path(std::string_view name) {
    const std::filesystem::path here = std::filesystem::path(std::source_location::current().file_name()).parent_path();
    const std::filesystem::path beside = here / "fonts" / name;
    return (std::filesystem::exists(beside) ? beside : std::filesystem::path("tests/fonts") / name).string();
}

std::string read_file(std::string_view name) {
    std::ifstream in(font_path(name), std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}

struct Font {
    hb_face_t* face;
    hb_font_t* font;
    explicit Font(std::string_view name)
        : face(hb_face_create_from_file_or_fail(font_path(name).c_str(), 0)), font(hb_font_create(face)) {}
    ~Font() {
        hb_font_destroy(font);
        hb_face_destroy(face);
    }
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
};

std::string tag_string(hb_tag_t tag) {
    char s[5] = {};
    hb_tag_to_string(tag, s);
    return s;
}

std::string name_of(hb_face_t* face, hb_ot_name_id_t id) {
    char buf[128];
    unsigned len = sizeof buf;
    hb_ot_name_get_utf8(face, id, HB_LANGUAGE_INVALID, &len, buf);
    return std::string(buf, len);
}

}  // namespace

TEST(Blob, CreateSubBlobAndDestroy) {
    static int destroyed = 0;
    std::string data = read_file("NotoSans-Latin.ttf");
    hb_blob_t* blob = hb_blob_create(data.data(), static_cast<unsigned>(data.size()), HB_MEMORY_MODE_READONLY, &data,
                                     [](void*) { ++destroyed; });
    EXPECT_EQ(hb_blob_get_length(blob), data.size());
    unsigned len = 0;
    EXPECT_EQ(hb_blob_get_data(blob, &len), data.data());
    hb_blob_t* sub = hb_blob_create_sub_blob(blob, 4, 8);
    EXPECT_EQ(hb_blob_get_length(sub), 8u);
    EXPECT_EQ(hb_blob_get_data(sub, nullptr), data.data() + 4);
    hb_blob_t* dup = hb_blob_create(data.data(), 16, HB_MEMORY_MODE_DUPLICATE, nullptr, nullptr);
    EXPECT_NE(hb_blob_get_data(dup, nullptr), data.data());  // its own copy
    EXPECT_FALSE(hb_blob_is_immutable(dup));
    hb_blob_make_immutable(dup);
    EXPECT_TRUE(hb_blob_is_immutable(dup));
    hb_blob_destroy(dup);
    // A writable view of a read-only blob is a copy.
    hb_blob_t* copy = hb_blob_copy_writable_or_fail(sub);
    ASSERT_NE(copy, nullptr);
    EXPECT_NE(hb_blob_get_data_writable(copy, nullptr), data.data() + 4);
    hb_blob_destroy(copy);
    hb_blob_destroy(sub);
    EXPECT_EQ(destroyed, 0);
    hb_blob_destroy(blob);
    EXPECT_EQ(destroyed, 1);
    EXPECT_EQ(hb_blob_get_length(hb_blob_get_empty()), 0u);
    EXPECT_EQ(hb_blob_create_from_file_or_fail("no-such-file.ttf"), nullptr);
    hb_blob_t* file = hb_blob_create_from_file(font_path("chromacheck-colr.ttf").c_str());
    EXPECT_EQ(hb_blob_get_length(file), 652u);
    hb_blob_destroy(file);
}

TEST(Face, TablesUnicodesAndBuilder) {
    Font f("NotoSans-Latin.ttf");
    EXPECT_EQ(hb_face_get_glyph_count(f.face), 380u);
    EXPECT_EQ(hb_face_get_upem(f.face), 1000u);
    EXPECT_EQ(hb_face_get_index(f.face), 0u);
    hb_tag_t tags[32];
    unsigned count = 32;
    const unsigned total = hb_face_get_table_tags(f.face, 0, &count, tags);
    EXPECT_EQ(total, 21u);  // fontTools: 21 tables (and its GlyphOrder pseudo-table)
    EXPECT_TRUE(std::ranges::contains(std::span(tags, count), HB_TAG('g', 'v', 'a', 'r')));
    hb_blob_t* head = hb_face_reference_table(f.face, HB_TAG('h', 'e', 'a', 'd'));
    EXPECT_EQ(hb_blob_get_length(head), 54u);
    hb_set_t* unicodes = hb_set_create();
    hb_face_collect_unicodes(f.face, unicodes);
    EXPECT_EQ(hb_set_get_population(unicodes), 192u);
    EXPECT_TRUE(hb_set_has(unicodes, 0x2026));
    hb_map_t* mapping = hb_map_create();
    hb_set_t* mapped = hb_set_create();
    hb_face_collect_nominal_glyph_mapping(f.face, mapping, mapped);
    EXPECT_EQ(hb_map_get(mapping, 'A'), 34u);
    // A face built from tables: the builder's blob is a font again.
    hb_face_t* builder = hb_face_builder_create();
    ASSERT_TRUE(hb_face_builder_add_table(builder, HB_TAG('h', 'e', 'a', 'd'), head));
    hb_blob_t* built = hb_face_reference_blob(builder);
    hb_face_t* rebuilt = hb_face_create(built, 0);
    hb_blob_t* rebuilt_head = hb_face_reference_table(rebuilt, HB_TAG('h', 'e', 'a', 'd'));
    EXPECT_EQ(hb_blob_get_length(rebuilt_head), 54u);
    EXPECT_EQ(hb_face_get_upem(rebuilt), 1000u);
    // A face over a table callback.
    struct Tables {
        hb_face_t* source;
        int calls = 0;
    } tables{f.face};
    hb_face_t* lazy = hb_face_create_for_tables(
        [](hb_face_t*, hb_tag_t tag, void* data) {
            auto* t = static_cast<Tables*>(data);
            ++t->calls;
            return hb_face_reference_table(t->source, tag);
        },
        &tables, nullptr);
    EXPECT_EQ(hb_face_get_upem(lazy), 1000u);
    EXPECT_GT(tables.calls, 0);
    std::vector<std::string_view> loaders;
    for (const char** l = hb_face_list_loaders(); *l; ++l) loaders.emplace_back(*l);
    EXPECT_TRUE(std::ranges::contains(loaders, "ot"));
    EXPECT_TRUE(std::ranges::contains(loaders, "ft"));
    hb_face_destroy(lazy);
    hb_blob_destroy(rebuilt_head);
    hb_face_destroy(rebuilt);
    hb_blob_destroy(built);
    hb_face_destroy(builder);
    hb_set_destroy(mapped);
    hb_map_destroy(mapping);
    hb_set_destroy(unicodes);
    hb_blob_destroy(head);
    EXPECT_EQ(hb_face_get_glyph_count(hb_face_get_empty()), 0u);
}

TEST(Font, GlyphsMetricsAndExtents) {
    Font f("NotoSans-Latin.ttf");
    hb_codepoint_t glyph = 0;
    ASSERT_TRUE(hb_font_get_nominal_glyph(f.font, 'A', &glyph));
    EXPECT_EQ(glyph, 34u);
    EXPECT_FALSE(hb_font_get_nominal_glyph(f.font, 0x0627, &glyph));
    char name[64];
    ASSERT_TRUE(hb_font_get_glyph_name(f.font, 34, name, sizeof name));
    EXPECT_STREQ(name, "A");
    ASSERT_TRUE(hb_font_get_glyph_from_name(f.font, "o", -1, &glyph));
    EXPECT_EQ(glyph, 80u);
    hb_font_glyph_to_string(f.font, 71, name, sizeof name);
    EXPECT_STREQ(name, "f");
    EXPECT_EQ(hb_font_get_glyph_h_advance(f.font, 34), 639);  // hmtx
    EXPECT_EQ(hb_font_get_glyph_h_advance(f.font, 0), 600);
    hb_glyph_extents_t extents;
    ASSERT_TRUE(hb_font_get_glyph_extents(f.font, 34, &extents));
    EXPECT_EQ(extents.x_bearing, 0);
    EXPECT_EQ(extents.y_bearing, 717);
    EXPECT_EQ(extents.width, 638);
    EXPECT_EQ(extents.height, -717);
    hb_font_extents_t h;
    ASSERT_TRUE(hb_font_get_h_extents(f.font, &h));
    EXPECT_EQ(h.ascender, 1069);  // OS/2 typo metrics (USE_TYPO_METRICS)
    EXPECT_EQ(h.descender, -293);
    EXPECT_EQ(h.line_gap, 0);
    // Scale: positions follow it.
    hb_font_set_scale(f.font, 2000, 2000);
    int x = 0, y = 0;
    hb_font_get_scale(f.font, &x, &y);
    EXPECT_EQ(x, 2000);
    EXPECT_EQ(hb_font_get_glyph_h_advance(f.font, 34), 1278);
    hb_font_set_ppem(f.font, 16, 16);
    unsigned px = 0, py = 0;
    hb_font_get_ppem(f.font, &px, &py);
    EXPECT_EQ(px, 16u);
    hb_font_set_ptem(f.font, 12.f);
    EXPECT_EQ(hb_font_get_ptem(f.font), 12.f);
    hb_font_set_synthetic_slant(f.font, 0.2f);
    EXPECT_EQ(hb_font_get_synthetic_slant(f.font), 0.2f);
    hb_font_set_synthetic_bold(f.font, 0.02f, 0.02f, true);
    float bx = 0, by = 0;
    hb_bool_t in_place = false;
    hb_font_get_synthetic_bold(f.font, &bx, &by, &in_place);
    EXPECT_TRUE(in_place);
    EXPECT_GT(hb_font_get_serial(f.font), 0u);
    std::vector<std::string_view> funcs;
    for (const char** p = hb_font_list_funcs(); *p; ++p) funcs.emplace_back(*p);
    EXPECT_EQ(funcs, (std::vector<std::string_view>{"ot", "ft"}));
}

TEST(Font, Variations) {
    Font f("NotoSans-Latin.ttf");
    ASSERT_TRUE(hb_ot_var_has_data(f.face));
    EXPECT_EQ(hb_ot_var_get_axis_count(f.face), 2u);
    hb_ot_var_axis_info_t axes[2];
    unsigned n = 2;
    hb_ot_var_get_axis_infos(f.face, 0, &n, axes);
    ASSERT_EQ(n, 2u);
    EXPECT_EQ(axes[0].tag, HB_OT_TAG_VAR_AXIS_WEIGHT);
    EXPECT_EQ(axes[0].min_value, 100.f);
    EXPECT_EQ(axes[0].default_value, 400.f);
    EXPECT_EQ(axes[0].max_value, 900.f);
    EXPECT_EQ(axes[1].tag, HB_OT_TAG_VAR_AXIS_WIDTH);
    EXPECT_EQ(name_of(f.face, axes[0].name_id), "Weight");
    hb_ot_var_axis_info_t wdth;
    ASSERT_TRUE(hb_ot_var_find_axis_info(f.face, HB_OT_TAG_VAR_AXIS_WIDTH, &wdth));
    EXPECT_EQ(wdth.min_value, 62.5f);
    EXPECT_EQ(hb_ot_var_get_named_instance_count(f.face), 9u);
    EXPECT_EQ(name_of(f.face, hb_ot_var_named_instance_get_subfamily_name_id(f.face, 0)), "Thin");
    // Bolder is wider: the advance of 'o' grows with wght (gvar/HVAR).
    const hb_position_t regular = hb_font_get_glyph_h_advance(f.font, 80);
    hb_variation_t bold{HB_OT_TAG_VAR_AXIS_WEIGHT, 900.f};
    hb_font_set_variations(f.font, &bold, 1);
    EXPECT_GT(hb_font_get_glyph_h_advance(f.font, 80), regular);
    unsigned coords_length = 0;
    const int* coords = hb_font_get_var_coords_normalized(f.font, &coords_length);
    ASSERT_EQ(coords_length, 2u);
    EXPECT_EQ(coords[0], 1 << 14);  // F2DOT14 1.0: the axis maximum
    hb_font_set_var_named_instance(f.font, 0);
    EXPECT_EQ(hb_font_get_var_named_instance(f.font), 0u);
    EXPECT_LT(hb_font_get_glyph_h_advance(f.font, 80), regular);
    hb_variation_t parsed;
    ASSERT_TRUE(hb_variation_from_string("wght=650", -1, &parsed));
    EXPECT_EQ(parsed.value, 650.f);
    char text[32];
    hb_variation_to_string(&parsed, text, sizeof text);
    EXPECT_STREQ(text, "wght=650");
    hb_font_set_variation(f.font, HB_OT_TAG_VAR_AXIS_WIDTH, 80.f);
    EXPECT_EQ(hb_style_get_value(f.font, HB_STYLE_TAG_WIDTH), 80.f);
}

TEST(Font, SubFontAndCustomFunctions) {
    Font f("NotoSans-Latin.ttf");
    // A sub-font inherits everything; one function is replaced.
    hb_font_t* sub = hb_font_create_sub_font(f.font);
    EXPECT_EQ(hb_font_get_parent(sub), f.font);
    hb_font_funcs_t* funcs = hb_font_funcs_create();
    hb_font_funcs_set_glyph_h_advance_func(
        funcs,
        [](hb_font_t* font, void*, hb_codepoint_t glyph, void*) -> hb_position_t {
            return 2 * hb_font_get_glyph_h_advance(hb_font_get_parent(font), glyph);
        },
        nullptr, nullptr);
    hb_font_funcs_make_immutable(funcs);
    EXPECT_TRUE(hb_font_funcs_is_immutable(funcs));
    hb_font_set_funcs(sub, funcs, nullptr, nullptr);
    EXPECT_EQ(hb_font_get_glyph_h_advance(sub, 34), 1278);
    hb_codepoint_t glyph = 0;
    ASSERT_TRUE(hb_font_get_nominal_glyph(sub, 'A', &glyph));  // from the parent
    EXPECT_EQ(glyph, 34u);
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, "AA", -1, 0, -1);
    hb_buffer_guess_segment_properties(buffer);
    hb_shape(sub, buffer, nullptr, 0);
    unsigned n = 0;
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buffer, &n);
    ASSERT_EQ(n, 2u);
    EXPECT_EQ(pos[1].x_advance, 1278);
    hb_buffer_destroy(buffer);
    hb_font_funcs_destroy(funcs);
    hb_font_destroy(sub);
    // The built-in implementations by name.
    hb_font_t* font = hb_font_create(f.face);
    ASSERT_TRUE(hb_font_set_funcs_using(font, "ft"));
    EXPECT_EQ(hb_font_get_glyph_h_advance(font, 34), 639);
    EXPECT_FALSE(hb_font_set_funcs_using(font, "no-such-funcs"));
    hb_ot_font_set_funcs(font);
    EXPECT_EQ(hb_font_get_glyph_h_advance(font, 34), 639);
    hb_font_destroy(font);
}

TEST(Draw, GlyphOutline) {
    Font f("NotoSans-Latin.ttf");
    struct Path {
        int moves = 0, lines = 0, quads = 0, cubics = 0, closes = 0;
        float min_x = 1e9f, max_x = -1e9f;
        void at(float x) {
            min_x = std::min(min_x, x);
            max_x = std::max(max_x, x);
        }
    } path;
    hb_draw_funcs_t* d = hb_draw_funcs_create();
    hb_draw_funcs_set_move_to_func(d, [](hb_draw_funcs_t*, void* p, hb_draw_state_t*, float x, float, void*) {
        ++static_cast<Path*>(p)->moves;
        static_cast<Path*>(p)->at(x);
    }, nullptr, nullptr);
    hb_draw_funcs_set_line_to_func(d, [](hb_draw_funcs_t*, void* p, hb_draw_state_t*, float x, float, void*) {
        ++static_cast<Path*>(p)->lines;
        static_cast<Path*>(p)->at(x);
    }, nullptr, nullptr);
    hb_draw_funcs_set_quadratic_to_func(d, [](hb_draw_funcs_t*, void* p, hb_draw_state_t*, float, float, float x, float, void*) {
        ++static_cast<Path*>(p)->quads;
        static_cast<Path*>(p)->at(x);
    }, nullptr, nullptr);
    hb_draw_funcs_set_cubic_to_func(d, [](hb_draw_funcs_t*, void* p, hb_draw_state_t*, float, float, float, float, float, float, void*) {
        ++static_cast<Path*>(p)->cubics;
    }, nullptr, nullptr);
    hb_draw_funcs_set_close_path_func(d, [](hb_draw_funcs_t*, void* p, hb_draw_state_t*, void*) { ++static_cast<Path*>(p)->closes; },
                                      nullptr, nullptr);
    hb_draw_funcs_make_immutable(d);
    // 'o': two closed TrueType contours of quadratic curves.
    hb_font_draw_glyph(f.font, 80, d, &path);
    EXPECT_EQ(path.moves, 2);
    EXPECT_EQ(path.closes, 2);
    EXPECT_GT(path.quads, 4);
    EXPECT_EQ(path.cubics, 0);
    EXPECT_EQ(path.min_x, 55.f);  // the left side bearing in hmtx
    EXPECT_TRUE(hb_font_draw_glyph_or_fail(f.font, 80, d, &path));
    // The draw API itself, driven directly: a quadratic is passed through.
    Path direct;
    hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;
    hb_draw_move_to(d, &direct, &st, 0, 0);
    hb_draw_quadratic_to(d, &direct, &st, 5, 5, 10, 0);
    hb_draw_line_to(d, &direct, &st, 0, 0);
    hb_draw_close_path(d, &direct, &st);
    EXPECT_EQ(direct.moves, 1);
    EXPECT_EQ(direct.quads, 1);
    EXPECT_EQ(direct.closes, 1);
    hb_draw_funcs_destroy(d);
}

TEST(Color, LayersPalettesAndPaint) {
    Font f("chromacheck-colr.ttf");
    EXPECT_TRUE(hb_ot_color_has_layers(f.face));
    EXPECT_TRUE(hb_ot_color_has_palettes(f.face));
    EXPECT_FALSE(hb_ot_color_has_paint(f.face));
    EXPECT_FALSE(hb_ot_color_has_png(f.face));
    EXPECT_FALSE(hb_ot_color_has_svg(f.face));
    EXPECT_EQ(hb_ot_color_palette_get_count(f.face), 1u);
    EXPECT_EQ(hb_ot_color_palette_get_flags(f.face, 0), HB_OT_COLOR_PALETTE_FLAG_DEFAULT);
    hb_color_t colors[4];
    unsigned n = 4;
    EXPECT_EQ(hb_ot_color_palette_get_colors(f.face, 0, 0, &n, colors), 1u);
    ASSERT_EQ(n, 1u);
    // CPAL #C80000FF: red 0xC8, opaque — read by the functions, and by the macros' arithmetic.
    EXPECT_EQ(hb_color_get_red(colors[0]), 0xC8);
    EXPECT_EQ(hb_color_get_green(colors[0]), 0);
    EXPECT_EQ(hb_color_get_blue(colors[0]), 0);
    EXPECT_EQ(hb_color_get_alpha(colors[0]), 0xFF);
    EXPECT_EQ(colors[0], HB_COLOR(0, 0, 0xC8, 0xFF));
    hb_codepoint_t glyph = 0;
    ASSERT_TRUE(hb_font_get_nominal_glyph(f.font, 0xE900, &glyph));
    hb_ot_color_layer_t layers[4];
    n = 4;
    EXPECT_EQ(hb_ot_color_glyph_get_layers(f.face, glyph, 0, &n, layers), 1u);
    EXPECT_EQ(layers[0].glyph, glyph);
    EXPECT_EQ(layers[0].color_index, 0u);
    EXPECT_FALSE(hb_ot_color_glyph_has_paint(f.face, glyph));
    // Painting the glyph: a clip to the layer's outline, filled with the colour.
    struct Paint {
        std::vector<std::string> calls;
        hb_color_t color = 0;
    } paint;
    hb_paint_funcs_t* p = hb_paint_funcs_create();
    hb_paint_funcs_set_push_clip_glyph_func(p, [](hb_paint_funcs_t*, void* d, hb_codepoint_t, hb_font_t*, void*) {
        static_cast<Paint*>(d)->calls.emplace_back("clip");
    }, nullptr, nullptr);
    hb_paint_funcs_set_pop_clip_func(p, [](hb_paint_funcs_t*, void* d, void*) { static_cast<Paint*>(d)->calls.emplace_back("pop"); },
                                     nullptr, nullptr);
    hb_paint_funcs_set_color_func(p, [](hb_paint_funcs_t*, void* d, hb_bool_t, hb_color_t c, void*) {
        static_cast<Paint*>(d)->calls.emplace_back("color");
        static_cast<Paint*>(d)->color = c;
    }, nullptr, nullptr);
    hb_paint_funcs_make_immutable(p);
    hb_font_paint_glyph(f.font, glyph, p, &paint, 0, HB_COLOR(0, 0, 0, 0xFF));
    EXPECT_EQ(paint.calls, (std::vector<std::string>{"clip", "color", "pop"}));
    EXPECT_EQ(paint.color, colors[0]);
    hb_paint_funcs_destroy(p);
}

TEST(OpenType, LayoutQueries) {
    Font f("NotoSansArabic-Regular.ttf");
    EXPECT_TRUE(hb_ot_layout_has_substitution(f.face));
    EXPECT_TRUE(hb_ot_layout_has_positioning(f.face));
    EXPECT_TRUE(hb_ot_layout_has_glyph_classes(f.face));
    hb_tag_t scripts[8];
    unsigned n = 8;
    EXPECT_EQ(hb_ot_layout_table_get_script_tags(f.face, HB_OT_TAG_GSUB, 0, &n, scripts), 6u);
    std::vector<std::string> names;
    for (unsigned i = 0; i < n; i++) names.push_back(tag_string(scripts[i]));
    EXPECT_EQ(names, (std::vector<std::string>{"DFLT", "arab", "cyrl", "dev2", "grek", "latn"}));
    unsigned script_index = 0;
    ASSERT_TRUE(hb_ot_layout_table_find_script(f.face, HB_OT_TAG_GSUB, HB_TAG('a', 'r', 'a', 'b'), &script_index));
    hb_tag_t features[32];
    n = 32;
    hb_ot_layout_language_get_feature_tags(f.face, HB_OT_TAG_GSUB, script_index, HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX, 0, &n, features);
    std::set<std::string> feature_names;
    for (unsigned i = 0; i < n; i++) feature_names.insert(tag_string(features[i]));
    for (const char* tag : {"init", "medi", "fina", "rlig", "ccmp"}) EXPECT_TRUE(feature_names.contains(tag)) << tag;
    unsigned feature_index = 0;
    ASSERT_TRUE(hb_ot_layout_language_find_feature(f.face, HB_OT_TAG_GSUB, script_index, HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX,
                                                   HB_TAG('i', 'n', 'i', 't'), &feature_index));
    EXPECT_NE(feature_index, HB_OT_LAYOUT_NO_FEATURE_INDEX);
    unsigned lookup_count = hb_ot_layout_feature_get_lookups(f.face, HB_OT_TAG_GSUB, feature_index, 0, nullptr, nullptr);
    EXPECT_GT(lookup_count, 0u);
    EXPECT_EQ(hb_ot_layout_table_get_lookup_count(f.face, HB_OT_TAG_GSUB), 23u);
    EXPECT_EQ(hb_ot_layout_table_get_lookup_count(f.face, HB_OT_TAG_GPOS), 6u);
    // GDEF classes: fontTools counts 76 base glyphs, 25 marks, 1 ligature.
    hb_set_t* marks = hb_set_create();
    hb_ot_layout_get_glyphs_in_class(f.face, HB_OT_LAYOUT_GLYPH_CLASS_MARK, marks);
    EXPECT_EQ(hb_set_get_population(marks), 25u);
    EXPECT_EQ(hb_ot_layout_get_glyph_class(f.face, hb_set_get_min(marks)), HB_OT_LAYOUT_GLYPH_CLASS_MARK);
    // The closure of 'init' over the Arabic letters reaches their initial forms.
    hb_set_t* lookups = hb_set_create();
    hb_ot_layout_collect_lookups(f.face, HB_OT_TAG_GSUB, nullptr, nullptr, features, lookups);
    hb_set_t* glyphs = hb_set_create();
    hb_codepoint_t seen = 0;
    hb_font_get_nominal_glyph(f.font, 0x0633, &seen);
    hb_set_add(glyphs, seen);
    hb_ot_layout_lookups_substitute_closure(f.face, lookups, glyphs);
    hb_codepoint_t init = 0;
    ASSERT_TRUE(hb_font_get_glyph_from_name(f.font, "uni0633.init", -1, &init));
    EXPECT_TRUE(hb_set_has(glyphs, init));
    // Script and language tags.
    hb_tag_t script_tags[HB_OT_MAX_TAGS_PER_SCRIPT], language_tags[HB_OT_MAX_TAGS_PER_LANGUAGE];
    unsigned sc = HB_OT_MAX_TAGS_PER_SCRIPT, lc = HB_OT_MAX_TAGS_PER_LANGUAGE;
    hb_ot_tags_from_script_and_language(HB_SCRIPT_ARABIC, hb_language_from_string("fa", -1), &sc, script_tags, &lc, language_tags);
    ASSERT_GE(sc, 1u);
    ASSERT_GE(lc, 1u);
    EXPECT_EQ(tag_string(script_tags[0]), "arab");
    EXPECT_EQ(tag_string(language_tags[0]), "FAR ");
    EXPECT_STREQ(hb_language_to_string(hb_ot_tag_to_language(HB_TAG('F', 'A', 'R', ' '))), "fa");
    EXPECT_EQ(hb_ot_tag_to_script(HB_TAG('d', 'e', 'v', '2')), HB_SCRIPT_DEVANAGARI);
    hb_position_t baseline = 0;
    hb_ot_layout_get_baseline_with_fallback2(f.font, HB_OT_LAYOUT_BASELINE_TAG_ROMAN, HB_DIRECTION_RTL, HB_SCRIPT_ARABIC,
                                             HB_LANGUAGE_INVALID, &baseline);
    EXPECT_EQ(baseline, 0);
    hb_set_destroy(glyphs);
    hb_set_destroy(lookups);
    hb_set_destroy(marks);
}

TEST(OpenType, MetricsNamesMathMeta) {
    Font f("NotoSans-Latin.ttf");
    hb_position_t v = 0;
    ASSERT_TRUE(hb_ot_metrics_get_position(f.font, HB_OT_METRICS_TAG_HORIZONTAL_ASCENDER, &v));
    EXPECT_EQ(v, 1069);
    ASSERT_TRUE(hb_ot_metrics_get_position(f.font, HB_OT_METRICS_TAG_HORIZONTAL_DESCENDER, &v));
    EXPECT_EQ(v, -293);
    ASSERT_TRUE(hb_ot_metrics_get_position(f.font, HB_OT_METRICS_TAG_X_HEIGHT, &v));
    EXPECT_EQ(v, 536);
    ASSERT_TRUE(hb_ot_metrics_get_position(f.font, HB_OT_METRICS_TAG_CAP_HEIGHT, &v));
    EXPECT_EQ(v, 714);
    ASSERT_TRUE(hb_ot_metrics_get_position(f.font, HB_OT_METRICS_TAG_UNDERLINE_OFFSET, &v));
    EXPECT_EQ(v, -100);
    ASSERT_TRUE(hb_ot_metrics_get_position(f.font, HB_OT_METRICS_TAG_UNDERLINE_SIZE, &v));
    EXPECT_EQ(v, 50);
    hb_ot_metrics_get_position_with_fallback(f.font, HB_OT_METRICS_TAG_STRIKEOUT_SIZE, &v);
    EXPECT_GT(v, 0);
    EXPECT_EQ(hb_ot_metrics_get_variation(f.font, HB_OT_METRICS_TAG_X_HEIGHT), 0.f);
    EXPECT_EQ(name_of(f.face, HB_OT_NAME_ID_FONT_FAMILY), "Noto Sans");
    EXPECT_EQ(name_of(f.face, HB_OT_NAME_ID_FONT_SUBFAMILY), "Regular");
    EXPECT_EQ(name_of(f.face, HB_OT_NAME_ID_POSTSCRIPT_NAME), "NotoSans-Regular");
    EXPECT_EQ(name_of(f.face, HB_OT_NAME_ID_TYPOGRAPHIC_FAMILY), "");  // none: openmui falls back to the family
    unsigned entries = 0;
    const hb_ot_name_entry_t* list = hb_ot_name_list_names(f.face, &entries);
    EXPECT_EQ(entries, 43u);
    EXPECT_STREQ(hb_language_to_string(list[0].language), "en");
    std::uint32_t utf32[32];
    unsigned len = 32;
    hb_ot_name_get_utf32(f.face, HB_OT_NAME_ID_FONT_FAMILY, hb_language_from_string("en", -1), &len, utf32);
    EXPECT_EQ(len, 9u);
    EXPECT_EQ(utf32[0], static_cast<std::uint32_t>('N'));
    EXPECT_FALSE(hb_ot_math_has_data(f.face));
    EXPECT_EQ(hb_ot_math_get_constant(f.font, HB_OT_MATH_CONSTANT_AXIS_HEIGHT), 0);
    EXPECT_EQ(hb_ot_meta_get_entry_tags(f.face, 0, nullptr, nullptr), 0u);
    EXPECT_FALSE(hb_aat_layout_has_substitution(f.face));
    EXPECT_FALSE(hb_aat_layout_has_positioning(f.face));
    EXPECT_FALSE(hb_aat_layout_has_tracking(f.face));
    EXPECT_EQ(hb_aat_layout_get_feature_types(f.face, 0, nullptr, nullptr), 0u);
    EXPECT_EQ(hb_style_get_value(f.font, HB_STYLE_TAG_WEIGHT), 400.f);
    EXPECT_EQ(hb_style_get_value(f.font, HB_STYLE_TAG_ITALIC), 0.f);
}
