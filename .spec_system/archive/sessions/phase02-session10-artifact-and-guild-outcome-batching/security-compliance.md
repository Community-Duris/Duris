# Security and Compliance

- Artifact/guild commands are pointer-free and bounded to 15 artifacts. Decode rebuilds
  the actor, guild, and artifact lock set plus every expected revision, preventing fence
  omission or substitution.
- Deterministic child IDs derive from the parent operation and actor PID. The actor
  player key orders child execution behind its epic/combat parent and the database
  foreign key prevents effects without a committed parent inbox row.
- Repository apply locks compatibility and authoritative rows, validates exact timer,
  bind, and revision state, and commits current values, immutable ledgers, inbox result,
  and outbox in one InnoDB transaction.
- Workers receive only typed scalar values. Live characters, objects, groups, guilds,
  Redis calls, and notifications remain on the game thread after exact completion or
  durable outbox delivery.
- Failure logs expose component/outcome classifications without player names, account
  names, artifact descriptions, raw SQL, or credentials.
- Schema, baseline, reconciliation, and verification tools are additive or read-only;
  the mutation tools require an explicit local/development/test environment and database.
- No credential, private key, log, player export, archive, generated world data, or local
  environment file was added or modified.
