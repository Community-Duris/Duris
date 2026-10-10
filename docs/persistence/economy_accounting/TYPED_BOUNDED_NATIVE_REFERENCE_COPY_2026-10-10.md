# Typed bounded native reference and indexed copy - 2026-10-10

The existing bounded native-reference codec flattened validator/hash reserve
refusal and checked-add overflow into invalid_value. Its one-shot SHA path also
hid the heap context needed by the complete publication resource contract.
An additive typed validator now preserves limit_exceeded for checked overflow,
allocation_failure for the existing unsupported profile or reserve refusal,
and invalid_value for genuine malformed value/checksum/hash failure. The public
boolean validator remains compatible; no new result enum or errno contract is
introduced. The complete original 148-byte framing, checksum/source ordering,
version/provenance laws, canonical validation and strong output tails remain.

Fixed SHA256_CTX uses complete authentic Init/Update/Final, compression,
memory/cleanse and constant-time comparison source inventories. Matching full
implementation/entry profile guards include LP64/SHA widths and OpenSSL's
no-deprecated configuration; suppression is scoped to real low-level calls.
Source storage is derived from actual declarations and simultaneous scopes.
Emitted/transitive storage and the full global 32MiB bound remain unqualified.

The new bounded binding-copy observation preserves the real game-thread,
indexed generation and NPC/prototype proof before accessing binding bytes.
It decodes and canonicalizes the complete reference through these genuine
bounded providers, retains its actual workspace and transfers only on success.
Original unbounded binding algorithms remain byte-identical. invalid_value
does not prove absence: the actual cold owner separately checks all private
binding bytes for zero. One narrow stage friendship permits that observation
without a public absence token or new authority; malformed nonzero bindings
still refuse. No identity, wallet, custody, birth or replay authority is minted.

Independent RAW review authenticates 47-member codec manifest 02b2b723,
31-member binding manifest 680ee672, four whole forward/inverse pairs, four
genuine current preimages, all 13 repository dependencies, the sole stage
friendship insertion and fresh reads of 32 captured installed headers.
Final installed source confirmation covers formatting/token/logical-PP
preservation and complete inverse/material provenance. Evidence is in
tmp/native-reference-integrated-20261010 and its reviewed root join.
The registry remains at 458 pins with four existing source pins refreshed;
931 writer policies and all mapped sites remain unchanged. Protected Plan 5
edits remain excluded.

Caller selection and the native image checksum successor are separate real
integration work. No native/build/gameplay/persistence/recovery tests ran for
this slice; qualification remains in the major-plan batch. Full producer/ROOT/
global-budget/recovery/R1-R8 qualification remains OPEN. Accounting stays
inactive, admission CLOSED, coverage incomplete and release BLOCKED. The
goal remains ACTIVE; this source milestone completes no major plan or gate.
