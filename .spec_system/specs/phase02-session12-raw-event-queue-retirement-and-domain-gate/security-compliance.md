# Security and Compliance

- The session-audit command is bounded, pointer-free, schema-versioned, reconstructs
  its player key during decode, and excludes IP, hostname, account, character name,
  client name, and raw SQL.
- Legacy fallback files are accepted only as explicit regular non-symlink paths. The
  tool reports hashes and record-class counts; quarantine is opt-in and permission
  restricted.
- Unrestricted SQL execution and replay fail closed. Typed Phase 01 and critical-command
  journals remain the only active durable work carriers.
- Reconciliation and schema runners require both environment and database names to
  identify local/development/test scope.
- Security source/configuration and dependency baseline gates pass. No credential,
  private key, log, player/account data, archive, or generated world data was added.
