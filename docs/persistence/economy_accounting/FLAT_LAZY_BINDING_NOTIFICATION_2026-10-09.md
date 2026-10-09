# Flat lazy binding notification — 2026-10-09

Ordinary `proclibObj_add` and the ITEM_SWITCH fallback install their original
procedure and then notify the retained template catalog. That notification
previously accepted only SQL mode. In flat mode, the catalog therefore retained
the old procedure and treated a genuine original transition as template drift.

The existing notification now dispatches actual flat mode to a private companion.
It requires the game thread, non-MySQL flat backend and original sealed catalog;
the actual index must already contain the reported new procedure. Only the
original bridge transition with its authenticated predecessor or null-to-switch
transition is accepted. VNUM, R_num, template position and the catalog's old
procedure must all match. The companion advances only that one catalog entry.
It allocates nothing, does not rewrite the native index or reseal the catalog,
and preserves the original SQL notification body and existing callers.

Independent finite source review traced both real post-binding notifications,
the predecessor lookup and sealed catalog checks. Exact source inverses,
changed-line clang-format 18/C++ token preservation and registry/matrix refresh
passed on the isolated published candidate. All 931 writer policies retain their
scope; 391 source pins are authenticated, with no new/unmapped lexical sites.
The pre-existing native flat factory draft remains unpublished and byte-exact.

Builds, gameplay, persistence and recovery testing remain in the major-plan
batch. This fixes the actual notification omission; it does not establish an
accepting flat ROOM route or replace runtime qualification. The genuine flat
reset cursor, retained backend/configured-root proof, warm publication/current
reader, once-only action checkpoints, durable terminal transfer and cold
reconstruction still need integration. Publication/submission bounds and full
native/restart qualification remain open. Accounting stays inactive, admission
CLOSED, coverage incomplete and release BLOCKED; Plans 2–4, combined Plan 5 and
R1–R8 remain unfinished.
