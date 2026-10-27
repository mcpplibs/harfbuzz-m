// hb-shape — HarfBuzz's command-line shaper (util/hb-shape.cc and its option
// headers), ported to C++23 against `import mcpplibs.mwrap.harfbuzz;`: the
// same options, output and exit codes, with GLib's option parser replaced by
// a small one of the same syntax (--name=value or --name value). Upstream's
// own shaping suites (test/shape/run-tests.py) run on it; see
// tools/run_conformance.py.
//
//   hb-shape [OPTION…] FONT-FILE [TEXT]
//   hb-shape --batch          # one ';'-separated command line per input line
//
// Ported from HarfBuzz 14.3.0, util/ — Copyright © 2010 Behdad Esfahbod;
// Copyright © 2011,2012 Google, Inc. (Google Author(s): Behdad Esfahbod).
// HarfBuzz is licensed under the "Old MIT" license:
//
// Permission is hereby granted, without written agreement and without
// license or royalty fees, to use, copy, modify, and distribute this
// software and its documentation for any purpose, provided that the
// above copyright notice and the following two paragraphs appear in
// all copies of this software.
//
// IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
// DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
// ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
// IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
// BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
// ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
// PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
import std;
import mcpplibs.mwrap.harfbuzz;

using namespace mcpplibs::mwrap::harfbuzz::macros;  // HB_DIRECTION_IS_HORIZONTAL, HB_FONT_NO_VAR_NAMED_INSTANCE …

namespace {

enum return_value_t {
    RETURN_VALUE_SUCCESS = 0,
    RETURN_VALUE_OPTION_PARSING_FAILED = 1,
    RETURN_VALUE_FACE_LOAD_FAILED = 2,
    RETURN_VALUE_OPERATION_FAILED = 3,
    RETURN_VALUE_FONT_FUNCS_FAILED = 4,
};
int return_value = RETURN_VALUE_SUCCESS;  // process-wide, as upstream's

constexpr double FONT_SIZE_UPEM = 0x7FFFFFFF;
constexpr double DEFAULT_FONT_SIZE = FONT_SIZE_UPEM;  // hb-shape's
constexpr unsigned SUBPIXEL_BITS = 0;

[[noreturn]] void fail(bool suggest_help, std::string_view message) {
    std::println(std::cerr, "hb-shape: {}", message);
    if (suggest_help) std::println(std::cerr, "Try `hb-shape --help' for more information.");
    if (return_value == RETURN_VALUE_SUCCESS) return_value = RETURN_VALUE_OPTION_PARSING_FAILED;
    std::cout.flush();
    std::exit(return_value);
}

std::string join(const char** list, std::string_view sep) {
    std::string out;
    for (const char** p = list; p && *p; ++p) {
        if (p != list) out += sep;
        out += *p;
    }
    return out;
}

[[noreturn]] void list_and_quit(const char** list) {
    for (const char** p = list; *p; ++p) std::println("{}", *p);
    std::cout.flush();
    std::exit(0);
}

// GLib's G_OPTION_ARG_INT: strtoll in base 0, the whole argument.
long long parse_int(std::string_view name, const std::string& arg) {
    char* end = nullptr;
    const long long v = std::strtoll(arg.c_str(), &end, 0);
    if (arg.empty() || *end != '\0' || v < std::numeric_limits<int>::min() || v > std::numeric_limits<unsigned>::max())
        fail(true, std::format("Cannot parse integer value “{}” for --{}", arg, name));
    return v;
}

double parse_double(std::string_view name, const std::string& arg) {
    char* end = nullptr;
    const double v = std::strtod(arg.c_str(), &end);
    if (arg.empty() || *end != '\0') fail(true, std::format("Cannot parse double value “{}” for --{}", arg, name));
    return v;
}

// sscanf (arg, "%lf%*[ ,]%lf", &x, &y): one or two numbers.
template <class T>
bool parse_pair(const std::string& arg, T& x, T& y) {
    const char* s = arg.c_str();
    char* end = nullptr;
    const double a = std::strtod(s, &end);
    if (end == s) return false;
    x = y = static_cast<T>(a);
    const char* p = end;
    if (*p != ' ' && *p != ',') return true;
    while (*p == ' ' || *p == ',') ++p;
    const double b = std::strtod(p, &end);
    if (end != p) y = static_cast<T>(b);
    return true;
}

// The UTF-8 GLib's g_unichar_to_utf8 writes: up to six bytes, so the
// out-of-range values of a test reach the buffer as upstream's tool sends them.
void append_unichar(std::string& out, std::uint32_t c) {
    int len;
    unsigned first;
    if (c < 0x80) first = 0, len = 1;
    else if (c < 0x800) first = 0xc0, len = 2;
    else if (c < 0x10000) first = 0xe0, len = 3;
    else if (c < 0x200000) first = 0xf0, len = 4;
    else if (c < 0x4000000) first = 0xf8, len = 5;
    else first = 0xfc, len = 6;
    char buf[6];
    for (int i = len - 1; i > 0; --i) {
        buf[i] = static_cast<char>((c & 0x3f) | 0x80);
        c >>= 6;
    }
    buf[0] = static_cast<char>(c | first);
    out.append(buf, static_cast<std::size_t>(len));
}

// text-options.hh's encode_unicodes: hex numbers between delimiters.
std::string encode_unicodes(const std::string& unicodes) {
    constexpr std::string_view delimiters = "<+-|>{},;&#\\xXuUnNiI\n\t\v\f\r ";
    std::string out;
    std::size_t i = 0;
    while (i < unicodes.size()) {
        while (i < unicodes.size() && delimiters.contains(unicodes[i])) ++i;
        if (i == unicodes.size()) break;
        const char* s = unicodes.c_str() + i;
        char* p = nullptr;
        const unsigned long long u = std::strtoull(s, &p, 16);
        if (p == s || u == std::numeric_limits<unsigned long long>::max())
            fail(true, std::format("Failed parsing Unicode value at: '{}'", s));
        append_unichar(out, static_cast<hb_codepoint_t>(u));
        i += static_cast<std::size_t>(p - s);
    }
    return out;
}

// parse_features / parse_variations: a comma- or space-separated list; an
// item that does not parse is skipped.
template <class T, class Parse>
std::vector<T> parse_list(std::string s, Parse parse) {
    std::vector<T> out;
    if (s.size() >= 2 && s.front() == s.back() && (s.front() == '"' || s.front() == '\'')) s = s.substr(1, s.size() - 2);
    std::size_t p = 0;
    while (p < s.size()) {
        const std::size_t end = s.find_first_of(", ", p);
        T item{};
        const auto len = end == std::string::npos ? -1 : static_cast<int>(end - p);
        if (parse(s.c_str() + p, len, &item)) out.push_back(item);
        if (end == std::string::npos) break;
        p = end + 1;
    }
    return out;
}

// face-options.hh's cache: a batch reuses the face of the previous line.
struct FaceCache {
    std::string path, loader;
    bool has_loader = false;
    unsigned index = static_cast<unsigned>(-1);
    hb_face_t* face = nullptr;
    ~FaceCache() { hb_face_destroy(face); }
} face_cache;

struct Options {
    // face
    std::optional<std::string> font_file;
    unsigned face_index = 0;
    std::optional<std::string> face_loader;
    // font
    bool sub_font = false;
    std::vector<hb_variation_t> variations;
    int x_ppem = 0, y_ppem = 0;
    double ptem = 0.;
    double x_embolden = 0., y_embolden = 0.;
    bool embolden_in_place = false;
    double slant = 0.;
    double font_size_x = DEFAULT_FONT_SIZE, font_size_y = DEFAULT_FONT_SIZE;
    std::optional<std::string> font_funcs;
    int ft_load_flags = 2;
    unsigned named_instance = HB_FONT_NO_VAR_NAMED_INSTANCE;
    // text
    std::optional<std::string> text;
    std::optional<std::string> text_file;
    bool single_par = false;
    std::optional<std::string> text_before, text_after;
    // shape
    std::optional<std::string> direction, language, script;
    bool bot = false, eot = false, preserve_default_ignorables = false, remove_default_ignorables = false;
    std::vector<hb_feature_t> features;
    std::vector<std::string> shapers;
    bool utf8_clusters = false;
    hb_codepoint_t invisible_glyph = 0, not_found_glyph = 0;
    hb_codepoint_t not_found_variation_selector_glyph = HB_CODEPOINT_INVALID;
    hb_buffer_cluster_level_t cluster_level = HB_BUFFER_CLUSTER_LEVEL_DEFAULT;
    bool normalize_glyphs = false, glyphs = false, scale_advances = true;
    bool verify = false, unsafe_to_concat = false, safe_to_insert_tatweel = false;
    unsigned num_iterations = 1;
    // output
    std::optional<std::string> output_file, output_format;
    bool show_glyph_names = true, show_positions = true, show_advances = true, show_clusters = true;
    bool show_text = false, show_unicode = false, show_line_num = false, show_extents = false, show_flags = false;
    bool trace = false;
};

constexpr std::string_view usage = R"(Usage:
  hb-shape [OPTION…] [FONT-FILE] [TEXT]

Shape text with given font: the results of shaping, in textual or JSON format.
Options (each also as --name=value): --font-file --face-index/-y --face-loader
--list-face-loaders --font-size --font-ppem --font-ptem --font-bold --font-grade
--font-slant --font-funcs --list-font-funcs --sub-font --ft-load-flags
--named-instance --variations --text --text-file --unicodes/-u --single-par
--text-before --text-after --unicodes-before --unicodes-after --shapers --shaper
--list-shapers --direction --language --script --bot --eot
--preserve-default-ignorables --remove-default-ignorables --invisible-glyph
--not-found-glyph --not-found-variation-selector-glyph --utf8-clusters
--cluster-level --normalize-glyphs --unsafe-to-concat --safe-to-insert-tatweel
--glyphs --verify --features --num-iterations/-n --output-file/-o
--output-format/-O --show-text --show-unicode --show-line-num --verbose/-v
--no-glyph-names --no-positions --no-advances --no-clusters --show-extents
--show-flags --ned --trace/-V --batch --version --help)";

Options parse(std::span<char*> argv) {
    Options o;
    bool text_from_unicodes = false;
    auto set_text = [&](std::string t, bool unicodes) {
        if (!unicodes) {  // parse_text
            if (o.text && t.empty()) return;
            if (o.text && text_from_unicodes && o.text->empty()) o.text.reset();
        }
        if (o.text) fail(true, "Either --text or --unicodes can be provided but not both");
        o.text = std::move(t);
        text_from_unicodes = unicodes;
    };
    std::vector<std::string> rest;
    bool options_done = false;
    for (std::size_t i = 1; i < argv.size(); ++i) {
        std::string arg = argv[i];
        if (options_done || arg.size() < 2 || arg[0] != '-') {
            rest.push_back(std::move(arg));
            continue;
        }
        if (arg == "--") {
            options_done = true;
            continue;
        }
        std::string name;
        std::optional<std::string> inline_value;
        if (arg.starts_with("--")) {
            const auto eq = arg.find('=');
            name = arg.substr(2, eq == std::string::npos ? std::string::npos : eq - 2);
            if (eq != std::string::npos) inline_value = arg.substr(eq + 1);
        } else {
            static const std::map<char, std::string_view> shorts = {
                {'y', "face-index"}, {'u', "unicodes"}, {'n', "num-iterations"}, {'o', "output-file"},
                {'O', "output-format"}, {'v', "verbose"}, {'V', "trace"}, {'h', "help"}};
            const auto it = shorts.find(arg[1]);
            if (it == shorts.end()) fail(true, std::format("Unknown option {}", arg));
            name = it->second;
            if (arg.size() > 2) inline_value = arg.substr(2);
        }
        auto value = [&]() -> std::string {
            if (inline_value) return *inline_value;
            if (i + 1 >= argv.size()) fail(true, std::format("Missing argument for --{}", name));
            return argv[++i];
        };
        auto flag = [&](bool& target, bool set) {
            if (inline_value) fail(true, std::format("Unexpected argument to --{}", name));
            target = set;
        };
        // face
        if (name == "font-file") o.font_file = value();
        else if (name == "face-index") o.face_index = static_cast<unsigned>(parse_int(name, value()));
        else if (name == "face-loader") o.face_loader = value();
        else if (name == "list-face-loaders") list_and_quit(hb_face_list_loaders());
        // font
        else if (name == "font-size") {
            const std::string v = value();
            if (v == "upem") o.font_size_x = o.font_size_y = FONT_SIZE_UPEM;
            else if (!parse_pair(v, o.font_size_x, o.font_size_y))
                fail(true, "--font-size argument should be one or two space-separated numbers");
        } else if (name == "font-ppem") {
            if (!parse_pair(value(), o.x_ppem, o.y_ppem)) fail(true, "--font-ppem argument should be one or two space-separated numbers");
        } else if (name == "font-ptem") o.ptem = parse_double(name, value());
        else if (name == "font-bold" || name == "font-grade") {
            o.embolden_in_place = name == "font-grade";
            if (!parse_pair(value(), o.x_embolden, o.y_embolden))
                fail(true, std::format("--{} argument should be one or two space-separated numbers", name));
        } else if (name == "font-slant") o.slant = parse_double(name, value());
        else if (name == "font-funcs") o.font_funcs = value();
        else if (name == "list-font-funcs") list_and_quit(hb_font_list_funcs());
        else if (name == "sub-font") flag(o.sub_font, true);
        else if (name == "ft-load-flags") o.ft_load_flags = static_cast<int>(parse_int(name, value()));
        else if (name == "named-instance") o.named_instance = static_cast<unsigned>(parse_int(name, value()));
        else if (name == "variations") o.variations = parse_list<hb_variation_t>(value(), hb_variation_from_string);
        // text
        else if (name == "text") set_text(value(), false);
        else if (name == "text-file") o.text_file = value();
        else if (name == "unicodes") {
            const std::string v = value();
            set_text(v == "*" ? std::string("*") : encode_unicodes(v), true);
        } else if (name == "single-par") flag(o.single_par, true);
        else if (name == "text-before" || name == "unicodes-before") {
            if (o.text_before) fail(true, "Either --text-before or --unicodes-before can be provided but not both");
            o.text_before = name == "text-before" ? value() : encode_unicodes(value());
        } else if (name == "text-after" || name == "unicodes-after") {
            if (o.text_after) fail(true, "Either --text-after or --unicodes-after can be provided but not both");
            o.text_after = name == "text-after" ? value() : encode_unicodes(value());
        }
        // shape
        else if (name == "shaper" || name == "shapers") {
            std::vector<std::string> list;
            const std::string v = value();
            for (auto part : std::views::split(std::string_view(v), ',')) list.emplace_back(std::string_view(part));
            for (const std::string& s : list) {
                bool found = false;
                for (const char** h = hb_shape_list_shapers(); *h; ++h) found = found || s == *h;
                if (!found) fail(true, std::format("Unknown or unsupported shaper: {}", s));
            }
            o.shapers = std::move(list);
        } else if (name == "list-shapers") list_and_quit(hb_shape_list_shapers());
        else if (name == "direction") o.direction = value();
        else if (name == "language") o.language = value();
        else if (name == "script") o.script = value();
        else if (name == "bot") flag(o.bot, true);
        else if (name == "eot") flag(o.eot, true);
        else if (name == "preserve-default-ignorables") flag(o.preserve_default_ignorables, true);
        else if (name == "remove-default-ignorables") flag(o.remove_default_ignorables, true);
        else if (name == "invisible-glyph") o.invisible_glyph = static_cast<hb_codepoint_t>(parse_int(name, value()));
        else if (name == "not-found-glyph") o.not_found_glyph = static_cast<hb_codepoint_t>(parse_int(name, value()));
        else if (name == "not-found-variation-selector-glyph")
            o.not_found_variation_selector_glyph = static_cast<hb_codepoint_t>(parse_int(name, value()));
        else if (name == "utf8-clusters") flag(o.utf8_clusters, true);
        else if (name == "cluster-level") o.cluster_level = static_cast<hb_buffer_cluster_level_t>(parse_int(name, value()));
        else if (name == "normalize-glyphs") flag(o.normalize_glyphs, true);
        else if (name == "unsafe-to-concat") flag(o.unsafe_to_concat, true);
        else if (name == "safe-to-insert-tatweel") flag(o.safe_to_insert_tatweel, true);
        else if (name == "glyphs") flag(o.glyphs, true);
        else if (name == "verify") flag(o.verify, true);
        else if (name == "features") o.features = parse_list<hb_feature_t>(value(), hb_feature_from_string);
        else if (name == "num-iterations") o.num_iterations = static_cast<unsigned>(parse_int(name, value()));
        // output
        else if (name == "output-file") o.output_file = value();
        else if (name == "output-format") o.output_format = value();
        else if (name == "show-text") flag(o.show_text, true);
        else if (name == "show-unicode") flag(o.show_unicode, true);
        else if (name == "show-line-num") flag(o.show_line_num, true);
        else if (name == "verbose") {
            bool on = false;
            flag(on, true);
            o.show_text = o.show_unicode = o.show_line_num = true;
        } else if (name == "no-glyph-names") flag(o.show_glyph_names, false);
        else if (name == "no-positions") flag(o.show_positions, false);
        else if (name == "no-advances") flag(o.show_advances, false);
        else if (name == "no-clusters") flag(o.show_clusters, false);
        else if (name == "show-extents") flag(o.show_extents, true);
        else if (name == "show-flags") flag(o.show_flags, true);
        else if (name == "ned") {
            bool on = false;
            flag(on, true);
            o.show_clusters = o.show_advances = false;
        } else if (name == "trace") flag(o.trace, true);
        else if (name == "version") {
            std::println("hb-shape (HarfBuzz) {}\nAvailable shapers: {}", hb_version_string(), join(hb_shape_list_shapers(), ","));
            std::exit(0);
        } else if (name == "help") {
            std::println("{}", usage);
            std::exit(0);
        } else
            fail(true, std::format("Unknown option {}", arg));
    }
    for (std::string& r : rest) {  // main-font-text.hh's collect_rest
        if (!o.font_file) o.font_file = std::move(r);
        else if (!o.text && !o.text_file) o.text = std::move(r);
        else fail(true, "Too many arguments on the command line");
    }
    return o;
}

class Shaper {
public:
    explicit Shaper(Options o) : o_(std::move(o)) {}

    int run() {
        post_parse();
        buffer_ = hb_buffer_create();
        if (o_.trace) hb_buffer_set_message_func(buffer_, message_func, this, nullptr);
        bool failed = false;
        for (unsigned line_no = 1;; ++line_no) {
            const std::optional<std::string> line = get_line();
            if (!line) break;
            line_no_ = line_no;
            bool shaped = true;
            for (unsigned n = o_.num_iterations; n && shaped; --n) {
                populate_buffer(*line);
                if (n == 1) consume_text(*line);
                if (!shape()) {
                    shaped = false;
                    failed = true;
                    *out_ << serialize_line_no() << "error: Shaping failed.\n";
                }
            }
            if (!shaped) continue;
            *out_ << serialize_line_no() << serialize(serialize_format_, serialize_flags_) << '\n';
        }
        out_->flush();
        hb_buffer_set_message_func(buffer_, nullptr, nullptr, nullptr);
        hb_buffer_destroy(buffer_);
        hb_font_destroy(font_);
        hb_face_destroy(face_);
        if (failed && return_value == RETURN_VALUE_SUCCESS) return_value = RETURN_VALUE_OPERATION_FAILED;
        return return_value;
    }

private:
    void post_parse() {
        // face
        if (!o_.font_file) fail(true, "No font file set");
        std::string path = *o_.font_file;
        if (path == "-") path = "/dev/stdin";
        const bool same_loader = face_cache.has_loader == o_.face_loader.has_value() &&
                                 (!o_.face_loader || face_cache.loader == *o_.face_loader);
        if (face_cache.path != path || !same_loader || face_cache.index != o_.face_index || !face_cache.face) {
            hb_face_destroy(face_cache.face);
            face_cache.face = hb_face_create_from_file_or_fail_using(path.c_str(), o_.face_index,
                                                                      o_.face_loader ? o_.face_loader->c_str() : nullptr);
            face_cache.path = path;
            face_cache.index = o_.face_index;
            face_cache.has_loader = o_.face_loader.has_value();
            face_cache.loader = o_.face_loader.value_or("");
            if (!face_cache.face) {
                return_value = RETURN_VALUE_FACE_LOAD_FAILED;
                fail(true, std::format("{}: Failed loading font face", path));
            }
        }
        face_ = hb_face_reference(face_cache.face);
        // font
        font_ = hb_font_create(face_);
        if (o_.font_size_x == FONT_SIZE_UPEM) o_.font_size_x = hb_face_get_upem(face_);
        if (o_.font_size_y == FONT_SIZE_UPEM) o_.font_size_y = hb_face_get_upem(face_);
        hb_font_set_ppem(font_, static_cast<unsigned>(o_.x_ppem), static_cast<unsigned>(o_.y_ppem));
        hb_font_set_ptem(font_, static_cast<float>(o_.ptem));
        hb_font_set_synthetic_bold(font_, static_cast<float>(o_.x_embolden), static_cast<float>(o_.y_embolden), o_.embolden_in_place);
        hb_font_set_synthetic_slant(font_, static_cast<float>(o_.slant));
        const int scale_x = static_cast<int>(std::scalbn(static_cast<float>(o_.font_size_x), static_cast<int>(SUBPIXEL_BITS)));
        const int scale_y = static_cast<int>(std::scalbn(static_cast<float>(o_.font_size_y), static_cast<int>(SUBPIXEL_BITS)));
        hb_font_set_scale(font_, scale_x, scale_y);
        if (o_.named_instance != HB_FONT_NO_VAR_NAMED_INSTANCE) hb_font_set_var_named_instance(font_, o_.named_instance);
        hb_font_set_variations(font_, o_.variations.data(), static_cast<unsigned>(o_.variations.size()));
        if (o_.font_funcs && !hb_font_set_funcs_using(font_, o_.font_funcs->c_str())) {
            return_value = RETURN_VALUE_FONT_FUNCS_FAILED;
            const char** supported = hb_font_list_funcs();
            if (!supported[0]) fail(true, std::format("Unknown font function implementation `{}'; no supported values found", *o_.font_funcs));
            fail(true, std::format("Unknown font function implementation `{}'; supported values are: {}; default is {}",
                                   *o_.font_funcs, join(supported, "/"), supported[0]));
        }
        hb_ft_font_set_load_flags(font_, o_.ft_load_flags);  // mcpplibs.mwrap.harfbuzz.ft
        if (o_.sub_font) {
            hb_font_t* old = font_;
            font_ = hb_font_create_sub_font(old);
            hb_font_set_scale(old, scale_x * 2, scale_y * 2);
            hb_font_destroy(old);
        }
        // text
        if (!o_.text && !o_.text_file) o_.text_file = "-";
        if (o_.text && o_.text_file) fail(true, "Only one of text and text-file can be set");
        if (o_.text_file && *o_.text_file != "-") {
            file_in_.open(*o_.text_file, std::ios::binary);
            if (!file_in_) fail(true, std::format("Failed opening text file `{}'", *o_.text_file));
            in_ = &file_in_;
        }
        // output
        const bool explicit_format = o_.output_format.has_value();
        if (o_.output_file && !o_.output_format) {
            const auto dot = o_.output_file->rfind('.');
            if (dot != std::string::npos) o_.output_format = o_.output_file->substr(dot + 1);
        }
        if (o_.output_file && *o_.output_file != "-") {
            file_out_.open(*o_.output_file, std::ios::binary);
            if (!file_out_) fail(true, std::format("Cannot open output file `{}'", *o_.output_file));
            out_ = &file_out_;
        }
        serialize_format_ = !o_.output_format ? HB_BUFFER_SERIALIZE_FORMAT_TEXT
                                              : hb_buffer_serialize_format_from_string(o_.output_format->c_str(), -1);
        if ((!o_.output_format || !o_.output_format->empty()) && !hb_buffer_serialize_format_to_string(serialize_format_)) {
            if (explicit_format)
                fail(false, std::format("Unknown output format `{}'; supported formats are: {}", *o_.output_format,
                                        join(hb_buffer_serialize_list_formats(), "/")));
            serialize_format_ = HB_BUFFER_SERIALIZE_FORMAT_TEXT;
        }
        unsigned flags = HB_BUFFER_SERIALIZE_FLAG_DEFAULT;
        if (!o_.show_glyph_names) flags |= HB_BUFFER_SERIALIZE_FLAG_NO_GLYPH_NAMES;
        if (!o_.show_clusters) flags |= HB_BUFFER_SERIALIZE_FLAG_NO_CLUSTERS;
        if (!o_.show_positions) flags |= HB_BUFFER_SERIALIZE_FLAG_NO_POSITIONS;
        if (!o_.show_advances) flags |= HB_BUFFER_SERIALIZE_FLAG_NO_ADVANCES;
        if (o_.show_extents) flags |= HB_BUFFER_SERIALIZE_FLAG_GLYPH_EXTENTS;
        if (o_.show_flags) flags |= HB_BUFFER_SERIALIZE_FLAG_GLYPH_FLAGS;
        serialize_flags_ = static_cast<hb_buffer_serialize_flags_t>(flags);
    }

    // text_options_t::get_line
    std::optional<std::string> get_line() {
        if (o_.text) {
            if (!started_) {
                started_ = true;
                remaining_ = *o_.text;
            }
            if (remaining_.empty()) return std::nullopt;
            const std::size_t nl = o_.single_par ? std::string::npos : remaining_.find('\n');
            std::string line = remaining_.substr(0, nl);
            remaining_ = nl == std::string::npos ? std::string() : remaining_.substr(nl + 1);
            return line;
        }
        std::string line;
        if (o_.single_par) {
            line.assign(std::istreambuf_iterator<char>(*in_), {});
            if (line.empty()) return std::nullopt;
            return line;
        }
        if (!std::getline(*in_, line)) return std::nullopt;
        return line;
    }

    void setup_buffer() {
        hb_buffer_set_direction(buffer_, hb_direction_from_string(o_.direction ? o_.direction->c_str() : nullptr, -1));
        hb_buffer_set_script(buffer_, hb_script_from_string(o_.script ? o_.script->c_str() : nullptr, -1));
        hb_buffer_set_language(buffer_, hb_language_from_string(o_.language ? o_.language->c_str() : nullptr, -1));
        hb_buffer_set_flags(buffer_, static_cast<hb_buffer_flags_t>(
                                         HB_BUFFER_FLAG_DEFAULT | (o_.bot ? HB_BUFFER_FLAG_BOT : 0) | (o_.eot ? HB_BUFFER_FLAG_EOT : 0) |
                                         (o_.verify ? HB_BUFFER_FLAG_VERIFY : 0) |
                                         (o_.unsafe_to_concat ? HB_BUFFER_FLAG_PRODUCE_UNSAFE_TO_CONCAT : 0) |
                                         (o_.safe_to_insert_tatweel ? HB_BUFFER_FLAG_PRODUCE_SAFE_TO_INSERT_TATWEEL : 0) |
                                         (o_.preserve_default_ignorables ? HB_BUFFER_FLAG_PRESERVE_DEFAULT_IGNORABLES : 0) |
                                         (o_.remove_default_ignorables ? HB_BUFFER_FLAG_REMOVE_DEFAULT_IGNORABLES : 0)));
        hb_buffer_set_invisible_glyph(buffer_, o_.invisible_glyph);
        hb_buffer_set_not_found_glyph(buffer_, o_.not_found_glyph);
        hb_buffer_set_not_found_variation_selector_glyph(buffer_, o_.not_found_variation_selector_glyph);
        hb_buffer_set_cluster_level(buffer_, o_.cluster_level);
        hb_buffer_guess_segment_properties(buffer_);
    }

    void populate_buffer(const std::string& text) {
        hb_buffer_clear_contents(buffer_);
        if (o_.glyphs) {
            // setup_buffer first, while the buffer is empty: guessing the
            // segment properties does not take glyphs.
            setup_buffer();
            std::string glyphs = text;
            if (!glyphs.empty() && glyphs.back() != ']') glyphs += ']';
            hb_buffer_deserialize_glyphs(buffer_, glyphs.c_str(), static_cast<int>(glyphs.size()), nullptr, font_,
                                         HB_BUFFER_SERIALIZE_FORMAT_TEXT);
            if (!glyphs.contains('+') && !glyphs.contains('@')) {
                o_.scale_advances = false;
                unsigned count = 0;
                const hb_direction_t direction = hb_buffer_get_direction(buffer_);
                hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer_, &count);
                hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer_, &count);
                for (unsigned i = 0; i < count; i++)
                    hb_font_get_glyph_advance_for_direction(font_, infos[i].codepoint, direction, &positions[i].x_advance,
                                                            &positions[i].y_advance);
            }
            return;
        }
        if (o_.text_before) {
            const auto len = static_cast<int>(o_.text_before->size());
            hb_buffer_add_utf8(buffer_, o_.text_before->c_str(), len, static_cast<unsigned>(len), 0);
        }
        hb_buffer_add_utf8(buffer_, text.c_str(), static_cast<int>(text.size()), 0, static_cast<int>(text.size()));
        if (o_.text_after) hb_buffer_add_utf8(buffer_, o_.text_after->c_str(), -1, 0, 0);
        if (!o_.utf8_clusters) {
            // Cluster values of Unicode character indices, not UTF-8 ones.
            const unsigned n = hb_buffer_get_length(buffer_);
            hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buffer_, nullptr);
            for (unsigned i = 0; i < n; i++) info[i].cluster = i;
        }
        setup_buffer();
    }

    bool shape() {
        if (o_.glyphs) {
            int x_scale = 0, y_scale = 0;
            hb_font_get_scale(font_, &x_scale, &y_scale);
            const int upem = static_cast<int>(hb_face_get_upem(hb_font_get_face(font_)));
            if (x_scale != upem || y_scale != upem) {
                unsigned count = 0;
                hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer_, &count);
                for (unsigned i = 0; i < count; i++) {
                    hb_glyph_position_t& pos = positions[i];
                    pos.x_offset = pos.x_offset * x_scale / upem;
                    pos.y_offset = pos.y_offset * y_scale / upem;
                    if (o_.scale_advances) {
                        pos.x_advance = pos.x_advance * x_scale / upem;
                        pos.y_advance = pos.y_advance * y_scale / upem;
                    }
                }
            }
        } else {
            std::vector<const char*> shapers;
            for (const std::string& s : o_.shapers) shapers.push_back(s.c_str());
            shapers.push_back(nullptr);
            if (!hb_shape_full(font_, buffer_, o_.features.data(), static_cast<unsigned>(o_.features.size()),
                               o_.shapers.empty() ? nullptr : shapers.data()))
                return false;
        }
        if (o_.normalize_glyphs) hb_buffer_normalize_glyphs(buffer_);
        return true;
    }

    std::string serialize_line_no() const { return o_.show_line_num ? std::format("{}: ", line_no_) : std::string(); }

    std::string serialize(hb_buffer_serialize_format_t format, hb_buffer_serialize_flags_t flags) const {
        std::string out;
        const unsigned n = hb_buffer_get_length(buffer_);
        unsigned start = 0;
        while (start < n) {
            char buf[32768];
            unsigned consumed = 0;
            start += hb_buffer_serialize(buffer_, start, n, buf, sizeof buf, &consumed, font_, format, flags);
            if (!consumed) break;
            out += buf;
        }
        return out;
    }

    void consume_text(const std::string& text) {
        if (o_.show_text) *out_ << serialize_line_no() << '(' << text << ")\n";
        if (o_.show_unicode)
            *out_ << serialize_line_no() << serialize(HB_BUFFER_SERIALIZE_FORMAT_TEXT, HB_BUFFER_SERIALIZE_FLAG_DEFAULT) << '\n';
    }

    static hb_bool_t message_func(hb_buffer_t*, hb_font_t*, const char* message, void* user_data) {
        const auto* self = static_cast<const Shaper*>(user_data);
        std::print(std::cerr, "{}trace: {}\tbuffer: {}\n", self->serialize_line_no(), message,
                   self->serialize(self->serialize_format_, self->serialize_flags_));
        return true;
    }

    Options o_;
    hb_face_t* face_ = nullptr;
    hb_font_t* font_ = nullptr;
    hb_buffer_t* buffer_ = nullptr;
    hb_buffer_serialize_format_t serialize_format_ = HB_BUFFER_SERIALIZE_FORMAT_INVALID;
    hb_buffer_serialize_flags_t serialize_flags_ = HB_BUFFER_SERIALIZE_FLAG_DEFAULT;
    unsigned line_no_ = 0;
    bool started_ = false;
    std::string remaining_;
    std::istream* in_ = &std::cin;
    std::ifstream file_in_;
    std::ostream* out_ = &std::cout;
    std::ofstream file_out_;
};

int run(std::span<char*> argv) { return Shaper(parse(argv)).run(); }

}  // namespace

int main(int argc, char** argv) {
    std::span<char*> args(argv, static_cast<std::size_t>(argc));
    if (argc == 2 && std::string_view(argv[1]) == "--batch") {
        // batch.hh: each input line is one command line, its arguments
        // separated by ';' (a run of them is one separator).
        int ret = 0;
        std::string line;
        while (std::getline(std::cin, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            std::vector<std::string> parts{std::string(argv[0])};
            std::size_t p = 0;
            parts.emplace_back();
            for (; p < line.size(); ++p) {
                if (line[p] == ';') {
                    while (p + 1 < line.size() && line[p + 1] == ';') ++p;
                    parts.emplace_back();
                } else
                    parts.back() += line[p];
            }
            std::vector<char*> cargs;
            for (std::string& s : parts) cargs.push_back(s.data());
            ret = std::max(ret, run(cargs));
            std::cout.flush();
        }
        return ret;
    }
    return run(args);
}
