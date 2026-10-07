# Primary integration of captured-item opening verification

The maintained origin reader now independently reconstructs the original
captured-item EBS2, ESN5 and EIC2 bindings. It preserves the existing empty
opening hashes and rejects missing, malformed, changed or foreign capture
evidence. The optional evidence input does not grant capture completeness,
selection authority, activation or release qualification.

This imports the completed independent slice from Plan5 commit `9328c5c3f`.
The exact executable bytes match that slice:

- `scripts/economic_sql_audit_origins.py`: SHA256
  `ef8ef860ac1bb69e5a5bd2127c4e12f6d6201d6ed957d45405beca26838dc4eb`.
- `tests/async/test_economic_sql_audit_origins.py`: SHA256
  `6c8bfc2d17a9c94b64b9c6f98a53138ace512cb663aba1b1fb5e84baee7311a8`.

Primary compares every existing test method's AST: all 46 original methods
remain exact, alongside six new methods. The reader's original preimage matches
the independent branch's base. Native code, schema, mutation owners and
activation gates are unchanged by this integration.

## Primary checks

`python -B tests/async/test_economic_sql_audit_origins.py` exits0: 52 methods
discovered, 49 pass and three original/added native integration methods skip
because this Windows invocation has no explicit Linux database qualification.
The five new pure methods execute. The run takes0.539 seconds.

`python -B scripts/validate_economy_accounting.py` exits0: 14 contract fixtures,
920 writer routes and2,876 candidate sites; `release_ready=False`. This is
contract validity, not runtime writer coverage. Whitespace validation passes.

The [independent qualification report](PLAN5_CAPTURED_ITEM_BINDING_QUALIFICATION_2026-10-06.md)
retains its separate109-method Linux result, ten authentic capture DTOs and
18 operator reads on both engines, including eight expected refusals. Those
native artifacts remain peer-attributed to their frozen inputs. They do not
qualify the private753-provider producer candidate, original capture provenance,
complete selection or activation. The six capture-authority/release flags stay
false.

## Remaining integration

The SQL composite-sweep, current flatfile envelope and locked/bounded flatfile
audit slices at Plan5 tip `ece7a6280` remain separately fetched for review and
integration. The private ANF2/ACT2 auction interface, combined producer/recovery
journeys, writer evidence matrix and full R1–R8 qualification remain unfinished.
No whole Plan or release gate closes with this reader capability.
