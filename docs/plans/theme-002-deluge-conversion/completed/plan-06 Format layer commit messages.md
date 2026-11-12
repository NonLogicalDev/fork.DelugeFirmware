# Format layer commit messages

## Goal

Give each layer a semantic commit subject and wrap detailed message bodies to a 90-character target while preserving their content and the resulting firmware tree. [Owners](../../OWNERS.md).

## Work

- [x] Classify each layer and revise its subject to a conventional `type: description` format.
- [x] Rewrap prose at 90 characters without changing its meaning, paragraphs, or original provenance.
- [x] Validate every manifest and replay the full composition and `layerctl check`; checkpoint locally.

All 32 layer subjects use conventional commit types and every line is at most 90 characters. The message bodies and paragraph boundaries retain their original text; unrelated manifest data is unchanged. The generated commit objects were checked against their manifests during a full composition replay.
