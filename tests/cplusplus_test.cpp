// The C++ helpers of <hb-cplusplus.hh> through the module: hb::shared_ptr
// and hb::unique_ptr over each object's reference/destroy pair, their traits,
// user data through the shared pointer, and std::hash for both.
#include <gtest/gtest.h>

import std;
import mcpplibs.mwrap.harfbuzz;

TEST(Cplusplus, SharedAndUniquePointers) {
    static hb_user_data_key_t key;
    static int destroyed = 0;
    {
        hb::shared_ptr<hb_buffer_t> a{hb_buffer_create()};
        ASSERT_TRUE(a);
        int value = 1;
        a.set_user_data(&key, &value, [](void*) { ++destroyed; }, true);
        EXPECT_EQ(a.get_user_data(&key), &value);
        hb::shared_ptr<hb_buffer_t> b = a;  // hb_buffer_reference
        EXPECT_EQ(a, b);
        EXPECT_EQ(a.get(), b.get());
        hb_buffer_add_utf8(b, "ab", -1, 0, -1);  // converts to hb_buffer_t*
        EXPECT_EQ(hb_buffer_get_length(a), 2u);
        b = hb::shared_ptr<hb_buffer_t>{};
        EXPECT_EQ(destroyed, 0);
        EXPECT_EQ(hb::shared_ptr<hb_buffer_t>::get_empty(), hb_buffer_get_empty());
    }
    EXPECT_EQ(destroyed, 1);
    hb::unique_ptr<hb_set_t> set{hb_set_create()};
    hb_set_add(set, 7);
    hb::unique_ptr<hb_set_t> moved = std::move(set);
    EXPECT_FALSE(set);
    EXPECT_TRUE(hb_set_has(moved.get(), 7));
    hb_set_t* raw = moved.release();
    EXPECT_FALSE(moved);
    hb_set_destroy(raw);
    static_assert(hb::is_shared_ptr<hb::shared_ptr<hb_font_t>>::value);
    static_assert(!hb::is_shared_ptr<hb::unique_ptr<hb_font_t>>::value);
    static_assert(hb::is_unique_ptr<hb::unique_ptr<hb_face_t>>::value);
    static_assert(std::is_same_v<hb::shared_ptr<hb_blob_t>::element_type, hb_blob_t>);
    static_assert(hb::vtable<hb_font_t>::destroy == &hb_font_destroy);
    static_assert(hb::vtable<hb_paint_funcs_t>::get_empty == &hb_paint_funcs_get_empty);
    static_assert(std::is_base_of_v<hb::vtable_t<hb_map_t, &hb_map_get_empty, &hb_map_reference, &hb_map_destroy,
                                                 &hb_map_set_user_data, &hb_map_get_user_data>,
                                    hb::vtable<hb_map_t>>);
}

TEST(Cplusplus, Hash) {
    hb::shared_ptr<hb_face_t> face{hb_face_create(hb_blob_get_empty(), 0)};
    std::unordered_set<hb::shared_ptr<hb_face_t>> faces;
    faces.insert(face);
    faces.insert(face);
    EXPECT_EQ(faces.size(), 1u);
    EXPECT_EQ(std::hash<hb::shared_ptr<hb_face_t>>{}(face), std::hash<hb_face_t*>{}(face.get()));
    hb::unique_ptr<hb_map_t> map{hb_map_create()};
    EXPECT_EQ(std::hash<hb::unique_ptr<hb_map_t>>{}(map), std::hash<hb_map_t*>{}(map.get()));
}
