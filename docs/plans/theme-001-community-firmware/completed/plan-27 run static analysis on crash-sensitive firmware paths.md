---
description: "Select reproducible C++ static-analysis tooling and audit crash-sensitive Deluge firmware paths."
tags:
  - note-type/project
  - status/mature
  - domain/learning
  - deluge
  - firmware
---

# Run static analysis on crash-sensitive firmware paths

## Purpose

Find practical static-analysis tools that understand the Deluge cross-compiled C++ build, then use complementary analyzers to catch credible memory, lifetime, integer, null, bounds, and control-flow defects in the crash-sensitive local firmware paths. This plan began on [[2026-08-24]].

## Scope

### In scope

- Inventory existing CMake, Nix, compiler, `compile_commands.json`, warning, sanitizer, Clang tooling, and project suppression support before adding anything.
- Select at least two complementary analyzers when the existing build model permits it, favoring compiler-aware analysis plus an independent source analyzer.
- Start with the current waveform peak collector and OLED cache path, then inspect the recently changed Slicer/sample ownership, Audio Looper recorder ownership, Panic/audio teardown, and MIDI sustain note-lifetime paths.
- Preserve exact commands, tool versions, target files, exclusions, diagnostics, triage, and reproduction evidence.
- Rank verified findings by crash severity, reachability, confidence, and repair effort; distinguish real defects from cross-compile noise and unsupported compiler extensions.
- Add a small reproducible project-local invocation or configuration only when it reduces repeat work without changing firmware behavior.

### Out of scope

- Treating warning count as a quality metric, bulk formatting, speculative source rewrites, suppressing unexplained findings, or fixing an unproven diagnostic.
- Scanning third-party or generated code unless a call path requires it.
- Firmware flashing, installation, publication, upstream submission, Git, or changes folded into unrelated feature revisions.

## Completion Criteria

- [x] The actual multi-config compile database, ARM GCC driver, generated includes, Nix shell, warning flags, and analyzer configuration are mapped below.
- [x] ARM GCC 14.2.1 `-fanalyzer` and LLVM 21.1.8 Clang Static Analyzer through `clang-tidy` were evaluated as complementary engines; `scan-build` and `cppcheck` decisions are recorded below.
- [x] ARM GCC analyzed the complete in-scope source set. Clang analyzed the finalized waveform producer, navigator, headers, and host OLED spec; its GCC-NEON parser limit on larger UI units is explicit.
- [x] Every diagnostic and independent-audit finding was checked against source reachability. P32 through P38 track the confirmed defects and their revision ownership; rejected warnings record their specific invariants.
- [x] The finalized separated waveform and OLED source has no unresolved high-confidence analyzer diagnostic. Both analyzers missed the earlier reversed-range defect, so P38 retains a focused half-open-range regression in its own bug revision.
- [x] No project-local tooling was added because existing CMake already supports GCC `-fanalyzer` and the current Clang normalization is not robust enough to productize.

## Blockers

None.

## Artifacts

- **Artifact:** Static-analysis inventory and findings.
	- **Type:** Plan evidence and bounded temporary analyzer output.
	- **State:** Completed.
	- **Contents:** Tool versions, normalized compile databases, object outputs, logs, diagnostics, and source-reachability triage for 19 production translation units plus the OLED host spec.
	- **Location:** This plan and the 35 MiB `/tmp/deluge-static-analysis-ab821z/` directory at completion.
	- **SCM surface:** No repository tooling or firmware source was added by P31.
- **Artifact:** P32 [Preserve recorder source name after Output detachment](../backlog/plan-28%20preserve%20recorder%20source%20name%20after%20output%20detachment.md).
	- **Type:** Focused bugfix plan.
	- **State:** Backlog.
	- **Contents:** Timing-dependent `SPECIFIC_OUTPUT` filename-source lifetime defect.
	- **Location:** Track 004 backlog.
	- **SCM surface:** Future fix `ns/bug-sample-recorder-01`.
- **Artifact:** P33 [Validate MIDI knob fields while loading projects](../backlog/plan-29%20validate%20MIDI%20knob%20fields%20while%20loading%20projects.md).
	- **Type:** Focused bugfix plan.
	- **State:** Backlog.
	- **Contents:** Incomplete-project MIDI-knob parser initialization and validation defect.
	- **Location:** Track 004 backlog.
	- **SCM surface:** Future fix `ns/bug-midi-knobs-01`.
- **Artifact:** P34 [Retain every generated Manual Slice drum](../active/plan-30%20retain%20every%20generated%20manual%20slice%20drum.md).
	- **Type:** Focused Slicer correction plan.
	- **State:** Backlog.
	- **Contents:** Deterministic new-Kit Manual Slice confirm null dereference.
	- **Location:** Track 004 backlog.
	- **SCM surface:** Future correction folds into `nl/patch-sample-slicer-04`.
- **Artifact:** P35 [Remove native Stutter uninitialized strength values](../backlog/plan-31%20remove%20native%20stutter%20uninitialized%20strength%20values.md).
	- **Type:** Focused audio-path bugfix plan.
	- **State:** Backlog.
	- **Contents:** Native-rate Stutter recording undefined behavior.
	- **Location:** Track 004 backlog.
	- **SCM surface:** Future fix `ns/bug-stutter-01`.
- **Artifact:** P36 [Make MIDI sustain deferral allocation-safe](../completed/plan-32%20make%20MIDI%20sustain%20deferral%20allocation%20safe.md).
	- **Type:** Focused MIDI-input correction plan.
	- **State:** Backlog.
	- **Contents:** Low-memory deferred-note insertion failure.
	- **Location:** Track 004 backlog.
	- **SCM surface:** Future correction folds into `nl/patch-midi-input-01`.
- **Artifact:** P37 [Keep waveform peak addressing valid for long files](../completed/plan-33%20keep%20waveform%20peak%20addressing%20valid%20for%20long%20files.md).
	- **Type:** Focused waveform bugfix plan.
	- **State:** Completed.
	- **Contents:** Pre-existing signed-32-bit sample and byte narrowing in the shared peak collector.
	- **Location:** Track 004 completed plans.
	- **SCM surface:** Jujutsu change `rrrrtvstzpvkmlsswkwqqtomqmxklmnr`, commit `026a809d9b8fd167badc979422413c613d182d6f`, bookmark `ns/bug-waveform-01`.
- **Artifact:** P38 [Correct reversed waveform row ranges](../completed/plan-34%20correct%20reversed%20waveform%20row%20ranges.md).
	- **Type:** Focused waveform bugfix plan.
	- **State:** Completed.
	- **Contents:** Pre-existing reversed half-open range mapping that can produce a negative source column.
	- **Location:** Track 004 completed plans.
	- **SCM surface:** Jujutsu change `ryzzlqzzlnrpzzlkxlmvplxvmtwopznu`, commit `c4bfc3d7a1157130b868aa08bfedb97c1b256c5e`, bookmark `ns/bug-waveform-02`.

## Decisions

- **Decision:** Run static analysis in its own P31 plan and keep findings separate from the active OLED implementation revision.
	- **Rationale:** Analyzer setup and warning triage affect several firmware components. Keeping them separate prevents unverified diagnostics or tooling files from contaminating `nl/patch-oled-01`, while confirmed OLED defects can still block that revision's validation.
- **Decision:** Require complementary evidence rather than trusting one warning stream.
	- **Rationale:** Embedded cross-compilation commonly produces parser and platform false positives. A diagnostic becomes actionable only after source inspection and, when practical, confirmation by another analyzer, a focused test, or a concrete reachable arithmetic/lifetime path.
- **Decision:** Use ARM GCC 14.2.1 `-fanalyzer` as the primary whole-source engine.
	- **Rationale:** It compiles the actual Cortex-A9 C++ with the project compiler, target ABI, generated headers, defines, and libstdc++. `-fsyntax-only` does not execute GCC's analyzer, so each run emits a temporary object with `-c`.
- **Decision:** Use LLVM 21.1.8 `clang-tidy` with `clang-analyzer-*`, `bugprone-*`, and `performance-*` only on translation units its ARM frontend parses completely.
	- **Rationale:** It independently found a control-flow warning in the navigator and parsed the OLED host test, but larger UI graphs fail in GCC's ARM NEON headers. `scan-build` shares this frontend and adds no coverage; `cppcheck` is absent from the configured shell.
- **Decision:** Do not add a project-local analyzer wrapper in this pass.
	- **Rationale:** CMake already adds `-fanalyzer` under `MAINTAINER_MODE`. The Clang command still needs a temporary normalized database and cannot parse every target unit, so a checked-in wrapper would make a fragile partial workflow look supported.
- **Decision:** Give each confirmed pre-existing defect its own component-grouped `ns/bug-<component>-<NN>` revision, but fold regressions caused by a local feature back into that feature's existing revision.
	- **Rationale:** Independent bug changes remain reviewable without leaving cancellation patches. Corrections to our own unfinished feature should replace the faulty implementation at its source.

## Tool and Source Inventory

- **Build model:** `build/compile_commands.json` contains Debug, Release, and RelWithDebInfo commands. `scripts/tasks/task-configure.py` enables `CMAKE_EXPORT_COMPILE_COMMANDS`; `CMakeLists.txt` defines baseline bounds/null/control-flow warnings and adds `-fanalyzer`, `-Wall`, `-Wextra`, `-Wabi`, and `-Wpedantic` in `MAINTAINER_MODE`.
- **Primary engine:** xPack GNU Arm Embedded GCC 14.2.1 20241119 from the pinned v22 toolchain.
- **Secondary engine:** LLVM/Clang 21.1.8 `clang-tidy`; `scan-build` is present from the same package, and `cppcheck` is absent.
- **Waveform and Slicer seed:** `waveform_renderer.cpp`, `waveform_basic_navigator.cpp`, both `sample_marker_editor.cpp` units, `slicer.cpp`, the local waveform headers, `tests/spec/oled_waveform_renderer_spec.cpp`, `tests/spec/waveform_large_address_spec.cpp`, and `tests/spec/waveform_reversed_range_spec.cpp`.
- **Audio Looper:** `audio_clip.cpp`, `output.cpp`, `sample_recorder.cpp`, and `playback/mode/session.cpp`.
- **Panic/audio teardown:** `menu_item/panic.cpp`, `ui/audio_recorder.cpp`, `hid/buttons.cpp`, `stutterer.cpp`, `global_effectable.cpp`, `mod_controllable_audio.cpp`, and `audio_engine.cpp`.
- **MIDI sustain:** Community Feature settings menu/runtime files and `melodic_instrument.cpp`.

## Ranked Findings

- **P1, confirmed pre-existing and pending, small focused fix:** P32 tracks the `SPECIFIC_OUTPUT` source-name lifetime defect. Normal creation rejects a null source, but `finishCapturing()` can detach the finished recorder before its first `cardRoutine()`. `removeFromOutput()` then clears `outputRecordingFrom`; filename creation still enters the specific-output branch and passes an uninitialized local `name`. The ordering is timing-dependent, but the path is concrete and a bad pointer can reach filename construction. It belongs in `ns/bug-sample-recorder-01`.
- **P2, confirmed pre-existing and pending, small focused fix:** P33 tracks incomplete MIDI-knob project data. `ModControllableAudio::readTagFromFile()` leaves `channel`, `ccNumber`, and `relative` uninitialized, then appends a knob whenever `controlsParam` is valid even when those fields were omitted. Valid files written by the firmware contain the fields; malformed or partial files do not. It belongs in `ns/bug-midi-knobs-01`.
- **P1, confirmed local regression and pending, small correction:** P34 tracks a deterministic new-Kit Manual Slice confirm crash introduced by the current Slicer placement split. The first generated drum is retained, but later generated drums are retained only in the existing-Kit branch; the successful Manual confirm path then dereferences every slot. Two or more Manual slices in a brand-new Kit therefore reach a null entry. The correction belongs inside `nl/patch-sample-slicer-04`, not a separate bug revision.
- **P2, confirmed pre-existing and pending, small audio-path fix:** P35 tracks native-rate Stutter recording passing two uninitialized strength scalars by value to a writer. The writer returns before using them on its native branch, but evaluating indeterminate scalar arguments is already undefined behavior. The native branch should call the established native writer directly in `ns/bug-stutter-01`.
- **P2, confirmed local regression and pending, bounded-state correction:** P36 tracks sustain note-off deferral allocating a map node through an allocator that throws `BAD_ALLOC`. The MIDI input path does not catch it. Normal-memory behavior remains coherent, but fixed 128-note state is preferable because it removes event-path allocation and its low-memory reset risk. Because the sustain feature introduced this path, the correction belongs inside `nl/patch-midi-input-01`, not a separate bug revision.
- **P1, confirmed pre-existing and resolved in its own bug revision:** P37 tracks shared peak-collector arithmetic that narrowed 64-bit viewport positions and valid file-byte positions through signed 32-bit values. `ns/bug-waveform-01` now owns the correction and two focused cases through the 2 GiB and 4 GiB byte positions.
- **P1, confirmed pre-existing and resolved in its own bug revision:** P38 tracks reversed single-row rendering that mapped half-open `[xStart,xEnd)` as `width - 1 - xEnd`, so `[0,16)` became `[-1,15)`, allowing `colStatus[-1]` and a zero divisor. Neither analyzer reported it before the repair. `ns/bug-waveform-02` now owns the correction and focused half-open-range regression.
- **Rejected with source or storage invariants:** GCC's `nextReadCluster` warning is blocked by the one-cluster end break and by loading the next cluster before every multi-cluster advance. Compressor display values cannot see the `LAST` sentinel through normal enum normalization. Object-pool/vector null warnings ignore allocators that throw on failure. Reverb amplitude increments are initialized at every current call that supplies nonzero pan; the default-false callers supply zero pan, so the reported read is not reachable. The production Reverb object has static storage and is zero-initialized before construction, so its omitted `hpf_` initializer is a state-restoration robustness issue rather than an indeterminate Panic read. Clang's navigator divide-by-zero path requires a pin delta above `2^47`, while supported WAV/AIFF data and recorder lengths are 32-bit, so the scaling loop cannot zero its denominator for a real sample.

## Implementation Steps

- [x] Map the build database, compiler wrappers, Nix packages, analysis configs, warnings, and source exclusions.
- [x] Run real ARM GCC analysis on the waveform/OLED seed and distinguish compile failures from analyzer findings.
- [x] Run Clang Static Analyzer on the parseable production and host-test units and record its ARM NEON limit.
- [x] Expand ARM GCC analysis to Slicer, Audio Looper, Panic/audio teardown, and MIDI sustain.
- [x] Reproduce and classify every warning; open focused plans for the confirmed defects and assign revision ownership by origin.
- [x] Re-run the combined OLED source after the cache-header split and reversed-range repair.
- [x] Independently inspect cross-function Slicer, recorder, Panic, Stutter, and sustain state that single-translation-unit analyzers cannot model; assign P34 through P36 for the additional confirmed paths.
- [x] Re-run both analyzer workflows after P37, P38, and the feature-only OLED revision are separated.

## Work Log

- [x] [[2026-08-24]]@17:05 - Opened P31 for a parallel crash-sensitive static-analysis audit.
	- **Evidence:** The user requested static-analysis tooling and a parallel pass over critical firmware sections while P30 remains in implementation.
	- **Effect:** Tool inventory and diagnostic triage have a separate durable scope. P30 remains responsible for the OLED feature and its own blocking findings.
	- **Next:** Inspect the current CMake/Nix compile database and trial compatible analyzers without changing firmware source.
- [x] [[2026-08-24]]@17:29 - Completed both analyzer pipelines and the source-reachability audit.
	- **Evidence:** ARM GCC emitted temporary objects for all 19 production units. Finalized waveform renderer, navigator, both marker editors, and Slicer logs are empty. Clang fully parsed the renderer, navigator, and OLED host spec. Every nonempty extended GCC log was traced to its call and lifetime invariant.
	- **Effect:** The combined OLED source is clear of unresolved analyzer findings. P32, P33, and P35 are independent pre-existing defects; P34 and P36 are regressions in local feature revisions; P37 and P38 separate the two pre-existing waveform repairs from OLED.
	- **Next:** Keep P31 closed. Use standalone bug revisions only for the pre-existing defects and fold P34 and P36 into their originating feature revisions.
- [x] [[2026-08-24]]@17:29 - Recorded the analyzer blind spot exposed by the reversed-range review.
	- **Evidence:** Pre-fix GCC and Clang logs contained no bounds or divide finding for the reachable `[-1,15)` source range. The repaired helper and focused host test now encode correct half-open reversal.
	- **Effect:** Static-analysis cleanliness is not used as the sole proof for waveform range geometry.
	- **Next:** Retain the focused regression in P38 and revalidate it after `ns/bug-waveform-02` is separated from P30.
- [x] [[2026-08-24]]@17:38 - Completed an independent cross-function audit after the analyzer pass.
	- **Evidence:** The audit followed Slicer mode state through `doSlice()` and post-confirm bound assignment, native Stutter branching through its delay-buffer writer, and sustain note-off insertion through the custom allocator. It also rechecked the recorder-name lifetime finding and the larger Panic/audio teardown call graph.
	- **Effect:** P34 records the deterministic new-Kit Manual Slice null dereference, P35 records native Stutter undefined behavior, and P36 records the low-memory sustain insertion risk. The Audio Looper ownership repair has no additional defensible use-after-free or double-free finding.
	- **Next:** Keep P32, P33, and P35 in separate component bug revisions; fold P34 and P36 into their originating feature revisions; keep the waveform separations in P37 and P38.
- [x] [[2026-08-24]] - Revalidated the separated waveform and OLED revisions.
	- **Evidence:** ARM GCC `-fanalyzer` produced zero diagnostics for the post-split renderer, navigator, sample-marker editor, and Slicer. LLVM reproduced only the previously reviewed bounded conversion and API-shape warnings and found no new reachable path. The direct spec runner passed 22 OLED, 2 large-address, and 1 reversed-range cases.
	- **Effect:** P37 and P38 are complete in their own bug revisions, and the analyzer conclusions still hold for feature-only `nl/patch-oled-01`.
	- **Next:** None for P31.

## Validation

- **Command or action:** Cross-build compatibility probe on the waveform/OLED seed files.
	- **Expected result:** The analyzer consumes the real include graph and relevant defines without silently dropping the target file or reporting only unsupported-driver noise.
	- **Observed result:** Passed with ARM GCC 14.2.1. For each source, the Debug command was selected with `jq -r --arg f "$PWD/$path" '.[] | select(.file == $f and (.command | contains("Debug"))) | .command' build/compile_commands.json`; `-mthumb-interwork` was removed, the output was redirected under `/tmp/deluge-static-analysis-ab821z/`, and `-fanalyzer -fanalyzer-verbosity=1 -c` was inserted before execution. The five final seed logs contain zero diagnostics.
	- **Failure signals:** Zero files analyzed, missing generated headers, ignored target flags, parser failure before function bodies, or diagnostics caused only by host/target width disagreement.
	- **Interpretation:** GCC analyzed the actual target C++ and ABI. A deliberate null-dereference probe established that `-fsyntax-only` does not run this analyzer and that the `-c` method does.
- **Command or action:** `nix develop path:./.nix --command clang-tidy -p /tmp/deluge-static-analysis-ab821z/compile-db-clang-final <source> --checks='-*,clang-analyzer-*,bugprone-*,performance-*' --header-filter='^/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/gui/waveform/' --quiet`.
	- **Expected result:** Clang independently parses the target unit with ARM widths and reports analyzer findings separately from style noise.
	- **Observed result:** Passed for `waveform_renderer.cpp` and `waveform_basic_navigator.cpp` after selecting Debug commands, dropping GCC-only flags, adding `--target=arm-none-eabi`, and adding the pinned compiler's libstdc++ include directories. The same checks passed on the OLED host spec through `/tmp/deluge-static-analysis-ab821z/compile-db-test-v2`. Larger marker-editor and Slicer graphs stop in GCC's `arm_neon.h` builtins and are not accepted as complete Clang runs.
	- **Failure signals:** Fatal target-header errors, warnings after a fatal parse error, or diagnostics that depend on host rather than ARM type widths.
	- **Interpretation:** Clang provides independent coverage for the new waveform math and cache code, but not whole-firmware coverage; ARM GCC remains authoritative for the larger units.
- **Command or action:** Complementary analyzer run and manual reachability review.
	- **Expected result:** Credible integer, bounds, null, ownership, lifetime, and control-flow findings are either corroborated or rejected with exact source evidence.
	- **Observed result:** Passed. P32 through P38 retain the confirmed paths and their origin-based revision ownership. Post-split GCC logs contain zero diagnostics for the renderer, navigator, sample-marker editor, and Slicer. LLVM reproduced only the previously reviewed bounded conversion and API-shape warnings, with no new reachable finding.
	- **Failure signals:** Bulk unsorted warnings, unexplained suppressions, diagnostics quoted without their code path, or fixes proposed solely to silence a tool.
	- **Interpretation:** The audit produced a ranked repair queue and cleared the finalized OLED source; it cannot prove absence of device-only timing or hardware faults.

## Unfinished Work

None for P31. P32 through P36 retain the unfinished follow-up work; P34 and P36 must amend their originating feature revisions rather than create separate bug revisions. P37 and P38 are complete in their separate waveform bug revisions.
