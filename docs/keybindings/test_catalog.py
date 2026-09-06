# /// script
# requires-python = ">=3.11"
# dependencies = ["jsonschema==4.26.0"]
# ///
"""Focused regression tests for the catalog schema and coverage audit."""

import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from jsonschema import Draft202012Validator

import audit
import validate

DIRECTORY = Path(__file__).resolve().parent


class SchemaTests(unittest.TestCase):
    def setUp(self):
        self.schema = json.loads((DIRECTORY / 'binding.schema.json').read_text())
        self.validator = Draft202012Validator(self.schema)
        self.row = json.loads((DIRECTORY / 'bindings.jsonl').read_text().splitlines()[0])

    def test_event_gesture_and_sequence(self):
        for key in ['learn.down', {'hold': ['horizontal'], 'trigger': 'horizontal.turn'}, {'sequence': ['shift.down', 'shift.down'], 'maxGapMs': 500, 'inclusive': True}]:
            with self.subTest(key=key):
                self.row['key'] = key
                self.assertEqual(list(self.validator.iter_errors(self.row)), [])

    def test_obsolete_and_invalid_gestures(self):
        for key in [{'press': 'learn'}, {'turn': 'horizontal'}, {'hold': ['horizontal.turn'], 'trigger': 'learn.down'}, {'trigger': 'learn'}, {'sequence': ['shift.down']}, {'sequence': ['shift.down', 'shift.down'], 'maxGapMs': 500}]:
            with self.subTest(key=key):
                self.row['key'] = key
                self.assertTrue(list(self.validator.iter_errors(self.row)))

    def test_duplicate_ids_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / 'bindings.jsonl'
            target.write_text((json.dumps(self.row) + '\n') * 2)
            self.assertTrue(any('duplicate id' in error for error in validate.validate(target)))

    def test_unknown_control_rejected(self):
        self.row['key'] = {'trigger': 'unknown-button.down'}
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / 'bindings.jsonl'
            target.write_text(json.dumps(self.row) + '\n')
            self.assertTrue(any('unknown control' in error for error in validate.validate(target)))

    def test_button_rotation_rejected(self):
        self.row['key'] = {'trigger': 'learn.turn'}
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / 'bindings.jsonl'
            target.write_text(json.dumps(self.row) + '\n')
            self.assertTrue(any('unsupported event' in error for error in validate.validate(target)))


class AuditTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.docs = self.root / 'docs'
        self.docs.mkdir()
        (self.root / 'handler.cpp').write_text('void buttonAction() {}\n')
        (self.root / 'helper.cpp').write_text('void helper() {}\n')
        self.hashes = {name: hashlib.sha256((self.root / name).read_bytes()).hexdigest() for name in ['handler.cpp', 'helper.cpp']}
        self.bindings = [{'id': 'test', 'key': {'trigger': 'learn.down'}, 'context': {'view': 'test'}, 'sources': [{'file': name} for name in self.hashes]}]
        self.ledger = [{'file': 'handler.cpp', 'symbol': 'UI::buttonAction', 'status': 'reviewed', 'bindingIds': ['test'], 'reason': 'Reviewed fixture.', 'fileSha256': self.hashes['handler.cpp']}]
        self.candidates = [{'file': 'handler.cpp', 'symbol': 'buttonAction'}]

    def run_audit(self):
        (self.docs / 'bindings.jsonl').write_text('\n'.join(map(json.dumps, self.bindings)) + '\n')
        (self.docs / 'coverage.jsonl').write_text('\n'.join(map(json.dumps, self.ledger)) + '\n')
        (self.docs / 'source-snapshot.json').write_text(json.dumps({'files': self.hashes}))
        (self.docs / 'controls.json').write_text('{}')
        with patch.object(audit, 'ROOT', self.root), patch.object(audit, 'DIRECTORY', self.docs), patch.object(audit, 'candidates', return_value=iter(self.candidates)):
            return audit.audit()

    def test_clean_snapshot(self):
        result = self.run_audit()
        for field in ['unaccountedCandidates', 'changedSourceFiles', 'invalidCoverage']:
            self.assertEqual(result[field], [])

    def test_uncovered_handler(self):
        self.candidates.append({'file': 'handler.cpp', 'symbol': 'padAction'})
        self.assertEqual(len(self.run_audit()['unaccountedCandidates']), 1)

    def test_helper_drift_without_ledger_row(self):
        (self.root / 'helper.cpp').write_text('void changedHelper() {}\n')
        self.assertEqual(self.run_audit()['changedSourceFiles'], ['helper.cpp'])

    def test_missing_helper_snapshot(self):
        del self.hashes['helper.cpp']
        self.assertTrue(self.run_audit()['invalidCoverage'])

    def test_reviewed_row_with_gap_is_not_complete(self):
        self.ledger[0]['gaps'] = ['unreviewed branch']
        self.assertTrue(self.run_audit()['invalidCoverage'])

    def test_unknown_binding_reference(self):
        self.ledger[0]['bindingIds'] = ['missing']
        self.assertTrue(self.run_audit()['invalidCoverage'])

    def test_equivalent_gestures_and_sequences(self):
        self.assertEqual(audit.gesture_key({'hold': ['shift', 'learn'], 'trigger': 'select.down'}), audit.gesture_key({'hold': ['learn', 'shift'], 'trigger': 'select.down'}))
        self.assertEqual(audit.gesture_key({'sequence': ['shift.down', 'shift.down']}), audit.gesture_key({'sequence': [{'trigger': 'shift.down'}, {'trigger': 'shift.down'}]}))
        self.assertEqual(audit.gesture_key({'sequence': [{'hold': ['shift', 'learn'], 'trigger': 'select.down'}, 'select.up']}), audit.gesture_key({'sequence': [{'hold': ['learn', 'shift'], 'trigger': 'select.down'}, {'trigger': 'select.up'}]}))
        self.assertNotEqual(audit.gesture_key({'sequence': ['select.down', 'select.up']}), audit.gesture_key({'sequence': ['select.up', 'select.down']}))
        self.assertNotEqual(audit.gesture_key({'sequence': ['shift.down', 'shift.down'], 'maxGapMs': 500}), audit.gesture_key({'sequence': ['shift.down', 'shift.down'], 'maxGapMs': 400}))


if __name__ == '__main__':
    unittest.main()
