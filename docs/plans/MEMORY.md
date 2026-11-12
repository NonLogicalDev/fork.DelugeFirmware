# Deluge layer project map

Read [owner instructions](AGENTS.md) and verify [ownership](OWNERS.md) before owner work. [README](README.md) links the project records. The [conversion execution state](theme-002-deluge-conversion/EXEC_STATE.md) tracks the definitions, retained history, and completed upstream-source and script conversion. The [worktree inventory](_owner/state/worktrees.md) records both the source checkout and definition worktree. [Task zero](theme-000-meta/EXEC_STATE.md) maintains this plan home.

The definitions live in this repository’s `layers/`; original commit and branch metadata are in `provenance.json`, and feature notes are in `DEVIATIONS.md`. Layerctl is developed in the sibling `misc.experiments/utils/layerctl` checkout.

Last audited: 2026-11-11. Evidence: the prior owner plan, current checkout, user direction, and verified upstream refs. Re-audit when the project map or checkout changes.

The base layer includes the original local Nix environment as an overlay; see the [completed consolidation](theme-002-deluge-conversion/completed/plan-04%20Include%20Nix%20environment%20in%20base.md).
