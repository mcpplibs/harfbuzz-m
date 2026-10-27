#!/usr/bin/env python3
"""Run HarfBuzz's own shaping suites on examples/hb-shape, a port of upstream's
util/hb-shape written against import mcpplibs.mwrap.harfbuzz.

The release tarball is downloaded once into target/upstream and checked
against upstream/documents.json; its files are used unchanged. Each suite of
test/shape/meson.build (in-house, aots, text-rendering-tests) runs exactly as
`meson test --suite shape` runs it: upstream's test/shape/run-tests.py, with
the .tests files its data/<suite>/meson.build lists, and the hb-shape built
here in place of upstream's. run-tests.py tests every font-funcs
implementation the library reports (ot and ft: compat.harfbuzz is built with
FreeType) and skips what upstream skips itself: lines for shapers this build
does not have (CoreText, DirectWrite, Uniscribe, harfrust, kbts), and system
fonts that are absent or of another version (absolute paths; always skipped
on Windows). Every test that runs must pass.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from inventory import fetch  # noqa: E402

SUITES = ('in-house', 'aots', 'text-rendering-tests')
MINIMUM_PASSES = 5000  # 14.3.0 passes 6,8xx here: a guard against running nothing


def host(example: Path, mcpp: str, toolchain: str | None) -> Path:
    command = [mcpp, 'build', '--profile', 'release'] + (['--toolchain', toolchain] if toolchain else [])
    print('+', ' '.join(command), flush=True)
    subprocess.run(command, cwd=example, check=True)
    name = 'hb-shape.exe' if os.name == 'nt' else 'hb-shape'
    found = sorted((p for p in (example / 'target').rglob(name) if p.parent.name == 'bin'), key=lambda p: p.stat().st_mtime)
    if not found:
        sys.exit('examples/hb-shape was not built')
    return found[-1]


def suite_files(upstream: Path, suite: str) -> list[Path]:
    meson = (upstream / 'test/shape/data' / suite / 'meson.build').read_text(encoding='utf-8')
    body = meson[meson.index('['):meson.index(']') + 1]
    names = re.findall(r"'([^']+\.tests)'", body)
    return [upstream / 'test/shape/data' / suite / 'tests' / n for n in names]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mcpp', default='mcpp')
    parser.add_argument('--toolchain')
    parser.add_argument('-v', '--verbose', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    upstream = fetch(root)
    hb_shape = host(root / 'examples/hb-shape', args.mcpp, args.toolchain)
    total = {'pass': 0, 'fail': 0, 'skip': 0}
    failed_suites = []
    for suite in SUITES:
        files = suite_files(upstream, suite)
        command = [sys.executable, str(upstream / 'test/shape/run-tests.py'), str(hb_shape)] + [str(f) for f in files]
        run = subprocess.run(command, capture_output=True, text=True, encoding='utf-8', errors='replace',
                             cwd=upstream, env=dict(os.environ, LC_ALL='C'))
        out = run.stdout
        passes = len(re.findall(r'^ok \d+', out, flags=re.M))
        fails = len(re.findall(r'^not ok \d+', out, flags=re.M))
        summary = re.search(r'^# (\d+) tests passed; (\d+) failed; (\d+) skipped\.', out, flags=re.M)
        skips = int(summary.group(3)) if summary else 0
        print(f'{suite}: {len(files)} files, {passes} passed, {fails} failed, {skips} skipped (exit {run.returncode})', flush=True)
        skipped = {}
        for line in out.splitlines():
            if line.startswith('not ok') or (args.verbose and line.startswith('#')):
                print('  ' + line)
            m = re.match(r'# (.*) not found, skip\.', line) or re.match(r'# Different version of (.*) found;', line)
            if m:
                skipped[m.group(1)] = skipped.get(m.group(1), 0) + 1
        for font, count in sorted(skipped.items()):
            print(f'  skipped {count}: {font} (a system font, absent or of another version)')
        if fails or run.returncode != 0 or not summary:
            failed_suites.append(suite)
            print(run.stderr[-6000:])
        total['pass'] += passes
        total['fail'] += fails
        total['skip'] += skips
    print(f"shape suites: {total['pass']} passed, {total['fail']} failed, {total['skip']} skipped")
    if failed_suites or total['pass'] < MINIMUM_PASSES:
        sys.exit(1)


if __name__ == '__main__':
    main()
