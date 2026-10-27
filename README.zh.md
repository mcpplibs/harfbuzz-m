# harfbuzz

> 通过 C++23 模块使用 HarfBuzz（OpenType 文本整形引擎）。

[English](README.md) · **简体中文** · [上游](https://github.com/harfbuzz/harfbuzz) · [发布](https://github.com/mcpplibs/harfbuzz-m/releases) · [mcpp](https://github.com/mcpp-community/mcpp) · [索引](https://github.com/mcpplibs/mcpp-index)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![mcpp-community](https://img.shields.io/badge/mcpp--community-mwrap-blue)](https://github.com/mcpp-community)
[![CI](https://github.com/mcpplibs/harfbuzz-m/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/mcpplibs/harfbuzz-m/actions/workflows/ci.yml)

上游的 C API 保持不变：`hb_buffer_t`、`hb_shape`、`hb_font_create`、
`HB_SCRIPT_ARABIC` 就是 `<hb.h>`、`<hb-ot.h>`、`<hb-aat.h>` 和 `<hb-ft.h>`
里的上游全局名字；`hb::shared_ptr` 和 `hb::unique_ptr` 就是
`<hb-cplusplus.hh>` 里的那两个。库本身是带 FreeType 后端的
`compat.harfbuzz` 14.3.0。宏被映射成常量和 constexpr 函数。每次 CI 都会在
一个只用本模块写成的上游 `hb-shape` 移植版上跑 HarfBuzz 自带的整形测试集，并且
全部通过。

## 模块

- `import mcpplibs.mwrap.harfbuzz;`：完整 API（533 个函数、142 个类型、805 个枚举值、`namespace hb`），并重新导出下面两个模块
- `import mcpplibs.mwrap.harfbuzz.ft;`：基于 FreeType `FT_Face` 的 `hb_ft_font_create`、`hb_ft_font_set_funcs` 等
- `import mcpplibs.mwrap.harfbuzz.macros;`：`HB_TAG`、`HB_OT_TAG_GSUB`、`HB_LANGUAGE_INVALID` 等，位于 `mcpplibs::mwrap::harfbuzz::macros`

> 更多模块 API -> [用法和 API](docs/usage.md)

## 快速开始

```sh
mcpp new banner --template mcpplibs.mwrap.harfbuzz@14.3.0
cd banner
mcpp run
mcpp run -- /path/to/font.ttf "Text" -liga,-kern
```

默认模板用自带的一个小字体（含连字和字距调整）整形一行文字，打印字形序列，
再根据字形轮廓在终端里画出这行字。给出字体文件时，可以整形任意文本。

```toml
[dependencies]
"mcpplibs.mwrap.harfbuzz" = "14.3.0"
```

```cpp
import std;
import mcpplibs.mwrap.harfbuzz;
using namespace mcpplibs::mwrap::harfbuzz::macros;  // HB_TAG、HB_LANGUAGE_INVALID …

int main(int argc, char** argv) {
    hb_face_t* face = hb_face_create_from_file_or_fail(argv[1], 0);
    if (!face) return 1;
    hb_font_t* font = hb_font_create(face);
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, "office", -1, 0, -1);
    hb_buffer_guess_segment_properties(buffer);
    hb_feature_t kern_off{HB_TAG('k', 'e', 'r', 'n'), 0, HB_FEATURE_GLOBAL_START, HB_FEATURE_GLOBAL_END};
    hb_shape(font, buffer, &kern_off, 1);
    unsigned n = 0;
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buffer, &n);
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buffer, &n);
    for (unsigned i = 0; i < n; ++i)
        std::println("glyph {} cluster {} advance {}", info[i].codepoint, info[i].cluster, pos[i].x_advance);
    hb_buffer_destroy(buffer);
    hb_font_destroy(font);
    hb_face_destroy(face);
}
```

## 宏与 FreeType

命名模块不能导出宏。常量（`HB_OT_TAG_*`、`HB_CODEPOINT_INVALID`、
`HB_FEATURE_GLOBAL_END`、`HB_VERSION_*` …）、初始化器
`HB_SEGMENT_PROPERTIES_DEFAULT` / `HB_DRAW_STATE_DEFAULT` 以及计算宏
（`HB_TAG`、`HB_COLOR`、`HB_DIRECTION_IS_*`、`HB_VERSION_ATLEAST` …）都在
`mcpplibs::mwrap::harfbuzz::macros` 里。`HB_UNTAG`（展开成调用的四个参数）和
`#if HB_VERSION_ATLEAST(…)` 需要在 import 之前包含
`<mwrap/harfbuzz/macros.hpp>`。`FT_Face` 属于 FreeType：import
`mcpplibs.mwrap.freetype` 或包含 `<ft2build.h>`，两种方式得到的是同一个类型。
详见 [docs/usage.md](docs/usage.md)。

## 示例

- [examples/hb-shape](examples/hb-shape)：上游的命令行整形工具 `hb-shape`（相同的选项、输出和
  `--batch` 模式），用本模块移植成 C++23；[tools/run_conformance.py](tools/run_conformance.py) 在它上面跑
  HarfBuzz 的 `test/shape` 测试集。

## 构建与测试

Linux、macOS 和 Windows CI 会构建模块并运行测试：拉丁、阿拉伯和天城文的整形
结果与另一份独立 HarfBuzz 构建的结果比对，缓冲区与序列化，集合与映射，Unicode
函数，绘制与绘画函数，OpenType 和 AAT 查询，FreeType 集成，C++ 辅助类，宏，以及
头文件与 import 混用。导出审计（[tools/check_exports.py](tools/check_exports.py)）
检查生成的接口是最新的、每个声明都已导出、每个宏要么已映射要么列明了原因。上游的
`in-house`、`aots` 和 `text-rendering-tests` 整形测试集分别用 OpenType 和
FreeType 字体函数运行。另有一个作业把默认模板当作已安装的包生成项目并运行。

## 许可证

封装部分使用 [Apache-2.0](LICENSE)，作者为 mcpp-community；HarfBuzz 仍使用它的
"Old MIT" 许可证（[upstream/harfbuzz/COPYING](upstream/harfbuzz/COPYING)）。
测试字体保留各自的许可证（[tests/fonts](tests/fonts/README.md)）。
