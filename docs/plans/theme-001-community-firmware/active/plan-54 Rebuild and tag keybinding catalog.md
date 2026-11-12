# Rebuild and tag the keybinding catalog

Reconcile the keybinding catalog with the v1.3.0 firmware projection and tag bindings by source: `upstream`, `fork:nonlogical`, or both when the fork modifies upstream behavior. Preserve source evidence and do not represent file-level drift alone as a reviewed binding.

- [x] Compare source changes and establish binding-level provenance.
- [ ] Review the 68 changed source files. The three new voice-usage handlers were reviewed and recorded as non-bindings.
- [x] Update records and origin tags using source comparisons and reviewed overrides.
- [x] Validate the rebuilt catalog, coverage audit, and provenance handling; retain the source-drift report.

## Current result

The 1,275 records have 1,167 upstream-only tags, 38 fork-only tags, and 70 with both. The source comparison is scoped to each record's cited files, function bodies, and selected pad mappings. Reviewed exceptions record their evidence and are tied to the current upstream commit and firmware source tree.

The source audit accounts for all 391 discovered handlers, but still reports 68 changed files from the v1.3.0 rebase. The source drift requires further review; do not mark the audit clean by replacing hashes without reviewing changes.
