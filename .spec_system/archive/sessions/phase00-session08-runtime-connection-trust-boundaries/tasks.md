# Task Checklist

**Session ID**: `phase00-session08-runtime-connection-trust-boundaries`
**Total Tasks**: 16
**Created**: 2026-08-27

---

## Inventory And Design

- [x] T001 Confirm Session 08 selection, clean base `767c7909`, and local context without reading secrets.
- [x] T002 Inventory compiled defaults, environment loading, four MySQL constructors, listener binds, and certificate fallback.
- [x] T003 Define explicit-role, allow-list, protected-transport, session-contract, and local-certificate invariants.

## Configuration And Database Trust

- [x] T004 Remove compiled credential/database defaults and validate required runtime fields and strict port.
- [x] T005 Validate `.env` regular-file ownership and 0600-or-stricter mode before reading.
- [x] T006 Require the resolved host/database pair in an explicit allow-list while retaining production-port protection.
- [x] T007 Add canonical connect/read/write deadlines and disabled reconnect.
- [x] T008 Require CA-verified enforced TLS and negotiated encryption for non-local transport.
- [x] T009 Set and verify charset, UTC, READ COMMITTED, and strict SQL mode.
- [x] T010 Route main, pool, child, and legacy persistence connections through the canonical constructor.

## Listener And Documentation Safety

- [x] T011 Apply explicit numeric listener binding to telnet/TLS and WebSocket sockets.
- [x] T012 Restrict tracked localhost certificate fallback to explicit loopback local mode.
- [x] T013 Require a restrictive operator-owned private key for network TLS startup.
- [x] T014 Update example and setup/configuration guidance without deployable credential defaults.

## Tests And Completion

- [x] T015 Add and run focused fixtures/contracts, formatting, warning-as-error build, and full suite.
- [x] T016 Complete review, repair findings, and validate the session.

## Completion Checklist

- [x] All 16 tasks complete
- [x] No outstanding blocker or unresolved failure
- [x] Review and validation complete
