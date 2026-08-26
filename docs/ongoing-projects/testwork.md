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

### Checkpoint 2 — JSON and terminal negotiation boundaries

Added 26 more executable runtime cases, bringing the current total to 50:

- `test_json_utils_runtime.py` covers null/default handling, JSON metacharacter
  and ASCII-control escaping, valid and malformed UTF-8, ANSI removal,
  truncated/unknown markup, strict command parsing, typed getters, and the text
  and GMCP wrapper builders.
- `test_ttype_runtime.py` covers the RFC 1091 negotiation bytes and state
  transitions, WebSocket suppression, client normalization, both terminal-list
  completion modes, MTTS capability parsing, malformed MTTS fields, control and
  non-ASCII rejection, oversized values, and charset selection.

Defects fixed while adding the coverage:

- JSON escaping now rejects overlong UTF-8, surrogate encodings, and values
  above `U+10FFFF` rather than copying structurally plausible invalid bytes.
- ANSI removal now recognizes only complete, valid Duris color codes; truncated
  markup no longer advances beyond the input and unknown markup is preserved.
- Command and embedded GMCP JSON parsing now rejects trailing data, and command
  parsing rejects non-object roots.
- JSON typed getters handle null objects and keys explicitly, while the GMCP
  builder safely handles null package and data inputs.
- Terminal-type input now rejects empty, oversized, control-bearing, and
  non-ASCII values instead of stalling, truncating, or retaining unsafe client
  metadata.
- MTTS bitvectors now require a complete unsigned decimal field, preventing
  strings such as `MTTS 4JUNK` from silently enabling capabilities.

Validation completed:

- `python3 tests/async/test_json_utils_runtime.py`
- `python3 tests/async/test_ttype_runtime.py`
- regression-runner discovery/execution for both new files
- `make -C src -j2`
- `./scripts/format.sh --check`
- `git diff --check`

The original minimum of twelve high-value tests is exceeded. The remaining
work is broad-suite validation and a final completion audit; the name parser is
recorded as a strong candidate for a future expansion rather than being mixed
into this boundary-focused series without the same validation depth.
