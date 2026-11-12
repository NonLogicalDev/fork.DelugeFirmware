# Deluge worktree inventory

These entries were retained from the shared layerctl owner record when the user moved Deluge plans here. Historical checkpoints are local to this Mac unless stated otherwise. [Owners](../../OWNERS.md) · [Conversion](../../theme-002-deluge-conversion/EXEC_STATE.md).

- Deluge source checkout: `/Users/nonlogical/Projects/remote/github.com/NonLogicalDev/fork.DelugeFirmware`, Git common directory `.git`; clean branch `nl/latest`, HEAD and origin/nl/latest `2391b7cad4907843890d8b6c052651a77abd6bbf`, merge base with origin/main `9e3132dc17070695b40f69bada5565a33ce428b4`. Manual SSH partial clone, non-sparse, on this Mac; host replacement durability unverified. Reconstruct with `git clone --filter=blob:none git@github.com:NonLogicalDev/fork.DelugeFirmware.git`; the original branches remain on the remote. See the [conversion plan](../../theme-002-deluge-conversion/completed/plan-01%20Convert%20JJ%20layers.md).

- Deluge conversion checkpoint: local branch `layerctl-conversion` at `5967d2e3e82e60a9739ffed2a9b34ec1c50bca8e`, an independent definition-history root held in the fork checkout Git object database. The branch is not pushed; ordinary process termination preserves it but host replacement durability is unverified. Original `nl/latest` remains checked out and clean. Temporary validation used the case-sensitive image `/private/tmp/layerctl-deluge-case-sensitive.sparseimage`; the local branch is the durable working copy on this Mac.

- Deluge definition worktree: `/Users/nonlogical/Projects/remote/github.com/NonLogicalDev/fork.DelugeFirmware-layerctl`, branch `layerctl-conversion`, HEAD `5967d2e3e82e60a9739ffed2a9b34ec1c50bca8e`, no upstream, no sparse checkout. It shares `/Users/nonlogical/Projects/remote/github.com/NonLogicalDev/fork.DelugeFirmware/.git` with the original checkout; no remote contains this branch. Reconstruct locally with `git -C /Users/nonlogical/Projects/remote/github.com/NonLogicalDev/fork.DelugeFirmware worktree add /Users/nonlogical/Projects/remote/github.com/NonLogicalDev/fork.DelugeFirmware-layerctl layerctl-conversion`. The temporary case-sensitive image was unmounted and retained at `/private/tmp/layerctl-deluge-case-sensitive.sparseimage`; temporary storage may be removed by the system.

- Latest Deluge definition worktree checkpoint: `7f7c7bdfd83dfc537972268a888b43c46d297053` on local `layerctl-conversion`, containing the generated README graph; no remote upstream. Storage and recovery limitations remain as described above.

- Latest local checkpoints: layerctl `7e06e241a550f835808816eae2e9e05dfdb963de` on misc.experiments `main`; Deluge definitions `e8c71d7b103e95a050e51c0c759308e512f27038` on `layerctl-conversion`. Neither checkpoint has been pushed; checkout storage and recovery limitations remain as described above.

- Latest local checkpoints (2026-11-11): layerctl implementation `f4264d3725e599074b431a1dbb099d4a89ef2882` on misc.experiments `main`, and Deluge definitions `417e6f9e4211ce6891e557e60285cfdbb5080418` on `layerctl-conversion`. The source checkout and remote refs are unchanged; neither checkpoint has been pushed. The earlier 7f7c and e8c checkpoints above are historical.

- Layerctl pre-checkout script implementation checkpoint: `fecb2a018fab93d295a64aaddc68cf74aa9ceefa` on misc.experiments `main`. Deluge plan relocation was checkpointed at `ad01b534ceb97e13e2cf3779e392a4fafdbc1bb7`; see `git log` for later definition checkpoints. Neither is published.

- Layerctl script working-directory and clone environment follow-up: local checkpoint `65c933091f74a19f3080cb99cb9f7dc50d0ec03f` on misc.experiments `main`. See `git log` for the latest Deluge definition checkpoint. Neither is published.

- Current definition checkout: branch `main` tracking `origin/main`; `layerctl-conversion` was removed locally and remotely after main was updated. The original `nl/latest` checkout remains separate. Consult `git status` and `git log` for the current checkpoint.
