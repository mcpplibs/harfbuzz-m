#!/usr/bin/env python3
"""Make the sample font of the default template (templates/shape).

A tiny TrueType font drawn here, Apache-2.0 like the rest of the wrapper (no
third-party font, no licence to carry into a generated project): the letters
A V f i l t and the space; the ligatures f_i and f_l (GSUB liga) and the kern
pairs AV / VA (GPOS kern), so a short text shows what shaping does; glyph
names (post format 2). Its bytes become a constexpr array in
templates/shape/src/sample_font.cppm.in.

    python3 -m pip install fonttools==4.67.0
    python3 tools/make_sample_font.py
"""
import io
import os
from pathlib import Path

os.environ.setdefault('SOURCE_DATE_EPOCH', '1767225600')  # head.created/modified: 2026-01-01

from fontTools.fontBuilder import FontBuilder  # noqa: E402
from fontTools.feaLib.builder import addOpenTypeFeaturesFromString  # noqa: E402
from fontTools.pens.ttGlyphPen import TTGlyphPen  # noqa: E402


def rect(pen, x0, y0, x1, y1):
    # Clockwise: TrueType's outer contours.
    pen.moveTo((x0, y0))
    pen.lineTo((x0, y1))
    pen.lineTo((x1, y1))
    pen.lineTo((x1, y0))
    pen.closePath()


def poly(pen, points):
    pen.moveTo(points[0])
    for p in points[1:]:
        pen.lineTo(p)
    pen.closePath()


def glyph(draw):
    pen = TTGlyphPen(None)
    draw(pen)
    return pen.glyph()


def f_shape(pen, dx=0):
    rect(pen, dx + 100, 0, dx + 200, 650)        # stem
    rect(pen, dx + 100, 650, dx + 380, 750)      # hook
    rect(pen, dx + 0, 400, dx + 340, 490)        # crossbar


GLYPHS = {
    '.notdef': (500, lambda p: (rect(p, 50, 0, 450, 700), poly(p, [(120, 70), (380, 70), (380, 630), (120, 630)]))),
    'space': (250, lambda p: None),
    'A': (600, lambda p: (poly(p, [(0, 0), (260, 700), (340, 700), (600, 0), (480, 0), (420, 180), (180, 180), (120, 0)]),
                          poly(p, [(215, 280), (385, 280), (300, 540)]))),
    'V': (600, lambda p: poly(p, [(0, 700), (120, 700), (300, 150), (480, 700), (600, 700), (340, 0), (260, 0)])),
    'f': (400, f_shape),
    'i': (300, lambda p: (rect(p, 100, 0, 200, 500), rect(p, 100, 600, 200, 700))),
    'l': (300, lambda p: rect(p, 100, 0, 200, 750)),
    't': (400, lambda p: (rect(p, 100, 0, 200, 650), rect(p, 0, 400, 340, 490))),
    'f_i': (600, lambda p: (f_shape(p), rect(p, 400, 0, 500, 650))),
    'f_l': (600, lambda p: (rect(p, 100, 0, 200, 650), rect(p, 100, 650, 500, 750), rect(p, 0, 400, 340, 490),
                            rect(p, 400, 0, 500, 750))),
}

FEATURES = '''
languagesystem DFLT dflt;
languagesystem latn dflt;
feature liga { sub f i by f_i; sub f l by f_l; } liga;
feature kern { pos A V -120; pos V A -120; } kern;
'''


def build() -> bytes:
    order = list(GLYPHS)
    fb = FontBuilder(1000, isTTF=True)
    fb.setupGlyphOrder(order)
    fb.setupCharacterMap({ord(c): c for c in 'AVfilt'} | {0x20: 'space'})
    fb.setupGlyf({name: glyph(draw) for name, (_, draw) in GLYPHS.items()})
    metrics = {}
    glyf = fb.font['glyf']
    for name, (advance, _) in GLYPHS.items():
        g = glyf[name]
        metrics[name] = (advance, getattr(g, 'xMin', 0))
    fb.setupHorizontalMetrics(metrics)
    fb.setupHorizontalHeader(ascent=800, descent=-200)
    fb.setupNameTable({
        'familyName': 'mwrap Sample',
        'styleName': 'Regular',
        'copyright': 'Copyright 2026 mcpp-community contributors',
        'licenseDescription': 'Licensed under the Apache License, Version 2.0',
        'licenseInfoURL': 'https://www.apache.org/licenses/LICENSE-2.0',
    })
    fb.setupOS2(version=4, sTypoAscender=800, sTypoDescender=-200, sTypoLineGap=0, usWinAscent=800, usWinDescent=200,
                sxHeight=500, sCapHeight=700, fsSelection=0x40 | 0x80)
    fb.setupPost(keepGlyphNames=True)
    addOpenTypeFeaturesFromString(fb.font, FEATURES)
    out = io.BytesIO()
    fb.save(out)
    return out.getvalue()


def render(data: bytes) -> str:
    rows = [', '.join(f'0x{b:02x}' for b in data[i:i + 16]) for i in range(0, len(data), 16)]
    body = ',\n    '.join(rows)
    return f'''// The sample font of this template: {len(data)} bytes of TrueType, drawn for
// it (letters A V f i l t and the space, the ligatures f_i and f_l, the kern
// pairs AV and VA, glyph names). Copyright 2026 mcpp-community contributors,
// Apache-2.0. Made by harfbuzz-m's tools/make_sample_font.py.
export module sample_font;

import std;

export constexpr std::array<unsigned char, {len(data)}> sample_font = {{
    {body},
}};
'''


def main():
    root = Path(__file__).resolve().parents[1]
    data = build()
    path = root / 'templates/shape/src/sample_font.cppm.in'
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(render(data), encoding='utf-8', newline='\n')
    (root / 'target').mkdir(exist_ok=True)
    (root / 'target/sample-font.ttf').write_bytes(data)
    print(f'wrote {path.relative_to(root).as_posix()} ({len(data)} bytes of font)')


if __name__ == '__main__':
    main()
