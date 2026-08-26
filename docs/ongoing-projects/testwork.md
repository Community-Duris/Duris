# High-value regression test expansion

## Goal

Add at least twelve high-value regression cases in previously uncovered areas,
repair defects exposed by those tests, and keep each checkpoint buildable and
published so work can resume safely.

## Progress

### Checkpoint 1 — Unicode and ANSI input boundaries

Added 24 executable runtime cases across two focused tests:

- `test_unicode_runtime.py` exercises all UTF-8 widths, malformed leading and
  continuation bytes, overlong/truncated encodings, surrogate and out-of-range
  scalar rejection, encoder replacement, CP437 conversion, map-input filtering,
  dollar quoting, ASCII downgrade, and sparse Unicode-map composition.
- `test_ansi_runtime.py` exercises color parsing and reset behavior, Unicode
  preservation, CR/LF normalization, malformed/truncated markup, ANSI and
  terminal rendering, uniform coloring, empty gradients, empty strings, and
  expanded/contracted gradients.

Defects fixed while adding the coverage:

- The UTF-8 decoder no longer consumes an unbounded continuation run or accepts
  UTF-16 surrogates and values above `U+10FFFF`.
- The UTF-8 encoder now replaces surrogate code points instead of emitting
  invalid UTF-8.
- Invalid UTF-8 can no longer be re-encoded as `U+FFFD` and accepted as a map
  glyph by `validate_utf8_and_dollars`.
- Applying a non-empty gradient to an empty `AnsiString` no longer dereferences
  `end()`.

Validation completed:

- `python3 tests/async/test_unicode_runtime.py`
- `python3 tests/async/test_ansi_runtime.py`
- regression-runner discovery/execution for both new files
- `make -C src -j2`
- `./scripts/format.sh --check`
- `git diff --check`

Next high-value targets are the JSON/WebSocket escaping boundary, terminal-type
negotiation parser, and file-backed random-name parser.
