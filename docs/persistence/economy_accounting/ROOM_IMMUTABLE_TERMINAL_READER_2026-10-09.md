# ROOM immutable origin and saved terminal reader

## Result and scope

The private flat ROOM publication storage can now authenticate an original
successful birth by its genuine root UID and read a separately saved terminal
BODY under that same recovered authority lock. Previously the ordinary origin
reader supplied structural origin values without the complete historical
receipt proof, and publication storage had no saved terminal reader.

The immutable reader verifies the canonical origin, original command and typed
intent, historical control and epoch, successful receipt and TIR48 result,
complete original compiler plan, source claim, operation-reference bucket and
original UID/result correlation. It does not require today's epoch, season or
current physical room to authenticate history. Those remain separate current
publication proofs. Missing origin remains `ENODATA`.

The saved terminal reader authenticates that full origin first, then securely
reads `domains/room_reset_terminal_<root_uid>.zrt`. The file contains exact
existing recovery BODY bytes. The new codec companion runs the complete
existing bounded decoder and original terminal predicate, and storage compares
the decoded receipt core with the authenticated original successful receipt.
Only genuine file absence returns `present=false`; malformed or inaccessible
storage refuses. Origin, core, BODY, context and transferred heap outputs stay
unchanged on failure.

These private values supply historical evidence only. They create no recovery
envelope, revision, coordinator generation, delivery or ACK, and grant no
constructor, source, native effect, current publication or accounting admission
permission. This milestone implements readers; it does not implement terminal
retention or authorize journal retirement.

## Source evidence

Changed source:

- `src/flatfile/flatfile_accounting_zone_reset_item_transaction.c/.h`
- `src/economy/zone_reset_item_recovery.c/.h`

All previous C source remains an exact byte prefix; header changes have exact
insertion inverses. Changed-line clang-format-18 checks preserve C++ tokens.
Independent review of the final four-file candidate passes for original ordered
authentication, private access, prospective overlapping storage, sticky scratch
refusal, receipt correlation and strong output transfer. Supported allocation
requests remain pinned to the existing libstdc++13 ABI; unsupported ABI refuses.

The existing registry retains all 931 writer policies and 393 authenticated
source pins. The clean candidate excludes separate native factory/source/binding
and unrelated WIP. Its lexical census remains 2,911 occurrences and 2,853 unique
sites, with zero new or unmapped sites. This metadata is source evidence and does
not establish complete accounting coverage.

Private source snapshots, inverses, format comparisons, review and candidate
metadata are in `tmp/room-terminal-reader-integrated-primary-20261009/`.

## Remaining acceptance work

No builds, native tests, gameplay, persistence or recovery journeys ran for this
successor; the user requested native testing at major-plan readiness. The older
`ec632155c` executable evidence does not qualify these new readers.

The actual flat warm/cold publication caller still needs integration, including
genuine source/factory/backend/root lifetimes, once-only effect checkpoints,
prospective transaction commit and durable terminal transfer before journal
retirement, ACK and complete aggregate storage bounds. The saved terminal file
has no writer in this milestone. Plan 5 must qualify its real eventual write,
independent audit, backup/restore and failure/restart behavior on the combined
candidate. No synthetic BODY or passive receipt substitutes for those journeys.

Accounting remains inactive, admission CLOSED, `coverage_complete=False`, and
release BLOCKED. Plans 2–4, combined Plan 5 and R1–R8 remain unfinished. The
primary goal stays ACTIVE.
