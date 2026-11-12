# Move firmware plans into the repository

## Goal

Move the existing Deluge firmware workstream plans into this repository while preserving their contents, statuses, original owner, and links from the separate music owner. Leave unrelated music work in place. [Owners](../../OWNERS.md).

## Work

- [x] Inventory firmware plans and existing references; preserve the original firmware owner and plan status.
- [x] Relocate the workstream into `docs/plans/theme-001-community-firmware/` and retain a link from the old location for existing references.
- [x] Verify file contents, plan structure, owner records, and links; checkpoint the repository locally.

Moved 50 plans and their execution state from the Synthstrom Deluge learning owner to `docs/plans/theme-001-community-firmware/`. Preserved the original owner register and all status directories. A compatibility symlink at the old location keeps the music owner’s existing links working. The relocated group has 29 active, 4 backlog, 16 completed, and 1 archived plan; its structure and relative links were checked. Corrected two links that already pointed to outdated status folders. Original plan details and statuses were otherwise retained.

## Integration follow-up

The user requested the firmware plans be integrated into the existing theme structure. Move the workstream to `docs/plans/theme-001-community-firmware/`, preserve its original owner in the shared owner register, remove the temporary nested plan home, and update the old-location compatibility link. Verify group structure and links.

The user requested integration into the existing theme structure. The workstream is now `theme-001-community-firmware`; its original owner is retained in the shared owner register, and the old learning-owner path points to the relocated theme. The temporary nested plan home was removed. The project plan doctor reports three groups and 59 plans with no errors; all moved relative Markdown links resolve.
