# Deluge layer project map

Read [owner instructions](AGENTS.md) and verify [ownership](OWNERS.md) before owner work. [README](README.md) links the project records. The [conversion execution state](theme-002-deluge-conversion/EXEC_STATE.md) tracks the definitions, retained history, and completed upstream-source and script conversion. The [worktree inventory](_owner/state/worktrees.md) records both the source checkout and definition worktree. [Task zero](theme-000-meta/EXEC_STATE.md) maintains this plan home.

The definitions live in this repository’s `layers/`; original commit and branch metadata are in `provenance.json`, and feature notes are in `DEVIATIONS.md`. Layerctl is developed in the sibling `misc.experiments/utils/layerctl` checkout.

Last audited: 2026-11-11. Evidence: the prior owner plan, current checkout, user direction, and verified upstream refs. Re-audit when the project map or checkout changes.

The base layer includes the original local Nix environment as an overlay; see the [completed consolidation](theme-002-deluge-conversion/completed/plan-04%20Include%20Nix%20environment%20in%20base.md).

Layer commit messages use conventional subjects and a 90-character wrap target. The base casing hook is Perl; see the [message format](theme-002-deluge-conversion/completed/plan-06%20Format%20layer%20commit%20messages.md) and [hook conversion](theme-002-deluge-conversion/completed/plan-07%20Convert%20casing%20hook%20to%20Perl.md) plans.

The [firmware workstream](theme-001-community-firmware/EXEC_STATE.md) contains the Deluge feature plans moved from the learning owner, preserving their ownership and history.

The [v1.3.0 layer audit](theme-001-community-firmware/artifacts/v1.3.0-layer-audit.md) compares all 32 layers against the release and records partial overlaps and trial conflicts. The pinned base has not been changed.
