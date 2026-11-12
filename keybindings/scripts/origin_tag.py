#!/usr/bin/env python3
"""Tag bindings from their cited source evidence against the pinned upstream base.

Identical cited files, function bodies, and pad mappings provide scoped upstream
source evidence. Changed or ambiguous evidence stays unverified unless an override
records a binding-level review. Matching code does not prove runtime reachability.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import re
import subprocess
import sys
import tomllib

CATALOG = Path(__file__).resolve().parent.parent
REPOSITORY = CATALOG.parent
DATA = CATALOG / 'data'
WORKTREE = (REPOSITORY / '.layerctl-worktree').resolve()
ORIGIN_TAGS = {'upstream', 'fork:nonlogical', 'origin:unverified'}


def mask(source):
    # Preserve offsets while excluding comments and literal contents from parsing.
    pattern = r'''//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*' '''.strip()
    return re.sub(pattern, lambda match: re.sub(r'[^\n]', ' ', match.group()), source)


def functions(source, symbol):
    if source is None:
        return []
    name = symbol if '::' in symbol and symbol + '(' in source else symbol.split('::')[-1]
    code = mask(source)
    pattern = r'\b' + re.escape(name) + r'\s*\([^;{}]*\)\s*(?:(?:const|override|final|noexcept)\s*)*(?:->\s*[\w:<>, &*]+\s*)?\{'
    found = []
    for match in re.finditer(pattern, code):
        depth, end = 1, match.end()
        while end < len(code) and depth:
            depth += (code[end] == '{') - (code[end] == '}')
            end += 1
        if not depth:
            found.append(source[match.start():end])
    return found


def definition_section(source, symbol):
    """Compare complete top-level method sections when preprocessor braces are ambiguous."""
    if source is None or '::' not in symbol:
        return None
    pattern = r'(?m)^[A-Za-z_][\w:<>, \t*&~]*\b[A-Za-z_]\w*(?:::\w+)+\s*\('
    methods = list(re.finditer(pattern, source))
    matches = [i for i, match in enumerate(methods) if re.search(r'\b' + re.escape(symbol) + r'\s*\(', match.group())]
    if len(matches) != 1 or matches[0] + 1 >= len(methods):
        return None
    i = matches[0]
    return source[methods[i].start():methods[i + 1].start()]


def grid_entry(source, symbol, key):
    if source is None or not isinstance(key, str):
        return None
    key_match = re.fullmatch(r'pad\[(\d+),(\d+)\]\.down', key)
    if not key_match:
        return None
    code = mask(source)
    start = re.search(r'\b' + re.escape(symbol) + r'\s*(?:\[[^\]]*\])+\s*=\s*\{', code)
    if not start:
        return None
    depth, end = 1, start.end()
    while end < len(code) and depth:
        depth += (code[end] == '{') - (code[end] == '}')
        end += 1
    if depth:
        return None
    body = code[start.end():end - 1]
    rows = re.findall(r'\{([^{}]*)\}', body)
    x, y = map(int, key_match.groups())
    if symbol in {'midiOrCVParamShortcuts', 'gateDrumParamShortcuts'}:
        if x != 11 or rows:
            return None
        cells = body.split(',')
    else:
        if symbol in {'modSourceShortcuts', 'modSourceShortcutsSecondLayer'}:
            x -= 14
        if x < 0 or x >= len(rows):
            return None
        cells = rows[x].split(',')
    return re.sub(r'\s+', '', cells[y]) if y < len(cells) else None


def reference_matches(source, old, new, key):
    if old is None or new is None:
        return False
    if old == new:
        return True
    if source['role'] == 'mapping':
        a = grid_entry(old, source['symbol'], key)
        b = grid_entry(new, source['symbol'], key)
        if a is not None and a == b and source.get('anchor', '') in b:
            return True
    a, b = functions(old, source['symbol']), functions(new, source['symbol'])
    if len(a) == len(b) == 1 and a == b:
        return True
    a = definition_section(old, source['symbol'])
    b = definition_section(new, source['symbol'])
    return a is not None and a == b


def classify(record, files, overrides):
    if record['id'] in overrides:
        return set(overrides[record['id']])
    if all(reference_matches(source, *files[source['file']], record['key']) for source in record['sources']):
        return {'upstream'}
    return {'origin:unverified'}


def load_overrides(base, source_tree):
    data = json.loads((DATA / 'origin-overrides.json').read_text())
    if data['upstream_commit'] != base or data['firmware_source_tree'] != source_tree:
        raise ValueError('review origin overrides for the current upstream commit and firmware source tree')
    overrides = {}
    for group in data['groups']:
        tags = set(group['tags'])
        if not tags or not tags <= {'upstream', 'fork:nonlogical'} or not group['evidence']:
            raise ValueError('invalid reviewed origin override')
        for ident in group['ids']:
            if ident in overrides:
                raise ValueError(f'duplicate origin override: {ident}')
            overrides[ident] = tags
    return overrides


def rebuild():
    manifest = tomllib.loads((REPOSITORY / 'layers/0001-base/layer.toml').read_text())
    base = next(op['git-clone']['commit'] for op in manifest['operation'] if 'git-clone' in op)
    git = ['git', '-C', str(WORKTREE)]
    subprocess.run(git + ['cat-file', '-e', base + '^{commit}'], check=True)
    if subprocess.check_output(git + ['status', '--porcelain', '--untracked-files=no'], text=True).strip():
        raise ValueError('firmware worktree has tracked changes; commit or use a clean projection first')
    path = DATA / 'bindings.jsonl'
    rows = [json.loads(line) for line in path.read_text().splitlines()]
    source_tree = subprocess.check_output(git + ['rev-parse', 'HEAD:src'], text=True).strip()
    overrides = load_overrides(base, source_tree)
    unknown = set(overrides) - {row['id'] for row in rows}
    if unknown:
        raise ValueError('override IDs not found: ' + ', '.join(sorted(unknown)))
    files = {}
    for name in sorted({source['file'] for row in rows for source in row['sources']}):
        target = (WORKTREE / name).resolve()
        if not target.is_relative_to(WORKTREE) or not target.is_file():
            raise ValueError(f'invalid source path: {name}')
        previous = subprocess.run(git + ['show', f'{base}:{name}'], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        old = previous.stdout.decode('utf-8', errors='replace') if previous.returncode == 0 else None
        files[name] = (old, target.read_text(errors='replace'))
    counts = Counter()
    for row in rows:
        origins = classify(row, files, overrides)
        row['tags'] = [tag for tag in row['tags'] if tag not in ORIGIN_TAGS] + sorted(origins)
        counts.update(origins)
    return rows, counts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write', action='store_true', help='update the catalog in place')
    args = parser.parse_args()
    try:
        rows, counts = rebuild()
    except (ValueError, subprocess.CalledProcessError) as error:
        parser.error(str(error))
    output = ''.join(json.dumps(row, ensure_ascii=False, separators=(',', ':')) + '\n' for row in rows)
    if args.write:
        (DATA / 'bindings.jsonl').write_text(output)
    else:
        sys.stdout.write(output)
    print('Origin tags: ' + ', '.join(f'{name}={counts[name]}' for name in sorted(ORIGIN_TAGS)), file=sys.stderr)


if __name__ == '__main__':
    main()
