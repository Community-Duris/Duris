# Recovery template binding order repair — 2026-10-05

## Established ordering defect

The original complete SQL object-template catalog captured special procedure
pointers before studioproc_boot and initialize_ships changed native bindings.
Additional supported binding changes occur after boot_db returns, through mining,
ferries, guildhalls, kingdom and nexus initialization. Current lookups correctly
refuse a prototype whose native binding differs from the sealed pointer, so the
early cut incorrectly made these legitimately initialized prototypes unavailable.

Moving all catalog preparation to the end of startup would break populated SQL
keeper restoration: restore_shopkeepers -> sql_restore_shopkeepers ->
shop_item_runtime_keeper_image -> native payload verification already calls
find_recovery_object_template during boot_db. Empty/legacy stock cannot prove
that consumer's compatibility. Parsing twice would also duplicate the complete
native prototype scan.

## Two existing boot cuts

The original complete parsing block moves after SHLIB/ships initialization and
before Mail/corpse/SavedItems/keeper restoration. All other startup calls retain
their original order. Its recoverable SQL-only failure handling is unchanged.

A separate finalize_recovery_object_template_bindings noexcept function runs
after optional comm initialization and before worker startup/game_booted=true.
It refuses runtime, foreign-thread and flat-policy calls before any mutation.
Within serialized SQL boot it first validates the existing complete parsed
catalog's table/file/count, native R_num/VNUM/file position and sorted uniqueness.
Failure invalidates the whole catalog; no partial update occurs. Only after all
validation does a second pass copy each current native func.obj into its private
entry.special. Parsed prototype addresses, native indices, shared strings and
all other values remain exact. It does not parse, allocate, invoke callbacks or
write native index bindings. This snapshots existing startup policy; it grants
no UID, original economic source, cleanup or ACK authority.

## Evidence and remaining scope

Independent source review accepted raw three-file receipt
29542c752aca168f52cd7791ac0c83243feefa9cd16352c2d49e7c32457308b3.
Exact raw preimages, full forward/inverse and changed-line clang18 fixed points
pass. The integration normalization changes only the header's CRLF to LF; source
text is identical. The production nevent thread binding precedes run_the_game,
its declaration is already available, and extern bool game_booted matches the
maintained definition. Existing parser/readiness/lookup/instantiation bodies and
all other boot statements remain exact. Source pins, census and static checks
are recorded in the milestone receipt; no build/test/native/SQL/service or
gameplay/recovery execution ran. User major-plan batching remains.

The accepted cold SHOP comm candidate's separate driver delta composes with
these two new boot additions; its reviewed combined value is
f8573e2d73fa4ffbe9f0e87db3628a890d92fe1afa7074ca52ad94b7d8ccb8c0.
That cold owner is not installed by this slice. Early-restoration lazy mutation
can still stale a affected target, and missing postboot SWITCH/proclib binding
remains explicit. Neither is repaired by copying final boot pointers. Full
native sidecar restore, cold save/replay/ACK, flat parity and original major-plan
qualification remain required. Existing inactive behavior and declined spell
path stay. R1–R8, release and activation remain BLOCKED.
