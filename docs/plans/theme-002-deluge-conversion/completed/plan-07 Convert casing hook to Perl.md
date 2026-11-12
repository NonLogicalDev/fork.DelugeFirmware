# Convert the casing hook to Perl

## Goal

Replace the base layer’s Python casing hook with Perl while preserving binary-safe Git index handling, collision checks, and the resulting firmware tree. [Owners](../../OWNERS.md).

## Work

- [x] Port the hook and update the manifest and documentation to use Perl.
- [x] Check the original normalized tree, idempotence, collision rejection, and full composition replay.
- [x] Save the verified changes locally.

The Perl hook reproduced normalized tree `e3c03ddf67df0a623a442ee28e3a70cd2ab8c308`, preserved executable modes and arbitrary filename bytes, ran idempotently, and rejected collisions without changing the index. A case-insensitive full composition and `layerctl check` reproduced final tree `e184389511a711f129d12b6064c2d29f760babaf`.
