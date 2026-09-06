# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///
"""Reconcile reviewed handlers and detect source drift; never prove reachability.

The inventory is name-based. A reviewed ledger entry records a human source
review, not a machine proof that every branch was catalogued. Gesture overlaps
are candidates for review, not necessarily conflicts: dispatch and `when` matter.
"""

import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import sys

from inventory import candidates

DIRECTORY = Path(__file__).resolve().parent
ROOT = DIRECTORY.parents[1]


def read_rows(path):
    return [json.loads(line) for line in path.read_text().splitlines()]


def handler_key(row):
    # Inline definitions lack a class qualifier in the scanner. File + method
    # name is therefore the reconciliation unit, not an overload identifier.
    return row['file'], row['symbol'].split('::')[-1]


def normalized_gesture(key):
    if isinstance(key, str):
        key = {'trigger': key}
    if 'sequence' in key:
        return dict(key, sequence=[normalized_gesture(part) for part in key['sequence']])
    return {'trigger': key['trigger'], 'hold': sorted(key.get('hold', []))}


def gesture_key(key):
    return json.dumps(normalized_gesture(key), sort_keys=True)


def source_files(value):
    if isinstance(value, dict):
        if 'file' in value and 'symbol' in value:
            yield value['file']
        for child in value.values():
            yield from source_files(child)
    elif isinstance(value, list):
        for child in value:
            yield from source_files(child)


def audit():
    bindings = read_rows(DIRECTORY / 'bindings.jsonl')
    ledger = read_rows(DIRECTORY / 'coverage.jsonl')
    snapshot = json.loads((DIRECTORY / 'source-snapshot.json').read_text())
    controls = json.loads((DIRECTORY / 'controls.json').read_text())
    ids = {row['id'] for row in bindings}
    reviewed = {handler_key(row) for row in ledger if row['status'] in {'reviewed', 'excluded'}}
    discovered = list(candidates())
    missing = [row for row in discovered if handler_key(row) not in reviewed]
    invalid = []
    changed = set()
    hashes = {}
    referenced_files = {source['file'] for row in bindings for source in row['sources']} | set(source_files(controls))
    for file in sorted(referenced_files):
        path = (ROOT / file).resolve()
        if not path.is_relative_to(ROOT) or not path.is_file():
            invalid.append({'file': file, 'error': 'invalid binding source path'})
            continue
        hashes[file] = hashlib.sha256(path.read_bytes()).hexdigest()
        if file not in snapshot['files']:
            invalid.append({'file': file, 'error': 'binding source missing from snapshot'})
        elif hashes[file] != snapshot['files'][file]:
            changed.add(file)
    for row in ledger:
        file = row['file']
        path = (ROOT / file).resolve()
        if not path.is_relative_to(ROOT) or not path.is_file():
            invalid.append({'file': file, 'error': 'invalid source path'})
            continue
        if file not in hashes:
            hashes[file] = hashlib.sha256(path.read_bytes()).hexdigest()
        if hashes[file] != row.get('fileSha256'):
            changed.add(file)
        if not row.get('reason'):
            invalid.append({'file': file, 'symbol': row['symbol'], 'error': 'missing review reason'})
        if row['status'] not in {'reviewed', 'excluded'}:
            invalid.append({'file': file, 'symbol': row['symbol'], 'error': 'unresolved review status'})
        if row.get('gaps'):
            invalid.append({'file': file, 'symbol': row['symbol'], 'error': 'unresolved coverage gaps', 'gaps': row['gaps']})
        unknown = sorted(set(row.get('bindingIds', [])) - ids)
        if unknown:
            invalid.append({'file': file, 'symbol': row['symbol'], 'unknownBindingIds': unknown})
    groups = defaultdict(list)
    for row in bindings:
        signature = (json.dumps(row['context'], sort_keys=True), gesture_key(row['key']))
        groups[signature].append(row['id'])
    overlaps = [group for group in groups.values() if len(group) > 1]
    return {
        'bindings': len(bindings),
        'inventoryCandidates': len(discovered),
        'reviewedLedgerRows': len(ledger),
        'sourceFiles': len(hashes),
        'unaccountedCandidates': missing,
        'changedSourceFiles': sorted(changed),
        'invalidCoverage': invalid,
        'sameContextGestureCandidates': overlaps,
        'limits': 'Name-based discovery and file-level drift checks. Conditions, inherited dispatch, overload coverage, pad-role overlap and physical behavior require review; overlap groups are not proven conflicts.',
    }


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--json', action='store_true', help='include all overlap candidates as JSON')
    args = parser.parse_args()
    report = audit()
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(f"{report['bindings']} bindings; {report['inventoryCandidates']} discovered definitions; {report['reviewedLedgerRows']} ledger rows across {report['sourceFiles']} files.")
        for field in ['unaccountedCandidates', 'changedSourceFiles', 'invalidCoverage']:
            print(f'{field}: {len(report[field])}')
            for item in report[field]:
                print(json.dumps(item))
        print(f"Same-context/gesture groups to review: {len(report['sameContextGestureCandidates'])}. Use --json for IDs; different conditions may make them intentional.")
        print(report['limits'])
    sys.exit(bool(report['unaccountedCandidates'] or report['changedSourceFiles'] or report['invalidCoverage']))
