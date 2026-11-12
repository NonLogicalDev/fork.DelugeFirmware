# DelugeFirmware conversion

Status: complete locally. Detailed layer messages use conventional subjects and a 90-character wrap target; `DEVIATIONS.md` lives in the definition repository; and the base clone prepares the casing fix before checkout. The current worktree is `/Users/nonlogical/Projects/remote/github.com/NonLogicalDev/fork.DelugeFirmware-layerctl`, local branch `layerctl-conversion` (see `git log` for the latest checkpoint). The original checkout and remote are unchanged. [Original conversion](completed/plan-01%20Convert%20JJ%20layers.md) · [follow-up](completed/plan-02%20Explain%20layer%20changes.md) · [owners](../OWNERS.md).

Completed: [use the upstream Deluge repository and normalize casing with a script](completed/plan-03%20Use%20upstream%20source.md).

The base now uses official upstream `main` at the pinned source commit and a pre-checkout script. The full projection and replay passed on a case-insensitive filesystem with the recorded final tree. The Deluge plans live in this repository.

Completed: [include the Nix environment in the base layer](completed/plan-04%20Include%20Nix%20environment%20in%20base.md).

Completed: [remove the empty change marker](completed/plan-05%20Remove%20empty%20change%20marker.md).

Completed: [format layer commit messages](completed/plan-06%20Format%20layer%20commit%20messages.md).

Completed: [convert the casing hook to Perl](completed/plan-07%20Convert%20casing%20hook%20to%20Perl.md).
