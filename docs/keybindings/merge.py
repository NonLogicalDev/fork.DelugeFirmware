# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///
"""Print an apply_patch patch merging reviewed worker shards into the catalog.

Reads only. Existing IDs are updated in place, new IDs appended. Missing shard
entries are not deleted. The owner reviews and applies the patch separately.
"""

import argparse
import json
from pathlib import Path

DIRECTORY = Path(__file__).resolve().parent


def read_rows(path):
    records = {}
    for line in path.read_text().splitlines():
        row = json.loads(line)
        if row["id"] in records:
            raise ValueError(f"duplicate ID in {path}: {row['id']}")
        records[row["id"]] = line
    return records


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("shards", nargs="+", type=Path)
    parser.add_argument("--limit", type=int, default=100)
    args = parser.parse_args()
    if args.limit < 1:
        parser.error("--limit must be positive")
    target = DIRECTORY / "bindings.jsonl"
    current = read_rows(target)
    incoming = {}
    for path in args.shards:
        for key, line in read_rows(path).items():
            if key in incoming:
                raise ValueError(f"ID appears in multiple shards: {key}")
            incoming[key] = line
    updates = {key: line for key, line in incoming.items() if current.get(key) != line}
    updates = dict(list(updates.items())[:args.limit])
    if updates:
        print(f"*** Begin Patch\n*** Update File: {target}")
        last = next(reversed(current))
        for key, line in updates.items():
            if key in current and key != last:
                print(f"@@\n-{current[key]}\n+{line}")
        additions = [line for key, line in updates.items() if key not in current]
        if last in updates or additions:
            print("@@")
            if last in updates:
                print(f"-{current[last]}\n+{updates[last]}")
            else:
                print(" " + current[last])
            for line in additions:
                print("+" + line)
        print("*** End Patch")
