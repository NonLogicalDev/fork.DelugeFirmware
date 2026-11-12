#!/usr/bin/env python3
"""Color layerctl's Mermaid graph by firmware area.

Usage: layerctl graph --format mermaid | python3 scripts/graph_layers_color.py [--write]
"""
import argparse
from pathlib import Path
import re
import sys

# Slugs remain stable when layer numbers change. A new layer must be assigned an area.
AREAS = {
    "foundation": ("Foundation", "#e5e7eb", "#4b5563", {
        "base", "reject-unsupported-song-compatibility-versions",
    }),
    "keyboard": ("Keyboard and scales", "#ede9fe", "#6d28d9", {
        "add-keyboard-view-chord-detection", "add-shared-song-scale-menu",
        "make-preview-only-note-input-release-order-safe",
    }),
    "slicer": ("Slicer and Kit", "#ffedd5", "#c2410c", {
        "add-manual-slicer-kit-creation-option", "improve-manual-slicer-controls",
        "copy-and-paste-kit-rows", "reuse-a-kit-from-manual-slice",
        "make-kit-row-reordering-undoable", "add-kit-choke-groups",
    }),
    "waveform": ("Audio and waveforms", "#ccfbf1", "#0f766e", {
        "fix-repeated-audio-looper-recording", "keep-waveform-rendering-valid-for-long-samples",
        "add-efficient-oled-waveform-displays", "add-compact-oled-clip-progress-ruler",
        "audition-current-waveform-editor-kit-row", "add-recoverable-audio-clip-start-editing",
        "improve-waveform-editor-entry-and-bound-selection", "correct-slicer-playhead-source",
        "improve-waveform-playhead-visibility",
    }),
    "song": ("Song and arrangement", "#dbeafe", "#1d4ed8", {
        "add-song-grid-track-column-glow", "synchronize-grid-and-audio-clip-colors",
    }),
    "control": ("Playback and MIDI", "#dcfce7", "#15803d", {
        "add-panic-action", "add-midi-sustain-pedal-feature-flag", "add-external-step-midi-clips",
    }),
    "sound": ("Sound and expression", "#fce7f3", "#be185d", {
        "add-velocity-drums-velocity-profiles", "add-precise-envelope-time-controls",
    }),
}


def color_graph(graph):
    if not graph.startswith("flowchart "):
        raise ValueError("expected a Mermaid flowchart from layerctl")
    members = {area: [] for area in AREAS}
    assigned = {}
    for area, (_, _, _, slugs) in AREAS.items():
        for slug in slugs:
            if slug in assigned:
                raise ValueError(f"duplicate area for {slug}")
            assigned[slug] = area
    for node, slug in re.findall(r'^\s*(n\d+)\["\d{4}-([^"]+)"\]\s*$', graph, re.M):
        if slug not in assigned:
            raise ValueError(f"assign an area to layer {slug}")
        members[assigned[slug]].append(node)
    if not any(members.values()):
        raise ValueError("no layer nodes found")
    lines = [graph.rstrip(), '  subgraph legend["Firmware areas"]']
    for area, (title, _, _, _) in AREAS.items():
        lines.append(f'    legend_{area}["{title}"]')
    lines.append("  end")
    for area, (_, fill, stroke, _) in AREAS.items():
        lines.append(f"  classDef {area} fill:{fill},stroke:{stroke},color:#111827")
        lines.append(f"  class {','.join(members[area] + ['legend_' + area])} {area}")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="replace the README diagram")
    args = parser.parse_args()
    try:
        graph = color_graph(sys.stdin.read())
    except ValueError as error:
        parser.error(str(error))
    if args.write:
        readme = Path(__file__).resolve().parents[1] / "README.md"
        text, count = re.subn(r"(?s)(```mermaid\n).*?(\n```)", lambda m: m.group(1) + graph.rstrip() + m.group(2), readme.read_text(), count=1)
        if count != 1:
            parser.error("README must contain a Mermaid diagram")
        readme.write_text(text)
    else:
        sys.stdout.write(graph)


if __name__ == "__main__":
    main()
