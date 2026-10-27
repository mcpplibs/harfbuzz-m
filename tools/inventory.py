#!/usr/bin/env python3
"""The public surface of HarfBuzz's installed C API, from the pinned release.

Shared by tools/gen_exports.py (which writes the export lists) and
tools/check_exports.py (which audits the modules against them). The headers
come from the upstream tarball, downloaded once into target/upstream and
checked against upstream/documents.json: the same bytes compat.harfbuzz
builds.

Scope. compat.harfbuzz compiles upstream's amalgamation src/harfbuzz.cc with
HAVE_FREETYPE and nothing else, so the installed API is the closure of
<hb.h>, <hb-ot.h> and <hb-aat.h>, <hb-ft.h> (the FreeType integration) and
the C++ helpers of <hb-cplusplus.hh>. Every other public header of src/ is
listed in OUT_OF_SCOPE with the reason it is not part of this library.
"""
from dataclasses import dataclass, field
import hashlib
import json
from pathlib import Path
import re
import sys
import tarfile
import urllib.request

UMBRELLAS = ('hb.h', 'hb-ot.h', 'hb-aat.h')   # module mcpplibs.mwrap.harfbuzz
FT_HEADERS = ('hb-ft.h',)                     # module mcpplibs.mwrap.harfbuzz.ft
CPLUSPLUS = 'hb-cplusplus.hh'                 # namespace hb, in the root module

# Every other public header of src/, with the reason it is not wrapped.
OUT_OF_SCOPE = {
    'hb-subset.h': 'libharfbuzz-subset (src/harfbuzz-subset.cc), which compat.harfbuzz does not build',
    'hb-subset-serialize.h': 'libharfbuzz-subset, which compat.harfbuzz does not build',
    'hb-subset-depend.h': 'libharfbuzz-subset, which compat.harfbuzz does not build',
    'hb-raster.h': 'libharfbuzz-raster, a separate library compat.harfbuzz does not build',
    'hb-vector.h': 'libharfbuzz-vector, a separate library compat.harfbuzz does not build',
    'hb-gpu.h': 'libharfbuzz-gpu, a separate library compat.harfbuzz does not build',
    'hb-cairo.h': 'libharfbuzz-cairo, which needs cairo; not built',
    'hb-icu.h': 'libharfbuzz-icu, which needs ICU; not built',
    'hb-glib.h': 'the GLib integration; compat.harfbuzz does not define HAVE_GLIB',
    'hb-gobject.h': 'libharfbuzz-gobject (GObject introspection); not built',
    'hb-gobject-structs.h': 'libharfbuzz-gobject (GObject introspection); not built',
    'hb-coretext.h': 'the CoreText backend; compat.harfbuzz does not define HAVE_CORETEXT',
    'hb-directwrite.h': 'the DirectWrite backend; compat.harfbuzz does not define HAVE_DIRECTWRITE',
    'hb-gdi.h': 'the GDI integration; compat.harfbuzz does not define HAVE_GDI',
    'hb-uniscribe.h': 'the Uniscribe shaper; compat.harfbuzz does not define HAVE_UNISCRIBE',
    'hb-graphite2.h': 'the Graphite2 shaper; compat.harfbuzz does not define HAVE_GRAPHITE2',
    'hb-fontations.h': 'the Rust fontations backend; not built',
    'hb-wasm-api.h': 'the experimental WebAssembly shaper; not installed upstream (hb_wasm_headers is empty), HAVE_WASM undefined',
}

# Declarations the headers make only under a macro this build leaves undefined.
CONDITIONAL = {
    'HB_EXPERIMENTAL_API': 'experimental API: declared and compiled only with HB_EXPERIMENTAL_API, which compat.harfbuzz does not define',
}


def fetch(root: Path) -> Path:
    pins = json.loads((root / 'upstream/documents.json').read_text(encoding='utf-8'))
    cache = root / 'target/upstream'
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / Path(pins['upstream']).name
    if not archive.exists() or hashlib.sha256(archive.read_bytes()).hexdigest() != pins['upstream_sha256']:
        print('+ fetch', pins['upstream'], flush=True)
        with urllib.request.urlopen(pins['upstream'], timeout=300) as response:
            archive.write_bytes(response.read())
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    if digest != pins['upstream_sha256']:
        sys.exit(f'upstream archive checksum {digest} != {pins["upstream_sha256"]}')
    with tarfile.open(archive) as tar:
        top = tar.getmembers()[0].name.split('/')[0]
        if not (cache / top / 'src/hb.h').exists():
            tar.extractall(cache, filter='data')
    return cache / top


@dataclass
class Surface:
    headers: list[str] = field(default_factory=list)        # the wrapped headers, root module
    ft_headers: list[str] = field(default_factory=list)
    types: list[str] = field(default_factory=list)          # typedef names and struct/union tags
    enumerators: list[str] = field(default_factory=list)
    functions: list[str] = field(default_factory=list)
    ft_functions: list[str] = field(default_factory=list)
    conditional: dict[str, list[str]] = field(default_factory=dict)   # macro -> functions it guards
    macros: dict[str, str] = field(default_factory=dict)    # name -> header
    deprecated_macros: set[str] = field(default_factory=set)
    cplusplus: list[str] = field(default_factory=list)      # class templates of namespace hb
    public_headers: list[str] = field(default_factory=list)  # every hb*.h of src/, and hb-cplusplus.hh


def strip_comments(text: str) -> str:
    text = re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def drop_if0(text: str) -> str:
    return re.sub(r'^#if 0\b.*?^#endif[^\n]*', '', text, flags=re.S | re.M)


def blocks(text: str, macro: str) -> tuple[str, str]:
    """(text without the `#ifdef macro` blocks, the blocks' text)."""
    pattern = re.compile(r'^#ifdef ' + macro + r'\b(.*?)^#endif[^\n]*', flags=re.S | re.M)
    inside = ''.join(m.group(1) for m in pattern.finditer(text))
    return pattern.sub('', text), inside


def no_preprocessor(text: str) -> str:
    return re.sub(r'^[ \t]*#(?:\\\n|[^\n])*', '', text, flags=re.M)


def normalise(text: str) -> str:
    # Attribute and linkage annotations hide a declaration's shape.
    text = re.sub(r'\bHB_DEPRECATED_FOR\s*\([^)]*\)', ' ', text)
    text = re.sub(r'\bHB_DEPRECATED\b', ' ', text)
    return re.sub(r'\bHB_(BEGIN|END)_DECLS\b', ' ', text)


def includes(src: Path, header: str) -> list[str]:
    return re.findall(r'^#include\s+"([^"]+)"', drop_if0((src / header).read_text(encoding='utf-8')), flags=re.M)


def closure(src: Path, roots) -> list[str]:
    seen, order = set(), []

    def visit(h):
        if h in seen:
            return
        seen.add(h)
        for inc in includes(src, h):
            visit(inc)
        order.append(h)
    for r in roots:
        visit(r)
    return order


def split_top(body: str) -> list[str]:
    items, depth, cur = [], 0, ''
    for ch in body:
        if ch in '([{':
            depth += 1
        elif ch in ')]}':
            depth -= 1
        if ch == ',' and depth == 0:
            items.append(cur)
            cur = ''
        else:
            cur += ch
    items.append(cur)
    return [i.strip() for i in items if i.strip()]


def statements(text: str):
    """Top-level `;`-terminated statements (braces nest)."""
    depth, cur = 0, ''
    for ch in text:
        cur += ch
        if ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
        elif ch == ';' and depth == 0:
            yield ' '.join(cur.split())
            cur = ''


def declarations(text: str, s: Surface, functions: list[str]):
    text = normalise(no_preprocessor(strip_comments(drop_if0(text))))
    for st in statements(text):
        if st.startswith('HB_EXTERN'):
            m = re.search(r'\b(\w+)\s*\(', st)
            functions.append(m.group(1))
        elif st.startswith('typedef'):
            m = re.match(r'typedef\s+(?:const\s+)?(enum|struct|union)?\s*(\w+)?\s*(\{.*\})?\s*(.*);$', st)
            kind, tag, body, rest = m.groups()
            fp = re.search(r'\(\s*\*\s*(\w+)\s*\)\s*\(', st)
            if fp:
                name = fp.group(1)
            else:
                name = re.findall(r'(\w+)\s*(?:\[[^\]]*\])?\s*$', rest)[0]
            s.types.append(name)
            if kind in ('struct', 'union') and tag and tag != name:
                s.types.append(tag)     # _hb_var_int_t, hb_language_impl_t …
            if kind == 'enum' and body:
                for item in split_top(body[1:-1]):
                    s.enumerators.append(re.match(r'(\w+)', item).group(1))
        elif m := re.match(r'(struct|union)\s+(\w+)\s*\{', st):
            s.types.append(m.group(2))  # the definition of a typedef'd tag (hb_color_line_t)
        elif re.match(r'enum\b', st):
            sys.exit(f'an untypedef\'d enum the inventory does not model: {st[:80]}')


def surface(upstream: Path) -> Surface:
    src = upstream / 'src'
    s = Surface()
    s.headers = closure(src, UMBRELLAS)
    s.ft_headers = [h for h in closure(src, FT_HEADERS) if h not in s.headers]
    for header in s.headers + s.ft_headers:
        raw = (src / header).read_text(encoding='utf-8')
        for macro in CONDITIONAL:
            raw, inside = blocks(raw, macro)
            if inside:
                found = []
                declarations(inside, Surface(), found)
                s.conditional.setdefault(macro, []).extend(found)
        declarations(raw, s, s.functions if header in s.headers else s.ft_functions)
        deprecated = header.endswith('-deprecated.h')
        for name in re.findall(r'^\s*#\s*define\s+(\w+)', drop_if0((src / header).read_text(encoding='utf-8')), flags=re.M):
            s.macros.setdefault(name, header)
            if deprecated:
                s.deprecated_macros.add(name)
    cpp = (src / CPLUSPLUS).read_text(encoding='utf-8')
    for name in re.findall(r'^#\s*define\s+(\w+)', cpp, flags=re.M):
        s.macros.setdefault(name, CPLUSPLUS)
    hb_ns = cpp[cpp.index('namespace hb {'):cpp.index('} // namespace hb')]
    s.cplusplus = sorted(set(re.findall(r"\bstruct\s+(\w+)\b", no_preprocessor(hb_ns))))
    # hb-common.h's own <stdint.h> substitutes for pre-2010 MSVC are not HarfBuzz's.
    s.types = sorted({t for t in s.types if t.startswith(('hb_', '_hb_'))})
    s.functions = sorted(set(s.functions))
    s.ft_functions = sorted(set(s.ft_functions))
    # The public headers: every C header of src/ (the .hh are internal, but for
    # hb-cplusplus.hh, which meson installs with them).
    s.public_headers = sorted([p.name for p in src.glob('hb*.h')] + [CPLUSPLUS])
    return s


if __name__ == '__main__':
    root = Path(__file__).resolve().parents[1]
    sf = surface(fetch(root))
    print(f'{len(sf.headers)} headers in the root module, {len(sf.ft_headers)} in .ft')
    print(f'{len(sf.types)} types, {len(sf.enumerators)} enumerators, {len(sf.functions)} functions, '
          f'{len(sf.ft_functions)} hb-ft functions, {len(sf.macros)} macros, hb:: {", ".join(sf.cplusplus)}')
    for macro, names in sf.conditional.items():
        print(f'under {macro}: {", ".join(names)}')
    print('types:', ' '.join(sf.types))
    print('public headers:', ' '.join(sf.public_headers))
