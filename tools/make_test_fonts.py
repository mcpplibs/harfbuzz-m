#!/usr/bin/env python3
"""Make tests/fonts: the fonts the API tests shape, draw and query.

Each source is downloaded at a pinned revision and checked against its
SHA-256; the Noto fonts are reduced with fontTools (instanced to a static
Regular where variations are not under test, subset to the scripts the tests
use, OpenType layout and glyph names kept). The outputs and their checksums
are recorded in tests/fonts/fonts.json. Re-running this script with the
fontTools version recorded there reproduces the committed bytes; --check only
verifies the committed files against fonts.json.

    python3 -m pip install fonttools==4.67.0
    python3 tools/make_test_fonts.py
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import os
import urllib.request

GOOGLE_FONTS = 'https://raw.githubusercontent.com/google/fonts/a4c0b21f839d7ac65a7d8b49eb81bddd110774a3/ofl/'
CHROMACHECK = 'https://raw.githubusercontent.com/RoelN/ChromaCheck/d2b0f87bf3e88e1a443d7fceec68ff6eb331df1e/'

SOURCES = {
    'NotoSans[wdth,wght].ttf': (GOOGLE_FONTS + 'notosans/NotoSans%5Bwdth,wght%5D.ttf',
                                'bfb7bb691513f12e734dc346c03a03f784912432d7e3fa8e56efcf906fe86b3d'),
    'NotoSansArabic[wdth,wght].ttf': (GOOGLE_FONTS + 'notosansarabic/NotoSansArabic%5Bwdth,wght%5D.ttf',
                                      '63111b5b2e074dd48cc67692e0a2726d86ee94c1c37fe8598257b7b4e87e869e'),
    'NotoSansDevanagari[wdth,wght].ttf': (GOOGLE_FONTS + 'notosansdevanagari/NotoSansDevanagari%5Bwdth,wght%5D.ttf',
                                          '14ec4af41f27482216d1c2229f417ff9b1425e1babb014e57d1d40d03229853e'),
    'OFL-NotoSans.txt': (GOOGLE_FONTS + 'notosans/OFL.txt',
                         'cee9892f9f0cc8fe882c9e9537ee6a89621d86ee7ceaf70b02e2b2b1c25c061a'),
    'OFL-NotoSansArabic.txt': (GOOGLE_FONTS + 'notosansarabic/OFL.txt', None),
    'OFL-NotoSansDevanagari.txt': (GOOGLE_FONTS + 'notosansdevanagari/OFL.txt', None),
    'LICENSE-ChromaCheck.md': (CHROMACHECK + 'LICENSE.md', None),
}

KEEP = ['--layout-features=*', '--name-IDs=*', '--glyph-names', '--notdef-outline']

# output: (source, instance location or None, unicodes)
FONTS = {
    'NotoSans-Latin.ttf': ('NotoSans[wdth,wght].ttf', None, 'U+0020-007E,U+00A0-00FF,U+2026'),
    'NotoSansArabic-Regular.ttf': ('NotoSansArabic[wdth,wght].ttf', ['wght=400', 'wdth=100'],
                                   'U+0020,U+0621-0655,U+0660-066D,U+200C-200F,U+25CC'),
    'NotoSansDevanagari-Regular.ttf': ('NotoSansDevanagari[wdth,wght].ttf', ['wght=400', 'wdth=100'],
                                       'U+0020,U+0900-0970,U+200C-200D,U+25CC'),
}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def download(name: str, work: Path, upstream: Path) -> Path:
    url, digest = SOURCES[name]
    path = work / name
    if not path.exists():
        print('+ fetch', url, flush=True)
        with urllib.request.urlopen(url, timeout=300) as response:
            path.write_bytes(response.read())
    if digest and sha256(path) != digest:
        sys.exit(f'{name}: checksum {sha256(path)} != {digest}')
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    out = root / 'tests/fonts'
    manifest = json.loads((out / 'fonts.json').read_text(encoding='utf-8')) if (out / 'fonts.json').exists() else {}
    if args.check:
        bad = [f for f, entry in manifest['files'].items() if sha256(out / f) != entry['sha256']]
        if bad:
            sys.exit('tests/fonts differ from fonts.json: ' + ', '.join(bad))
        print(f'{len(manifest["files"])} test fonts and licenses match tests/fonts/fonts.json')
        return
    import fontTools  # noqa: F401  (the tool that makes the subsets)
    sys.path.insert(0, str(root / 'tools'))
    from inventory import fetch
    upstream = fetch(root)
    work = root / 'target/fonts'
    work.mkdir(parents=True, exist_ok=True)
    files = {}
    # fontTools stamps head.modified with SOURCE_DATE_EPOCH when it is set.
    env = dict(os.environ, SOURCE_DATE_EPOCH='1767225600')  # 2026-01-01T00:00:00Z
    for name in SOURCES:
        path = download(name, work, upstream)
        if name.endswith(('.txt', '.md')):
            shutil.copyfile(path, out / name)
            files[name] = {'source': SOURCES[name][0], 'sha256': sha256(out / name)}
    for name, (source, location, unicodes) in FONTS.items():
        src = work / source
        if location:
            instanced = work / ('static-' + name)
            subprocess.run([sys.executable, '-m', 'fontTools.varLib.instancer', str(src), *location, '-q', '-o', str(instanced)], check=True, env=env)
            src = instanced
        subprocess.run([sys.executable, '-m', 'fontTools.subset', str(src), '--unicodes=' + unicodes, *KEEP,
                        '--output-file=' + str(out / name)], check=True, env=env)
        files[name] = {'source': SOURCES[source][0], 'source_sha256': SOURCES[source][1],
                       'instance': location, 'unicodes': unicodes, 'license': 'OFL-1.1', 'sha256': sha256(out / name)}
    # ChromaCheck's COLRv0 font, byte for byte as HarfBuzz's own tests ship it.
    colr = upstream / 'test/api/fonts/chromacheck-colr.ttf'
    shutil.copyfile(colr, out / 'chromacheck-colr.ttf')
    files['chromacheck-colr.ttf'] = {'source': 'harfbuzz 14.3.0 test/api/fonts/chromacheck-colr.ttf (from ' + CHROMACHECK + ')',
                                     'license': 'MIT', 'sha256': sha256(out / 'chromacheck-colr.ttf')}
    import fontTools
    (out / 'fonts.json').write_text(json.dumps({'fonttools': fontTools.version, 'files': files}, indent=2) + '\n', encoding='utf-8')
    print('wrote tests/fonts')


if __name__ == '__main__':
    main()
