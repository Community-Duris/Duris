# Versioning

DurisMUD follows [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html).
The current project version is `1.81.46`. The version shown at the top of the
root [README.md](../README.md) is the canonical version marker.

The repository's long development history informed the initial `1.81.8`
baseline. It does not create a formula between Git commit counts and release
numbers. Future versions communicate compatibility according to the rules
below.

## Version format

Normal versions use `MAJOR.MINOR.PATCH`:

- `MAJOR` changes for an incompatible change to the compatibility surface.
- `MINOR` changes for backward-compatible functionality.
- `PATCH` changes for backward-compatible fixes and maintenance.

Incrementing `MAJOR` resets `MINOR` and `PATCH` to zero. Incrementing `MINOR`
resets `PATCH` to zero. Published version contents are immutable; a correction
is released under a new version.

Pre-release identifiers may be appended for test builds, such as
`1.82.0-alpha.1`, `1.82.0-beta.1`, or `1.82.0-rc.1`. Build metadata may be
appended with `+`, for example `1.81.8+git.abcdef0`; it does not affect version
precedence.

## Compatibility surface

DurisMUD is an application rather than a reusable library. For versioning,
its public compatibility surface consists of interfaces on which players,
operators, integrations, and world builders may reasonably depend:

- player-facing Telnet, TLS, WebSocket, and GMCP protocol behavior and payloads;
- persisted player, account, world, and recovery data, including the supported
  database migration path;
- documented configuration variables, process controls, and operational script
  interfaces;
- documented area, object, trigger, and help-source formats used by builders;
- documented integration contracts, including authentication and external
  service boundaries.

Internal C/C++ functions, source-file organization, undocumented implementation
details, and ordinary content or balance adjustments are not compatibility
interfaces by themselves.

## Choosing the next version

Use the highest-impact change included since the preceding release:

- **Major:** remove or incompatibly change a supported protocol, payload,
  persisted-data format, configuration contract, operational interface, or
  builder format. A change that requires coordinated consumer updates or lacks
  a supported compatibility or migration path is normally major.
- **Minor:** add backward-compatible gameplay or operational capability, extend
  a protocol or format compatibly, add a configuration option, or deprecate a
  supported interface while leaving it functional.
- **Patch:** make a backward-compatible bug, security, performance,
  documentation, test, tooling, or internal refactoring change. A database
  migration may be patch-level when it only supports such a fix and preserves
  the documented migration path.

When a change is difficult to classify, judge it by the work imposed on
players, operators, integrations, and builders, not by the size of the diff.

## Release checklist

1. Review all changes since the preceding release and choose the highest
   required version increment.
2. Update the canonical version marker in the root README.
3. Record user-visible behavior, migrations, deprecations, and upgrade actions
   in the release notes.
4. Run the validation appropriate to the changed code and data.
5. Commit the version change and tag the validated release as `vMAJOR.MINOR.PATCH`.
6. Never move or reuse a published release tag; issue a new version instead.
