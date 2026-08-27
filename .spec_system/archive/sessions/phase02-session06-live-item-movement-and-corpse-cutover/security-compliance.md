# Security Compliance

- Pending movement storage is bounded to 1,024 entries and 128 copied context bytes per
  entry. It contains no player, object, room, corpse, or container pointer.
- Item subtrees are bounded to 12 entries and validated for one selected rooted subtree;
  destination parents cannot be members of the moved subtree.
- Owner and item identities are typed integers. Logs contain operation/site/error and
  item UID metadata, not descriptions, account secrets, raw SQL payloads, or credentials.
- Authoritative restore fails closed on missing database access, invalid owner types,
  malformed numeric identities, absent rows, inactive state, or hydration conflict.
- All database mutation remains in the critical worker transaction. The simulation
  thread captures scalars and publishes only an exact committed result.
- The corpse normalization wrapper requires both environment and database names to
  identify development/local/test before executing.
- The repository security/dependency baseline passes. No secret, credential, private
  key, production data, or environment file was changed or committed.
