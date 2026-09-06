# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///
"""Print source input-handler candidates as JSONL; never claim branch coverage.

Includes inline header definitions and source files. The name-based scan is a
discovery aid, not a C++ parser. Reviewed coverage remains in coverage.jsonl.
"""

import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
METHOD = re.compile(
    r"\b(?P<symbol>(?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)\s*"
    r"\([^;{}]*\)\s*(?:(?:const|override|final|noexcept)\s*)*\{"
)
INPUT_NAME = re.compile(
    r"(?:buttonAction|padAction|EncoderAction|EncoderButtonAction|modButtonAction|"
    r"selectButtonPress|patchingSourceShortcutPress|learnKnob|learnNote|learnCC|"
    r"learnProgramChange|unlearnAction|handlePad|evaluatePads|handle.*Encoder|"
    r"timerCallback|exitUI|playButtonPressed|recordButtonPressed|interpretEncoders|"
    r"readButtonsAndPads|potentialShortcutPadAction|padPressAction|buttonPressAction)$"
)


def candidates():
    for file in sorted((ROOT / "src/deluge").rglob("*")):
        if file.suffix not in {".cpp", ".h", ".hpp"}:
            continue
        original = file.read_text()
        # Preserve offsets and newlines while excluding comments and string literals.
        text = re.sub(
            r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"',
            lambda match: re.sub(r"[^\n]", " ", match.group()),
            original,
        )
        for match in METHOD.finditer(text):
            symbol = match["symbol"]
            if not INPUT_NAME.search(symbol.split("::")[-1]):
                continue
            line = original.count("\n", 0, match.start()) + 1
            yield {
                "file": file.relative_to(ROOT).as_posix(),
                "symbol": symbol,
                "line": line,
                "fileSha256": hashlib.sha256(original.encode()).hexdigest(),
                "status": "candidate",
            }


if __name__ == "__main__":
    for row in candidates():
        print(json.dumps(row, separators=(",", ":")))
