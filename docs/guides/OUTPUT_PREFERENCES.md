# Character output preferences

Display preferences belong to the character controlling the recipient, including
switched bodies and morphs. NPCs without a controlling character use defaults.
Changing one character's settings affects that character's future output only.
Preserve bypasses added styling and retains authored ANSI. An inherited choice
uses the channel's configured presentation; missing configuration preserves legacy
output. A supported explicit foreground can style an adopted channel without a
word dictionary. A caller that explicitly requests Preserve remains protected.

`OutputProfilePreferences` supports inherit, Preserve, Static, Animated, named
foregrounds from the shared palette, reset-one, reset-all, and global motion-off.
Motion-off restricts Animated to Static. The active preference state is a small
plain value in `pc_only_data`, so the existing pooled/calloc allocation is safe.
Animation sequence counters belong to the descriptor and are never persisted.
Reconnect starts fresh counters while retaining the character's choices.

## Persistence and failure behavior

The preferences service requests the existing asynchronous STATUS checkpoint.
Queued/coalesced updates take effect in memory for future messages. The result is
**pending save**, not a claim of completed durability. An admission failure restores
the previous in-memory preferences; no-op updates request no additional write.
The normal worker, revision guard, journal replay, and retry semantics determine
durability. Rendering performs no database access.

The compact representation is empty for defaults, or `v1` followed by independent
semicolon-delimited fields, for example `v1;m=1;1=3;12=27`. Numeric channel IDs come
from `net/output_channel.h`; values 0–3 mean inherit/Preserve/Static/Animated,
and supported foreground IDs 17–31 use the shared ANSI palette. `m=1` prohibits
decorative motion. The maximum stored representation is 512 bytes. Unknown
versions or oversized values become defaults; invalid, obsolete, or duplicate
fields reset only the affected known preference. Stored data cannot inject ANSI.

Migration `0014_output_preferences` adds `player_data.output_preferences`
(`VARBINARY(512) NOT NULL DEFAULT ''`) through a guarded, repeatable ALTER. Existing
characters inherit defaults. The migration manifest, migration history digest,
runtime schema manifest, and compiled compatibility constants advance together.
Metadata fingerprints were measured on isolated MySQL 8 and MariaDB 10.11 schemas.
No production migration was executed during implementation.

SQL checkpoint and load repositories carry the field in the existing status query
and transaction. Legacy SQL character load/save paths also carry it. Flatfile
STATUS merges replace it; unrelated component merges retain it. The player
snapshot wire versions are 5 (normal) and 6 (death). Readers still accept versions
1–4 with empty preferences, including their original journal envelopes; old
executables cannot read the new snapshot versions. Rollback therefore requires a
compatible reader or restoration of pre-upgrade snapshot/journal files, in addition
to the normal deployment backup process. The additive SQL column can remain.

Preferences follow the existing `database:player_data` lifecycle entry, associated
character snapshots, journals, backup, recovery, and export/erasure policy. They
contain display choices only, no credentials, message content, or independent
identity. This change creates no new retention store or policy decision.

## Validation

Run `python3 tests/async/test_output_preferences.py` for the preference codec,
ownership, reset, motion, admission, and reconnect contracts. Existing snapshot,
journal, pet restore, and flatfile repository tests cover the extended saved state
and compatibility with previous normal/death snapshots and journal records.

`tests/async/run_output_preferences_mysql.sh` creates and removes a disposable
database without reading `.env`. Set `COLOR_DB_IMAGE=mysql:8.0` or
`COLOR_DB_IMAGE=mariadb:10.11` to test both supported engines. It checks independent
characters, missing data, reconnect, idempotency, revision ordering, rollback,
reset-one/reset-all, and invalid stored values against the actual repositories.
