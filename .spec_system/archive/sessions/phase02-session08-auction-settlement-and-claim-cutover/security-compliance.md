# Security and Compliance

- Commands use bounded binary codecs, stable operation IDs, normalized entity locks, and
  exact expected revisions; decoded payloads cannot omit or substitute declared fences.
- Account names, player names, item text, metadata, and blobs are length-bounded and SQL
  escaped before repository use. Numeric input is range-checked before copper conversion.
- Same-character and same-account bidding remain rejected. Only trusted live callers can
  submit removal commands; the repository rejects non-open or legacy custody rows.
- Listing, wallet, claim, ownership, inbox, ledger, and outbox changes share one InnoDB
  transaction. Notifications cannot precede commit and outbox delivery is consumer-deduped.
- Existing ambiguous auctions are quarantined rather than assigned invented item identity.
  Guard scripts reject non-local/non-development environments.
- No credential, private key, log, player export, archive, or generated world data was
  added or modified.
