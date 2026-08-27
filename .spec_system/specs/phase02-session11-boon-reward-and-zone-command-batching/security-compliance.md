# Security and Compliance

- Boon and zone commands are bounded, pointer-free, schema-versioned, and reject key,
  participant, option, flag, and result-shape substitution during decode.
- Zone participant IDs are unique, include the toucher, and reconstruct the exact
  player/zone fence set. The repository accepts at most 15 participants.
- Progress, shop, zone current state, alignment, immutable audit rows, inbox result, and
  outbox are committed under InnoDB row locks in one transaction.
- Redis calls, live characters, objects, groups, notifications, and reset events remain
  outside worker code and occur only after committed completion/outbox delivery.
- Failure logs expose redacted component/outcome classifications without character or
  account names, reward contents, raw SQL payloads, or credentials.
- Schema tools are additive or read-only. Test runners fail closed unless both the
  environment and database name explicitly identify local/development/test scope.
- No credential, private key, log, player/account data, archive, or generated world data
  was added or modified.
