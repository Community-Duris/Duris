# Code Review

## Findings Repaired

1. **High - downstream capture initially occurred at parent completion.** Group and
   equipment state could have changed while the parent was in flight. Artifact/guild
   payloads are now captured and journaled immediately after parent acceptance, and an
   actor player key orders each child behind its parent.
2. **High - generic guild saves could overwrite transaction-owned totals.** Existing-row
   upserts now preserve `prestige` and `construction`; only the revisioned outcome
   repository advances those fields for award-linked effects.
3. **High - generic repository harnesses did not link every dispatch target after the new
   command was added.** All domain MySQL harness link lines now include the combat and
   artifact/guild repositories and required OpenSSL dependency.
4. **Medium - explicit high combat test PIDs advanced the development auto-increment past
   a signed legacy auction column.** The auction harness now uses isolated signed-safe
   fixture PIDs and affected harnesses clean their operation outbox fixtures.
5. **Medium - a standalone coordinator codec test omitted the new SHA-256 link dependency.**
   Its compile command now links OpenSSL and passes independently and in `make test-all`.
6. **Medium - parent and child commands originally did not share an entity fence.** The
   child command now always declares its actor player key in addition to artifact and
   optional guild keys, preserving deterministic admission order across rapid awards.

No unresolved blocking finding remains. Artifact item custody changes remain delegated
to the Session 05 ownership transaction; this session advances feed/bind metadata and
guild award totals only.
