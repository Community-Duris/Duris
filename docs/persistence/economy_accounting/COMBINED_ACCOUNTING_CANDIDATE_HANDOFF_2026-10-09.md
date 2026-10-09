# Guarded combined accounting candidate - 2026-10-09

This development source joins the reviewed accounting work without opening
accounting admission or claiming Plans 2-4, R1-R8 or release completion. Existing
inactive behavior and the declined inactive spell path remain preserved.

Selected manifest: `a7bc4532d6b0146f4f6144889e42d4ebb7a11f960cbe3e1f50f1861aa0a9d646`.
The 228 selected paths include 188 source inputs, 31 tests, five schema inputs
and four operator scripts. The source application records exact maintained
preimages and preserves the three unrelated WIP files. Inventory is not coverage.

The selected work includes shared native/SHOP owners, retained Smith preparation
and save profiles, original ROOM source/cursor contracts, SQL and flat atomic
participants, complete item forests, independent native flat season metadata,
and the genuine ROOM worker/executor/startup connection. The callback defaults
to null on SQL and is registered only for client-free flatfile. Original general
flat admission still excludes ROOM; registration grants no new authority.
Completed independent Plan 5 reader/operator slices remain included at their
recorded source scope. SQL migration manifests terminate at
`0065_zone_reset_item_birth_origin`; bootstrap alone does not establish that schema.

The larger cold-reader proposal `5b2ff6b8` remains unselected. Its caller budget
and the separate INITIAL keeper fixture's `item{}` correction remain unfinished.
Original source/factory admission, the prospective aggregate 32 MiB bound,
world publication, terminal recovery/ACK and stopped legacy enrollment remain
required. The coordinator's 64 MiB retention bound does not satisfy the 32 MiB
requirement. Existing holds and activation gates remain closed.

## Bounded executable checkpoint

Freeze this source in one published commit, then build from that exact commit
with separate fresh outputs. Record the actual commit, commands, binary hashes,
build logs and results; historical component passes do not qualify this binary.

```sh
make -C src -j2 PERSISTENCE_BACKEND=mariadb BUILD_PROFILE=production \
  OBJDIR=/absolute/owned/output/sql-objects DMS_BINARY=/absolute/owned/output/dms-sql
make -C src -j2 PERSISTENCE_BACKEND=flatfile BUILD_PROFILE=production \
  OBJDIR=/absolute/owned/output/flat-objects DMS_BINARY=/absolute/owned/output/dms-flatfile
python3 tests/async/test_flatfile_boot_preflight.py --server /absolute/owned/output/dms-flatfile
```

The flat preflight uses the full binary, tracked minimal world, disposable roots
and ephemeral listeners. It verifies health and normal shutdown and retains its
existing UID-authority refusal cases. `test_minimal_boot.py` is a source check.
Bounded storage modeling supports its pinned libstdc++ 13/C++11 ABI; unsupported
policies deliberately refuse bounded routes.

SQL smoke requires a genuinely prepared disposable schema-65 restore candidate
and its private socket-only daemon, then the existing
`persistence_restore.service_load`/`qualify_service_restore.py` owner. Do not use
a shared service or fabricated restore marker. If that prerequisite is unavailable,
record SQL smoke as unavailable while publishing available build/flat results.
The broader backup suite remains with Plan 5 and the major qualification batch.

## Native integration repair checkpoint

Full fresh SQL and flatfile scans of `73195295e1384d7c3aae31107e644a9a8c612b99`
collected the remaining compiler blockers. Their repairs preserve strict warnings,
inactive behavior and closed admission: direct existing declarations, an unsigned
errno result, optional unused diagnostic parameters, redundant signed-byte tests,
exact widened SHOP PID comparisons, reference-only counter compaction and a
cleanup parameter rename. Original actor observations now execute inside their
existing private birth owner. The ROOM source owner gets narrow private observer
friendship and sorts only its copied observation UID list; original tree order and
duplicate/foreign-link refusal remain intact. All eleven affected translation
units compile in both fresh production profiles. Independent source reviews
passed for the private owner and SHOP changes. No new lexical writer sites or
writer policies are introduced. Evidence: `build-successor-73195295e/{native,build-fixes,build-fixes-extra,build-fix-publication}/`
under the candidate evidence directory below. Full rebuilt binary link and smoke
remain pending; these object checks grant no gameplay or release qualification.

The subsequent full `201b5fc71` SQL compile reached linking without source errors;
linking failed because installed WSL linker scripts reference absent `/lib` math
libraries. Genuine matching libraries exist under `/usr/lib`; a private linker
script alias can retain the original production flags and system files. The
full flat compile found one missing direct `RENT_CRASH` declaration in the SHOP
native checkpoint. Including its existing `core/files.h` owner fixes that TU;
fresh strict SQL and flat object checks both passed. The temporary build outputs
were unavailable after the WSL session ended, so the retry could not verify its
source and did not link. The next full candidate run must retain binaries and
use owned persistent output directories. Full link and smoke remain unqualified.

Published source `25b863da7727b57c30901cf271a8fa555c9ee954` reached both fresh
production compilers. Both stopped at the same missing-field initializer error
in the Harvester caller. The complete repair explicitly initializes refinement
fields in all four non-refinement aggregates (Harvester, poison and both Encrust
outcomes); affected `drannak.c` and `salchemist.c` objects now compile with the
unchanged strict production flags on both SQL and flatfile. No new writer sites
or policies are introduced. Full candidate link and smoke remain pending.
Evidence and exact commands: `bin/tests/combined-accounting-candidate-primary-20261009/native/`
and `bin/tests/combined-accounting-candidate-primary-20261009/craft-initializer-repair/`.
The existing disposable SQL fixture also requires Docker access; its read-only
WSL prerequisite checks failed because Docker integration is unavailable.
Image availability is unknown. No SQL smoke or service was started.

Full player, persistence,
crash/recovery, dual-engine migration, reconciliation and release qualification
remain in the original acceptance batches. No production data, service or
accounting activation is part of this handoff.
