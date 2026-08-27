# Implementation Summary

Locker custody now uses stable numeric locker and chest identities for public floors,
sorting wrappers, and private chests. Player deposits and withdrawals reuse the live
item ACK boundary; temporary room and chest objects never become durable ownership or
topology identities. Exact restore checks fail closed against current-owner authority.

Locker snapshot and terminal paths are fenced while movement is pending. Copyover also
drains the final locker generation after critical completions and terminal player saves.
The guarded, rerunnable normalization maps legacy public rows to public chest context and
reconciliation reports zero mismatches.

Project version: `1.81.36`
