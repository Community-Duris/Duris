# Actual inactive alchemist reset fixture

`run_actual_reset.py` links `alchemist_actual_reset.cpp` against the complete
original production flatfile object closure. It queries the original Makefile,
verifies the supplied source and exported object pins, and renames only `main`
in an owned copy of `net/comm.o`. No gameplay provider or authority is replaced.

This fixture is deliberately tied to the qualified source manifest
`51de87fb263df80e1aba5265f92600d6a6dfa438bc19934a5217142640285b8f`.
It requires the matching retained source manifest, full source tree and original
production build export (`export-manifest.json`, objects and
`tests/native-quest-major-builds/results.json`). It refuses mismatched source
or providers. It does not silently rebuild or treat another revision as tested.

Run on Linux, with source/providers read-only and a fresh owned evidence path:

```sh
python3 -B -u tests/async/native_birth_accounting/run_actual_reset.py \
  --source-root /path/to/qualified-source \
  --providers-root /path/to/qualified-build/bin \
  --source-pins /path/to/qualified-source-pins.json \
  --artifacts-root /path/to/owned-evidence
```

The source includes the existing disposable mini-world builder; Linux requires
the original build tools/libraries, `objcopy`, `timeout` and Python. Each native
world keeps the original 60-second bound, five-second TERM grace and 64 MiB
stack. No SQL, Redis, live account or production data is used.

Present/missing VNUM102 worlds exercise actual boot/reset, class selection,
chance and UID allocation. Retry assertions check the actual latch. A real
granted vial is moved into a real container and captured through the public
literal tree API and canonical codec, with original UIDs and parent placement.

This is a scoped inactive fixture, separate from active birth admission and
authenticated SQL/custody/publication/ACK/cold recovery. Its driver is a nested
qualification helper, not an automatically selected top-level regression or a
new release gate. See the [recorded native evidence](../../../docs/persistence/economy_accounting/ALCHEMIST_ACTUAL_RESET_LITERAL_QUALIFICATION_2026-10-06.md).
