// mcpplibs.mwrap.harfbuzz.ft — HarfBuzz over FreeType, with FreeType from its
// own header (as mcpplibs.mwrap.freetype would give it: the same entities,
// one FT_Face): faces and fonts over an FT_Face shape as the OpenType
// functions do, FreeType font functions on an hb_font_t, load flags,
// variations reaching FreeType, the locked face and the change notification.
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H
#include <gtest/gtest.h>

import std;
import mcpplibs.mwrap.harfbuzz.ft;  // hb_ft_* alone …
import mcpplibs.mwrap.harfbuzz;     // … and the rest of HarfBuzz, which re-exports it

using namespace mcpplibs::mwrap::harfbuzz::macros;

namespace {

std::string font_path(std::string_view name) {
    const std::filesystem::path here = std::filesystem::path(std::source_location::current().file_name()).parent_path();
    const std::filesystem::path beside = here / "fonts" / name;
    return (std::filesystem::exists(beside) ? beside : std::filesystem::path("tests/fonts") / name).string();
}

std::string shape(hb_font_t* font, std::string_view text) {
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, text.data(), static_cast<int>(text.size()), 0, static_cast<int>(text.size()));
    hb_buffer_guess_segment_properties(buffer);
    hb_shape(font, buffer, nullptr, 0);
    char out[1024];
    hb_buffer_serialize_glyphs(buffer, 0, hb_buffer_get_length(buffer), out, sizeof out, nullptr, font,
                               HB_BUFFER_SERIALIZE_FORMAT_TEXT, HB_BUFFER_SERIALIZE_FLAG_DEFAULT);
    hb_buffer_destroy(buffer);
    return out;
}

struct Library {
    FT_Library ft = nullptr;
    Library() { FT_Init_FreeType(&ft); }
    ~Library() { FT_Done_FreeType(ft); }
    Library(const Library&) = delete;
    Library& operator=(const Library&) = delete;
};

}  // namespace

TEST(Ft, FaceAndFontOverAnFtFace) {
    Library lib;
    FT_Face ft_face = nullptr;
    ASSERT_EQ(FT_New_Face(lib.ft, font_path("NotoSansArabic-Regular.ttf").c_str(), 0, &ft_face), 0);
    ASSERT_EQ(FT_Set_Char_Size(ft_face, 1000 * 64, 1000 * 64, 72, 72), 0);  // 1000 ppem: the font's units
    hb_font_t* font = hb_ft_font_create_referenced(ft_face);
    EXPECT_EQ(hb_ft_font_get_ft_face(font), ft_face);
    // The font's scale is FreeType's 26.6 size: at 1000 ppem, 64 times the
    // advances the OpenType functions give in font units (shape_test.cpp:
    // 484, 379, 220, 784), the same glyphs.
    EXPECT_EQ(shape(font, "سلام"), "[uni0645=6+30976|uni0627.fina.rlig.2=4+24256|uni0644.medi.rlig=2+14080|uni0633.init=0+50176]");
    FT_Face locked = hb_ft_font_lock_face(font);
    EXPECT_EQ(locked, ft_face);
    hb_ft_font_unlock_face(font);
    hb_ft_font_set_load_flags(font, FT_LOAD_NO_HINTING);
    EXPECT_EQ(hb_ft_font_get_load_flags(font), FT_LOAD_NO_HINTING);
    hb_face_t* face = hb_ft_face_create_referenced(ft_face);
    EXPECT_EQ(hb_face_get_upem(face), 1000u);
    EXPECT_EQ(hb_face_get_glyph_count(face), 161u);
    hb_face_t* cached = hb_ft_face_create_cached(ft_face);
    EXPECT_EQ(hb_face_get_upem(cached), 1000u);
    hb_face_destroy(cached);
    FT_Reference_Face(ft_face);
    hb_face_t* owned = hb_ft_face_create(ft_face, [](void* f) { FT_Done_Face(static_cast<FT_Face>(f)); });
    EXPECT_EQ(hb_face_get_glyph_count(owned), 161u);
    hb_face_destroy(owned);
    FT_Reference_Face(ft_face);
    hb_font_t* plain = hb_ft_font_create(ft_face, [](void* f) { FT_Done_Face(static_cast<FT_Face>(f)); });
    EXPECT_EQ(hb_ft_font_get_ft_face(plain), ft_face);
    hb_font_destroy(plain);
    hb_face_destroy(face);
    hb_font_destroy(font);
    FT_Done_Face(ft_face);
}

TEST(Ft, FacesFromFilesAndBlobs) {
    hb_face_t* face = hb_ft_face_create_from_file_or_fail(font_path("NotoSansDevanagari-Regular.ttf").c_str(), 0);
    ASSERT_NE(face, nullptr);
    EXPECT_EQ(hb_face_get_glyph_count(face), 596u);
    hb_blob_t* blob = hb_blob_create_from_file_or_fail(font_path("chromacheck-colr.ttf").c_str());
    hb_face_t* from_blob = hb_ft_face_create_from_blob_or_fail(blob, 0);
    ASSERT_NE(from_blob, nullptr);
    EXPECT_EQ(hb_face_get_upem(from_blob), 1024u);
    EXPECT_EQ(hb_ft_face_create_from_file_or_fail("no-such-font.ttf", 0), nullptr);
    hb_face_destroy(from_blob);
    hb_blob_destroy(blob);
    hb_face_destroy(face);
}

TEST(Ft, FreeTypeFunctionsAndVariations) {
    hb_face_t* face = hb_face_create_from_file_or_fail(font_path("NotoSans-Latin.ttf").c_str(), 0);
    hb_font_t* font = hb_font_create(face);
    hb_ft_font_set_funcs(font);  // FreeType loads the glyphs from the hb_face_t's blob
    FT_Face ft_face = hb_ft_font_get_ft_face(font);
    ASSERT_NE(ft_face, nullptr);
    EXPECT_TRUE(FT_HAS_MULTIPLE_MASTERS(ft_face));
    EXPECT_EQ(shape(font, "office AVA"), "[o=0+605|f_f_i=1+946|c=4+480|e=5+564|space=6+260|A=7+599|V=8+560|A=9+639]");
    const hb_position_t regular = hb_font_get_glyph_h_advance(font, 80);
    hb_variation_t bold{HB_OT_TAG_VAR_AXIS_WEIGHT, 900.f};
    hb_font_set_variations(font, &bold, 1);  // reaches FreeType's blend coordinates
    EXPECT_GT(hb_font_get_glyph_h_advance(font, 80), regular);
    FT_Fixed coords[2] = {};
    ASSERT_EQ(FT_Get_Var_Blend_Coordinates(hb_ft_font_get_ft_face(font), 2, coords), 0);
    EXPECT_EQ(coords[0], 1 << 16);  // 16.16 1.0: the wght maximum
    // A change made through FreeType is announced to HarfBuzz.
    FT_Face locked = hb_ft_font_lock_face(font);
    FT_Set_Char_Size(locked, 2000 * 64, 2000 * 64, 72, 72);
    hb_ft_font_changed(font);
    hb_ft_font_unlock_face(font);
    int x = 0, y = 0;
    hb_font_get_scale(font, &x, &y);
    EXPECT_EQ(x, 2000 * 64);
    EXPECT_FALSE(hb_ft_hb_font_changed(font));  // nothing changed on HarfBuzz's side since
    hb_font_destroy(font);
    hb_face_destroy(face);
}
