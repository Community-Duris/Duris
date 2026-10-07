# Enhancement module-boundary investigation — 2026-10-07

Disposition: **private investigation complete; exact one-file repair proposed**.
No maintained source/test/helper was edited. Root reviews the reservation before
implementation. The original maintained test still FAILS. Private source-boundary
feasibility PASS61 and28 rejected controls are separate from a direct-reentry
missing-config concern recorded RED. Neither is native-world execution, accounting
authority, a repaired maintained test PASS or a new release gate.

Owned starting revision `8d794f0c32b316f4d50ab0bf8dda7448c613b67d`.
Published result **`2188c2caa82bb8af670aa950c9df473a4c0b8240`** authors only
this investigation and HANDOFF.md, pushed to origin/codex/accounting-quest-prep.
This canonical successor adds delivery pins to those two owned docs only.
Fresh patchless primary **`0fd938ce2fbae7b7e675346d60b30b3867bea0f7`** publishes
the private assignment and final live-route review. Its advance from0f466967
changes four documentation paths only; no examined producer/test/helper changes.
The source export in this investigation is actually0fd938, not a relabeled0f
archive. No R11 or optional architecture patch was applied. The pre/post-R11
failure retained by root is context; this investigation executes the current
patchless original once and does not reopen prior packs or live-route jobs.
Publication-time fetch observes `6ca40d01652b29bd7faf475db2754ff628bdd228`,
changing four qualification documents only; examined src/tests remain unchanged.
SHARED_NATIVE_QUALIFICATION_PROGRESS_2026-10-07.md records a private shutdown/SHOP
candidate, actual SQL754 build and cache/tooling blockers, with flat/native journeys
and Plans2–5/R1–R8 still open. No fix is imported or native gate promoted. It exposes
no new quest lifecycle/capture/delayed-settlement interface; retain actual0fd pins.

## Complete original execution and predicate map

Actual command, cwd/candidate:

```text
python3 -B tests/async/test_enhance_module_boundary_contract.py
exit1,0.031060971086844802s, empty stdout
AssertionError at line29:
assert db.count("enhance_on_npc_item_reset_skipped(mob, obj);") == 2
```

Original test blob **`311f5292d17357c1a9d7197615b82901948e1b6f`**, SHA256
`33b75c4339e18fdaa2417f8baea3160ce62138839a6c35adef0cfddd72e3e944`, remains
untouched in both the owned checkout and canonical export. ORIGINAL.json retains
the complete traceback. DIAGNOSTIC.json independently evaluates all20 original
assert predicates:19 satisfied, one literal-count failure. That census does not
bypass an assertion in the original execution or make it PASS.

| Original predicates / lines | Intended boundary and actual source |
|---|---|
| Three header API declarations,18 | enhance.h declares boot_enhancement_system(void), enhance_on_eligible_npc_death(ch,killer), enhance_on_npc_item_reset_skipped(mob,missing_item). All present; proposed checks retain signatures and inspect actual definitions. |
| Three enhance.c definitions,20–22 | All present. Actual module owns boot configuration/index, eligible death selection and legacy skipped-item selection/grant. Function extraction avoids global-name-only evidence. |
| No enhancematload in fight; death hook present,24–25 | Both satisfied. Defined die(:1930) calls enhancement death API(:2051); enhancement definition(:2082) gates NPC/nonpet/positive XP and invokes module essence policy. |
| No load_npc_missing_item_material, no fallback-enabled policy, no get_matstart(missing_item) in db,26–28 | All satisfied. Reset driver remains dispatch/construction ownership. Shared selection policy is in enhance.c:2092; neither native dispatch nor DB constructor defines that policy. |
| Exactly two raw db hook calls,29 | Literal count0 due formatting. Existing code-whitespace helper counts exactly2. They are distinct legacy arms inside reset_zone G(:7491) and E(:7616), after ITEM_LOAD_CHECK failure. Each native arm calls native skipped_item instead. A global count alone cannot prove either branch or native policy owner. |
| No direct config/index loaders in comm; boot API present,31–33 | All satisfied. main(:534) calls run_the_game(:878), whose optional subsystem block under !mini_mode calls boot_enhancement_system(:1009). The wrapper owns config→index→ready order and returns immediately if already ready. |
| Five reload zero assignments,40 | All five present within defined load_enhance_config(:1577), before fgets parsing an opened file. Preserve each requirement separately. Mere presence does not prove clearing on a missing-file early return; that broader direct-reentry concern is separate below. |

The exact literal-count failure is whitespace, but a sound repair needs the real
branch/owner chain. Simply substituting a global whitespace-insensitive symbol
count would miss native dispatch and could accept both legacy hooks in one branch.

## Actual skipped-item owners and publication binding

The following bindings refer to canonical0fd938 source:

- db.c reset_zone:6938 selects native regular SQL reset and requires the same birth
  owner for native G/E/P item routes. G/E skipped-load branches each choose native
  skipped_item or legacy enhancement hook, then discard/extract the skipped
  candidate. G preserves its existing shopkeeper exception. Earlier artifact
  refusal/discard branches are different paths and must not substitute for the
  ITEM_LOAD_CHECK skipped branch.
- quest_mobile_native_birth_owner::skipped_item:916 first requires owns(mob)
  and quest_mobile_native_reset_material_owner::select(mob,missing,&vnum). The
  selector's actual definition is enhance.c:2101, forwarding to
  original_npc_reset_material:2092. That shared policy requires explicit stat and
  fallback opt-ins, nonnull inputs and NPC, then selects get_matstart(missing)+4.
- Native skip resolves real_object(vnum); a missing rnum logs/returns before
  staging. It calls prepare_item(rnum), then carry(material,mob) only if material
  exists. It does not call the legacy enhancement grant or ordinary read_object.
  owns:721 identifies the unconsumed original born mob. prepare_item:781 allocates
  the UID, prepares the actual native item stage and retains it in original stock.
- carry:850 delegates to quest_mobile_native_local_stock::carry in handler.c:7147;
  failed placement blocks the birth. Local projection requires a detached newborn
  and exact nonnull/unlinked/NOWHERE UID-bearing candidate before grouping and
  setting LOC_CARRIED/mob custody. This is staged local stock, not durable admission.
- Native birth publish:2103 requires original successful completion, its source/
  custody cut and image agreement, then retains admission before ITEM_PUBLICATION
  and item.stage->publish. Actual stage publish in db.c:6333 checks admission,
  original UID/prototype/current bindings and consumes its original stage before
  object-list linkage. These source obligations do not prove runtime publication,
  native source/custody/ACK authority or faulted tails.
- Legacy enhance_on_npc_item_reset_skipped:2106 uses the same original selector,
  then ordinary VIRTUAL construction and obj_to_char within enhance.c. Its policy
  is shared with native selection; its construction/grant path remains distinct.
- world_recovery_npc_items.c has an independent hook in load_recovery_object:59,
  after ITEM_LOAD_CHECK failure and the shopkeeper exception, before extraction.
  Both rehydrate_carried_item:88 and rehydrate_equipped_item:108 call that loader.
  It is not either reset_zone G/E site and is not evidence of native birth authority.

The private checker inspects20 complete definitions, both actual G/E case and
skipped-load regions, the independent recovery region and original whole-unit
policy prohibitions. Existing `_paths.extract_function` and `contract_text` are
reused unchanged; no shared parser/framework or accepting provider is introduced.

## Reload concern and precise reachability limit

load_enhance_config opens lib/enhance.cfg at:1587; its !fp branch returns at:1592.
The five zero assignments occur at:1599–1603. Thus **direct reentry** with previous
nonzero masks and an unavailable file returns before all five clears. This is an
actual source control-flow witness, not an executed reload or current staff exploit.

The production call scan finds only one direct config-loader call: within
boot_enhancement_system in enhance.c. That wrapper checks enhancement_system_ready
before loading. The flag initiallyfalse becomestrue after config/index; subsequent
wrapper reentry returns immediately. All five masks initiallyzero. comm and chaos
invoke the guarded boot API; chaos also checks enhancement_system_is_ready.
No current staff reload/direct reentry caller was found. First missing-file boot
cannot inherit previously configured nonzero masks through this observed path.

The existing test checked assignment presence, not this missing-file branch. Keep
all five original reset predicates and strengthen opened-file parsing order, while
preserving this concern independently. A test-only route repair cannot claim it
fixed. It does not authorize a production correction or silently add a release
gate. Any future reload contract/path needs an explicit owner decision and actual
execution evidence. No current native accounting prerequisite is discharged.

## Private feasibility and sensitivity results

Final probe.py reports **PARTIAL**:61 declared source-boundary checks PASS,
direct-reentry source witness separately RED. Controls run that entire private
checker against actual copied current inputs and unchanged helpers in a temporary
source fixture, with only ROOT/OUT directory literals relocated. All28 selected
guarantee removals rejected; harmless multiline formatting PASS. The counterexample
putting both legacy hooks in G while preserving total2 is rejected by branch
checks. The five reload clears each have their own removal control.

Other controls remove declaration/definition/death/boot/recovery hooks, leak policy
into fight/db/comm, remove native G/E dispatch or same-owner/material-kind refusal,
bypass the staged constructor/module selector, change the material offset, drop
stock retention/detached guard or publish before the original source cut. These
are source counterfactuals, not compiled/native-world operations. Restored fixture
inputs reauthenticate after each control; temporary directory is confirmed removed.

Two initial checker failures are retained in DESIGN-DIAGNOSTICS.json with exact
program pins: first incorrectly selected main for the boot call; second used the
first artifact discard in the entire G case instead of the later skipped-load
branch. Both are checker definition/branch-selection errors, not production
failures. Their programs remain preserved. The final checker follows run_the_game
and actual skipped-load regions without weakening an original requirement.

Root independently executed the earlier stable60-check draft
SHA337472f140828d980778e34a993c4beabe77b618faba0a32456af46b254817dc in
/tmp/coordinator-enhance, preserving assertions/helpers and authenticating all20
definitions against0fd Git source. It reproduced the separate direct-reentry
witness. That draft is retained as reviewed-60-probe.py; the final61-check version
adds explicit missing-material return/staging order proof and has its own pins/
controls. Root now also reports independent **final61/28 PASS**, harmless formatting
and cleanup, all20 definitions canonical0fd, all13 sealed receipts/programs,
all11 actual input hashes, all6448 archive bodies/modes/links and independently
executed original FAIL plus20-predicate census authenticated. Its first sensitivity
run was interrupted(exit137) by the normal worker container stop after sealing;
root preserved that lifecycle overlap and completed the repeat in its own isolated
container. It is not a production/test failure and required no worker restart.
Keep final61 results separate from337/60. Formal one-file boundary approval remains
pending this canonical proposal/handoff. Independent evidence is coordinator-owned;
the earlier checkpoint record is enhance-feasibility-review-RESULT.json.

Actual private commands:

```text
python3 -B tests/async/test_enhance_module_boundary_contract.py
docker exec quest-prep-enhance-0f466 python3 -B /evidence/original.py
docker exec quest-prep-enhance-0f466 python3 -B /evidence/probe.py
docker exec quest-prep-enhance-0f466 python3 -B /evidence/controls.py
docker exec quest-prep-enhance-0f466 python3 -B /evidence/seal.py
docker stop quest-prep-enhance-0f466
```

original.py executes the maintained inner command once, then the independent
census. The probe command includes the retained two failed design attempts and
successful drafts; these do not increase case counts. Final authentication covers
all6448 canonical file/link bodies/sizes/modes/targets before/after, unchanged,
and no unexpected file/link. Container is stopped: unchanged image
sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b,
networknone,2CPU,3GiB. Only Python/source inspection and sleep; no compiler,
DB/player/server/native-world operation or new backend/binary/schema qualification.

Ignored proof location (directory name reflects initial0f observation; actual
executed export is separately named current-primary-0fd938.tar and pinned0fd):

```text
C:\Users\alexa\.codex\worktrees\accounting-quest-prep\NewDuris Max\bin\tests\enhance-boundary-0f466-20261007
```

| Package / input / receipt | SHA256 |
|---|---|
| current-primary-0fd938.tar | `4ebbd97e08c30223ef577db219a83df5b433c590524d2c151fd8e0f53c8678bd` |
| src/item/enhance.c | `dc50a3fc114c29d63cba855f1e3c790420724274911693a01bf971717aa60f12` |
| src/item/enhance.h | `64c7a481af537d453c6c2ba745cee0aa1b8526170b81b31b26de3686385f7443` |
| src/world/db.c | `47db8a88e7b5abc97b698a29b0224492a6311a077a2f9bb4b3b6c49aa49b6d40` |
| src/world/quest_mobile_native_birth.c | `95cb42c46e9fd0cc54363a2ba8ff6171cec4f7ca232cf41013e0c28b2c4597de` |
| src/world/handler.c | `3f046194166dbbfd949f9ec0199e72f6155283edbdac69a7fd186da21333ce07` |
| src/world/world_recovery_npc_items.c | `d43d27e3a3df6bf938ab1f9d5eb4f08111e2e8e4d719e155fe7f39296940c67e` |
| src/combat/fight.c | `400b6ab87213b6b0526632e014ffb1781624d94d67199957aa11add8b1a4614f` |
| src/net/comm.c | `b7c2f1cc4a91cbe30f0a06151e7795635546c6bf833ace42ec7901ba67910b89` |
| src/combat/chaos.c | `673591cd7d6c765cd2d6ee9863a2afdf3e2e4a5f922b8f1db2bada97c3d6105e` |
| tests/async/_paths.py | `bb1ffca804d125835894427e22ef3b286e3555e8ab9af75978a817bef00958e6` |
| tests/async/contract_text.py | `65202fb525855c647053c98eded3204d332b9e747faa29deeb1ad1759439d9a5` |
| ORIGINAL.json | `292548f9fb4b156dae90d45d109e5b796eea8f2ba5aa1974094012b61ab67eb6` |
| DIAGNOSTIC.json | `269ab01f65ebd9b0549c1e7fcb4b180f5327d1eda06230d16cd1278f9bd9c5d8` |
| FEASIBILITY.json | `eb3805f838999ea69b1791541e650177a58c2983a6d4e151fe2a90a09f708a1d` |
| FUNCTIONS.json | `8a1aaf0d54762c058e288f5ec1a1e1c5a53455e3710b806e65a0b7da672ad3ae` |
| CONTROLS.json | `1e34d9a2c9ff74114e6927ee48b10a4a11bd94c4888e0055bce6431c0b2d2839` |
| DESIGN-DIAGNOSTICS.json | `768187200095f3bb18ed8760872ab118b4f4548856d5c57699ed979e8f845623` |
| Final probe.py | `3decd22930588b142279cc47b2c5338ad908a24fb52f225eb1f90c20a6304aa4` |
| controls.py | `02ec31473fb7f9e7680bc6455b8918b4323975f33d8dfa7b66ff9b5ca20dacdf` |
| SEALED.json | `777a93e95a20443013fa0a832cbfb317624a2fb727ebcc07b2e5cbdd51ea1602` |

SEALED.json pins all13 receipt/program files, complete input hashes and definition
lines. Full canonical inventory SHA256 is
`539645b104ad18b49a0933071130c28d3f79ecaf0da26ea320ff49349fba546e`.
Raw source exports/proofs/private scripts are excluded from commits.

## Exact prospective reservation and remaining work

Reserve **tests/async/test_enhance_module_boundary_contract.py ONLY** if root
approves implementation. Reuse existing helpers unchanged; retain all20 original
semantic predicates, replacing the obsolete raw count with exact2 code calls and
actual one-per-G/E skipped branches, native/legacy dispatch and enhancement policy
forwarding. Follow the defined death/boot APIs, same native owner, kind refusal,
staged constructor/stock/local carry/publication obligations and independent
recovery hook. Keep five scoped reload clears before opened-file parsing and the
direct-reentry concern explicitly separate. No production/helper/framework/schema/
authority/Plan5 change or strengthened release gate is selected.

No maintained repair or patch exists yet; fresh export is patchless. After boundary
review, qualify the committed one-file import and full maintained entry point with
the current input pins and equivalent guarantee-removal controls. This proposal
cannot claim missing-file concern fixed or native stock/publication authority.

Live-route final review is now remotely published at0fd938; codefd4563fab remains
independently importable. Primary adoption is unknown. Actual continuing quest
Goal remains BLOCKED on native lifecycle/verifier/reset-born source/custody,
cost/publication/ACK capture, supported delayed bartender settlement, durable
held-charge/refund and pre-ACK move/paired retirement. All prior evidence/pins
remain preserved; this finite adjacent investigation does not complete the goal.
