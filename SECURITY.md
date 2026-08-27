# Security Policy

## Supported Versions

Security fixes are developed on `master` and released in the current `1.81.x` line.
Older source snapshots and private forks are not supported with backported fixes.

| Version | Security support |
| --- | --- |
| Current `master` and latest `1.81.x` release | Supported |
| Older releases | Unsupported; upgrade before requesting a backport |

## Reporting A Vulnerability

Do not open a public issue, discussion, pull request, game ticket, or chat message for
an undisclosed vulnerability. Use the repository's enabled
[private vulnerability reporting form](https://github.com/LuminariMUD/DurisMUD/security/advisories/new).
Include the affected revision, impact, prerequisites, minimal reproduction, and any
suggested mitigation. Do not include real player data, credentials, private keys, or
production database contents; use fabricated test data.

The maintainers aim to:

- acknowledge a complete report within three business days;
- provide an initial severity/scope assessment or a request for more information
  within ten business days;
- give status updates at least every ten business days while remediation is active;
- coordinate a disclosure date after a fix or mitigation is available.

These are response targets, not a bug-bounty promise or guarantee. If GitHub private
reporting is unavailable, open a public issue containing only the words "Security
contact requested" and no vulnerability details; a maintainer will establish a
private channel.

## Scope And Safe Research

Reports about authentication, authorization, persistence integrity, injection,
credential or personal-data exposure, network protocol handling, dependency
vulnerabilities, and denial of service are in scope. Reports consisting only of
automated scanner output should explain reachability and impact.

Use only systems, accounts, and data you own or have explicit permission to test. Do
not test the production game, access other players' data, degrade service, perform
social engineering, or retain exposed data. Stop when enough evidence exists to
demonstrate the issue.

## Disclosure And Credit

Please allow a reasonable remediation window before public disclosure. The project
will coordinate advisory publication and credit reporters who request it. Security
advisories may omit exploit details until supported users have had time to update.

Repository scans and the dependency baseline are engineering controls, not assurance
that no vulnerability exists. See [the security baseline](docs/operations/SECURITY_BASELINE.md)
for scope, ownership, and current limitations.
