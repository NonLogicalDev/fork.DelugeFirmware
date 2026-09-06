# /// script
# requires-python = ">=3.11"
# dependencies = ["jsonschema==4.26.0"]
# ///
"""Validate JSONL records and textual source references without changing files.

This checks structure and source-reference existence, not C++ reachability,
predicate truth, source freshness, or hardware behavior.
"""

import argparse
import json
from pathlib import Path
import re
import sys

from jsonschema import Draft202012Validator

DIRECTORY = Path(__file__).resolve().parent
REPOSITORY = DIRECTORY.parent.parent


def key_controls(key):
    if isinstance(key, str):
        yield key.rsplit('.', 1)
        return
    if 'trigger' in key:
        yield key['trigger'].rsplit('.', 1)
    for held in key.get('hold', []):
        yield held, 'hold'
    for part in key.get('sequence', []):
        yield from key_controls(part)
    for cancel in key.get('cancelOn', []):
        yield cancel['trigger'].rsplit('.', 1)
        for excluded in cancel.get('except', []):
            yield excluded, 'identity'


def control_family(name):
    # Parameters may be literal coordinates or state-bound variables. Their
    # meaning lives in controls.json, not a blanket coordinate range check.
    return name.split('[', 1)[0]


def validate(path: Path) -> list[str]:
    schema = json.loads((DIRECTORY / "binding.schema.json").read_text())
    Draft202012Validator.check_schema(schema)
    validator = Draft202012Validator(schema)
    errors = []
    registry = json.loads((DIRECTORY / 'controls.json').read_text())
    controls = registry['controls']
    seen = set()
    lines = path.read_text().splitlines()
    if not lines:
        errors.append("catalog is empty")
    for number, line in enumerate(lines, 1):
        prefix = f"{path}:{number}"
        try:
            record = json.loads(line)
        except ValueError as error:
            errors.append(f"{prefix}: {error}")
            continue
        invalid = list(validator.iter_errors(record))
        if invalid:
            errors.extend(f"{prefix}: {error.json_path}: {error.message}" for error in invalid)
            continue
        if record["id"] in seen:
            errors.append(f'{prefix}: duplicate id {record["id"]}')
        seen.add(record["id"])
        for name, event in key_controls(record['key']):
            matches = [control for control in controls if control_family(control['id']) == control_family(name)]
            if not matches:
                errors.append(f'{prefix}: unknown control {name}')
            elif event == 'hold' and not any(control['holdable'] for control in matches):
                errors.append(f'{prefix}: control is not holdable: {name}')
            elif event not in {'hold', 'identity'} and not any(event in control['events'] for control in matches):
                errors.append(f'{prefix}: unsupported event {name}.{event}')
        for source in record["sources"]:
            target = (REPOSITORY / source["file"]).resolve()
            if not target.is_relative_to(REPOSITORY) or not target.is_file():
                errors.append(f'{prefix}: invalid source file {source["file"]}')
                continue
            text = target.read_text()
            # Namespace-qualified functions may be defined inside a namespace block.
            symbol = source["symbol"].split("::")[-1]
            if not re.search(r"\b" + re.escape(symbol) + r"\b", text):
                errors.append(f'{prefix}: missing symbol token {source["symbol"]}')
            if source.get("anchor") and source["anchor"] not in text:
                errors.append(f'{prefix}: missing anchor {source["anchor"]}')
    return errors


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("catalog", nargs="?", type=Path, default=DIRECTORY / "bindings.jsonl")
    args = parser.parse_args()
    failures = validate(args.catalog)
    if failures:
        print("\n".join(failures), file=sys.stderr)
        sys.exit(1)
    print(f"Validated {len(args.catalog.read_text().splitlines())} records: schema, unique IDs, control families/events, source tokens and anchors.")
