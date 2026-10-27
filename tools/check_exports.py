#!/usr/bin/env python3
"""Audit the module surface against HarfBuzz's headers (the pinned release's).

- src/harfbuzz.cppm and src/ft.cppm are what tools/gen_exports.py writes:
  every type (typedef names and struct/union tags), enumerator and function
  of <hb.h>, <hb-ot.h>, <hb-aat.h> and their parts exported; every function of
  <hb-ft.h> exported by the .ft module; every class template of namespace hb
  in <hb-cplusplus.hh> exported.
- Every public header of src/ is wrapped or listed in tools/inventory.py's
  OUT_OF_SCOPE with the reason; every declaration a header makes only under
  an undefined macro (HB_EXPERIMENTAL_API) is listed with the reason.
- Every macro of the wrapped headers is mapped in src/macros.cppm, is a
  macro over an exported function of the same name, is kept as a macro in
  <mwrap/harfbuzz/macros.hpp> (listed below with the reason a function cannot
  be it), or is excluded below with a reason.
"""
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from inventory import fetch, surface, OUT_OF_SCOPE, CONDITIONAL, UMBRELLAS  # noqa: E402

SIDE_HEADER = {
    'HB_UNTAG': 'expands to four comma-separated arguments (for a printf-style call); no function returns that',
    'HB_VERSION_ATLEAST': 'also mapped as a constexpr function; the macro stays for a test in #if',
}

EXCLUDED = {
    'HB_EXTERN': 'a linkage annotation of declarations',
    'HB_BEGIN_DECLS': 'opens the extern "C" block of declarations',
    'HB_END_DECLS': 'closes the extern "C" block of declarations',
    'HB_DEPRECATED': 'a deprecation attribute of declarations (the exported functions keep theirs)',
    'HB_DEPRECATED_FOR': 'a deprecation attribute of declarations (the exported functions keep theirs)',
    'HB_DEFINE_VTABLE': "hb-cplusplus.hh's definition of an hb::vtable specialisation; it defines every one of this "
                        "library's object types (exported), the others belong to libraries this package does not build",
    'HB_H_IN': "the umbrella headers' permission for their parts to be included",
    'HB_OT_H_IN': "the umbrella headers' permission for their parts to be included",
    'HB_AAT_H_IN': "the umbrella headers' permission for their parts to be included",
}


def include_guards(src: Path, headers) -> set[str]:
    guards = set()
    for h in headers:
        text = (src / h).read_text(encoding='utf-8')
        for name in re.findall(r'^#ifndef (\w+)[ \t]*\n#define \1[ \t]*$', text, flags=re.M):
            guards.add(name)
    return guards


def mapped_names(text: str) -> set[str]:
    text = text[text.index('export module'):]
    names = set(re.findall(r'inline constexpr (?:[\w:&]+\s+)+?&?(\w+)\s*=', text))
    names |= set(re.findall(r'^constexpr\s+[\w:\*\s]+?\b(\w+)\s*\(', text, flags=re.M))
    return names


def main():
    root = Path(__file__).resolve().parents[1]
    problems = []
    generated = subprocess.run([sys.executable, str(root / 'tools/gen_exports.py'), '--check'], capture_output=True, text=True)
    if generated.returncode != 0:
        problems.append(generated.stderr.strip() or generated.stdout.strip())
    upstream = fetch(root)
    src = upstream / 'src'
    sf = surface(upstream)

    cppm = (root / 'src/harfbuzz.cppm').read_text(encoding='utf-8')
    exported = set(re.findall(r'export using ::(\w+);', cppm))
    missing = (set(sf.types) | set(sf.enumerators) | set(sf.functions)) - exported
    if missing:
        problems.append('src/harfbuzz.cppm lacks: ' + ', '.join(sorted(missing)))
    hb_ns = set(re.findall(r'^using hb::(\w+);', cppm, flags=re.M))
    if set(sf.cplusplus) - hb_ns:
        problems.append('namespace hb lacks: ' + ', '.join(sorted(set(sf.cplusplus) - hb_ns)))
    ft = set(re.findall(r'export using ::(\w+);', (root / 'src/ft.cppm').read_text(encoding='utf-8')))
    if set(sf.ft_functions) - ft:
        problems.append('src/ft.cppm lacks: ' + ', '.join(sorted(set(sf.ft_functions) - ft)))
    if exported & ft:
        problems.append('exported by both modules: ' + ', '.join(sorted(exported & ft)))

    wrapped = set(sf.headers) | set(sf.ft_headers) | {'hb-cplusplus.hh'}
    unaccounted = set(sf.public_headers) - wrapped - OUT_OF_SCOPE.keys()
    if unaccounted:
        problems.append('public headers neither wrapped nor out of scope: ' + ', '.join(sorted(unaccounted)))
    stale = OUT_OF_SCOPE.keys() - set(sf.public_headers)
    if stale:
        problems.append('out of scope but not in src/: ' + ', '.join(sorted(stale)))
    if OUT_OF_SCOPE.keys() & wrapped:
        problems.append('both wrapped and out of scope: ' + ', '.join(sorted(OUT_OF_SCOPE.keys() & wrapped)))
    for macro, names in sf.conditional.items():
        if macro not in CONDITIONAL:
            problems.append(f'declarations under {macro} without a reason: {", ".join(names)}')
        if set(names) & exported:
            problems.append(f'exported although declared only under {macro}: {", ".join(sorted(set(names) & exported))}')

    mapped = mapped_names((root / 'src/macros.cppm').read_text(encoding='utf-8'))
    guards = include_guards(src, list(wrapped))
    macros = set(sf.macros)
    over_functions = {m for m in macros if m in exported or m in ft}   # hb_color_get_red …
    unaccounted = macros - mapped - over_functions - SIDE_HEADER.keys() - EXCLUDED.keys() - guards
    if unaccounted:
        problems.append('macros neither mapped, function, side-header, excluded nor include guards: ' + ', '.join(sorted(unaccounted)))
    both = (EXCLUDED.keys() | guards) & mapped
    if both:
        problems.append('listed as unmapped yet mapped: ' + ', '.join(sorted(both)))
    stale = (SIDE_HEADER.keys() | EXCLUDED.keys()) - macros
    if stale:
        problems.append('listed but not a macro of the headers: ' + ', '.join(sorted(stale)))
    extra = mapped - macros
    if extra:
        problems.append('mapped but not a macro of the headers: ' + ', '.join(sorted(extra)))
    side = (root / 'include/mwrap/harfbuzz/macros.hpp').read_text(encoding='utf-8')
    for name in SIDE_HEADER:
        header = sf.macros[name]
        if not any(f'#include <{u}>' in side for u in UMBRELLAS if header in surface_closure(src, u)):
            problems.append(f'{name} ({header}) is not reachable through <mwrap/harfbuzz/macros.hpp>')
    if problems:
        sys.exit('\n'.join(problems))
    print(f'{len(sf.types)} types, {len(sf.enumerators)} enumerators, {len(sf.functions)} functions and hb::{{{", ".join(sf.cplusplus)}}} '
          f'exported by mcpplibs.mwrap.harfbuzz; {len(sf.ft_functions)} functions by mcpplibs.mwrap.harfbuzz.ft; '
          f'{len(wrapped)} headers wrapped, {len(OUT_OF_SCOPE)} out of scope; {len(macros)} macros: {len(macros & mapped)} mapped, '
          f'{len(over_functions)} over exported functions, {len(SIDE_HEADER)} kept as macros, {len(EXCLUDED)} excluded, '
          f'{len(macros & guards)} include guards; not declared without '
          + ', '.join(f'{m}: {", ".join(n)}' for m, n in sf.conditional.items()))


def surface_closure(src: Path, umbrella: str) -> set[str]:
    from inventory import closure
    return set(closure(src, (umbrella,)))


if __name__ == '__main__':
    main()
