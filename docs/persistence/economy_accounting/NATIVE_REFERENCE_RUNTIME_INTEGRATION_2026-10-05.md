# Native reference codecs and runtime observation integration

The original NPC lifetime owner needs to retain and observe the existing native
reference without importing world-capture globals into its value codec. The
maintained 148-byte reference codec depended on source-event functions in the
full accounting-plan unit. This slice extracts both existing codecs, supplies
the complete production/focused link closure, and adds runtime-only reference
storage plus a read-only observation API. It does not install a birth writer,
issue an identity, activate an NPC route or establish durable NPC recovery.

## Implemented source

`economic_source_event.c/.h` contain the existing 48-byte source-event codec.
`quest_mobile_native_reference.c/.h` contain the existing 148-byte reference
codec. Their original values, layouts, versions, endian rules, checksum and
refusal outputs are preserved. Old headers retain their compatibility includes;
native image/capture policy is unchanged. Production Makefile and 63 direct
compile recipes now link the required extracted units without changing test
cases, assertions, compiler flags, sanitizer policy or resource budgets.

`char_data` gains a 148-byte trivial, zeroable private runtime binding. Its
read-only accessor requires the caller's originally observed runtime ID and
checks its live registry association before dereferencing the character. A
reused pool address cannot inherit the old caller's reference. Canonical decode
and reencode, NPC type validation and a final-only output assignment preserve
refusal behavior. Only the original birth/restore owner is declared as a future
binding writer; no public setter or prototype-derived identity is added.

Existing pool allocation/reuse and `clear_char` zero the complete character;
the pool derives its size from `sizeof(char_data)`. Existing player codecs and
the separate `copyover_mob` record remain unchanged. Consequently the new
runtime bytes are not restored by existing saves/copyover. Explicit durable
reference recording/restoration and the original birth, revision and retirement
owner still require implementation and qualification. Runtime size/layout and
actual lifecycle behavior have not been measured.

## Source review and correction

The initial extraction accidentally placed the extra source inside eleven
single-argument `rel()` calls. Independent review found the concrete TypeError
before installation. A separate immutable successor splits these into two
calls. All 470 `rel()` calls across 51 Python recipes were then checked against
the actual one-argument helper. Corrected recipe changes reverse exactly to
their maintained preimages; the initial defective proposal stays historical.
The other twelve recipes are shell source-list edits with their original bytes
and flags preserved.

The accepted correction receipt is
`7c3995a34c125f45ee79fe6eee792e58e8006e11ff1d9eb8880aae4591c50a59`.
The corrected actual-before patch is
`fd429033c7f46ffa49565b2aabe50898f00fedb897f83ce9bff50d8f27e522c3`.
Independent architecture review accepted the combined 72-file extraction after
checking all 78 parent and 16 successor artifact hashes. The three runtime
binding files also passed independent source/lifetime review. Root integrates
these 75 files together and adds the binding object registration. All 69 tracked
source-provenance inputs use explicit LF checkout policy. No writer coverage or
release status is promoted by these changes.

Changed-line formatting reached a fixed point; raw preimages, inverse source
changes, Python AST, shell syntax views, source pins and central manifest
coverage were checked. No builds, tests, native execution, SQL, services,
gameplay or migrations ran. Existing peer child-identity qualification remains
bound to its frozen pre-extraction sources; compile-list changes require fresh
major-plan execution when that batch is ready.

In parallel, the private cold shop reader now supports manifest-bound v8
publication values without an old in-memory checkpoint token and verifies the
exact historical receipt under the same SQL cut. That source-reviewed seam is
not installed here and adds no world publication or ACK authority. Cold shop
startup/publication, flat parity, coherent migrations, native birth/restore,
remaining Plan 2–4 writers and full R1–R8 qualification remain unfinished.
