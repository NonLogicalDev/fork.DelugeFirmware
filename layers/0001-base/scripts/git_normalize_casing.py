#!/usr/bin/env python3
"""Normalize legacy KORG directory names in the prepared Git index."""

import os
import subprocess
import sys


RENAMES = (
    (b"contrib/sd_card/MIDI/KORG/", b"contrib/sd_card/MIDI/Korg/"),
    (b"contrib/sd_card/MIDI_DEVICES/DEFINITION/KORG/", b"contrib/sd_card/MIDI_DEVICES/DEFINITION/Korg/"),
)


def main():
    listing = subprocess.check_output(["git", "ls-files", "--stage", "-z"])
    entries = []
    for record in listing.split(b"\0"):
        if not record:
            continue
        metadata, path = record.split(b"\t", 1)
        mode, oid, stage = metadata.split(b" ")
        if stage != b"0":
            raise SystemExit("Cannot normalize an index containing unresolved entries")
        for old, new in RENAMES:
            if path.startswith(old):
                entries.append((mode, oid, path, new + path[len(old):]))
                break

    paths = {record.split(b"\t", 1)[1] for record in listing.split(b"\0") if record}
    removed = {old for _, _, old, _ in entries}
    destinations = [new for _, _, _, new in entries]
    remaining = paths - removed
    # Reject ASCII case collisions before changing an index used on macOS.
    folded_destinations = [path.lower() for path in destinations]
    folded_remaining = {path.lower() for path in remaining}
    if len(set(folded_destinations)) != len(destinations) or any(
        path in folded_remaining for path in folded_destinations
    ):
        raise SystemExit("Case normalization would collide with an existing index entry")

    if entries:
        zero = b"0" * len(os.environ["LAYERCTL_SOURCE_COMMIT"])
        changes = b"".join(b"0 " + zero + b"\t" + old + b"\0" for _, _, old, _ in entries)
        changes += b"".join(mode + b" " + oid + b"\t" + new + b"\0" for mode, oid, _, new in entries)
        subprocess.run(["git", "-c", "core.ignorecase=false", "update-index", "-z", "--index-info"],
                       input=changes, check=True)

    subprocess.run(["git", "write-tree"], check=True, stdout=sys.stdout)


if __name__ == "__main__":
    main()
