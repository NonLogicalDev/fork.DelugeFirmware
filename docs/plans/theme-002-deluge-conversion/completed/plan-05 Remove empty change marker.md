# Remove the empty change marker

## Goal

Remove the unneeded `0014-empty-change-marker` layer while retaining the original commit metadata in provenance. [Owners](../../OWNERS.md).

## Work

- [x] Remove the marker, record its omission, and regenerate the README graph.
- [x] Verify dependencies and the full composed tree, then checkpoint locally.

The original commit and its parent have the same tree `ecd8cb069431a3917de4b985483c2b9832785324`; no layer depends on the marker. A case-insensitive full projection and `layerctl check` passed, with final tree `e184389511a711f129d12b6064c2d29f760babaf`. Original metadata remains in `provenance.json` marked as omitted.
