# Task Checklist

- [x] Inventory listing, bid, buy-now, expiry, finalize, pickup, and notification routes.
- [x] Define bounded auction command/result codecs and entity/revision keys.
- [x] Add additive auction operation, revision, claim, ledger, and outbox schema.
- [x] Implement one transactional auction repository with canonical locks and replay.
- [x] Add pointer-free game-thread submission and exact completion publication.
- [x] Cut listing and live item custody onto the auction command.
- [x] Cut bid/refund/buy-now/finalization wallet effects onto the auction command.
- [x] Cut item and money claims onto one-time transactional commands.
- [x] Add guarded baseline/reconciliation and quarantine tooling.
- [x] Add focused concurrency, replay, failure, offline, and source-contract tests.
- [x] Run format, focused MySQL, build, security, full suite, review, and publish.
