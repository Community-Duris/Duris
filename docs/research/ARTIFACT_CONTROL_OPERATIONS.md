# Artifact control: operator interface contract

Proposed behavior, not currently available commands. Implements R03–R07 and R09–R10 in [the specification](ARTIFACT_CONTROL_SPEC.md). All examples use fictional revisions/request IDs and must be tested on disposable game state before operational use.

The shipped pilot catalog is legacy-only by default for every player, wild-NPC,
and controlled-NPC holder. The telegraphic examples below describe explicit
canary drafts and do not imply that the modern adapter is active in the baseline
revision.

## 1. Permissions and common behavior

Default permissions map to existing ranks: `LESSER_G` (59) can inspect definitions/status and view nonsecret diagnostics; `GREATER_G` (60) can create/edit/validate drafts; `FORGER` (61) can publish/rollback, spawn, pin instance modes, and perform typed lifecycle repairs. A greater-god's existing `artifact timer` right is retained through the compatibility command bridge. Account/character checks must reject NPC impersonation, untrusted script invocation, and authority lost while a menu is open. Recheck permission on every mutation and asynchronous commit; draft creation does not permanently grant publish rights.

Fine-grained capabilities may restrict these defaults: `artifact.read`, `artifact.draft`, `artifact.publish`, `artifact.placement`, `artifact.lifecycle`, `artifact.instance`. Only deployment/security configuration can grant capabilities; a balance draft cannot grant its author more rights. SQL and CLI principals map to equivalent capabilities. Mortal `artifact` listings and holder-only `itemmana` remain compatible.

Read-only operations never instantiate a real object, reserve rarity, change mana, or issue world SQL from the input loop. All write responses are asynchronous: `queued <id>`, then a completion notice; `status <id>` always recovers the result. A disconnected operator's durable request still completes, and the next login can inspect it. Retrying an ID never performs the action twice.

Use one explicit Publish action after review; do not prompt repeatedly for ordinary draft changes. Destructive lifecycle operations use one confirmation tied to the exact preview hash/revisions, target UID, and operation type. If the preview becomes stale, return a conflict and require a fresh preview. Saving a draft is never publishing it.

## 2. Workbench entry and navigation

Add `artifact control` (short alias `artctl`) as the unambiguous new command namespace. Preserve all existing `artifact` subcommand behavior. `artifact edit <vnum|name>` opens the new workbench as an additional shortcut. Do not hijack the old `artifact reset` spelling for spawning.

Requirements:

- Plain Telnet is the baseline: numbered selections, short text commands, useful labels, 78-column layout at 80-column terminals; stack fields at widths below 78.
- Optional ANSI colors aid reading but carry no unique meaning. Color-off output has identical information. Do not require MXP, GMCP, web UI, mouse clicks, or cursor positioning.
- Page size defaults to 12 results and honors existing pager/terminal preferences. Search by partial name, vnum, category, callback, mode, load zone, draft state, or unsupported capability.
- Every screen shows breadcrumb, current artifact, catalog revision, draft ID/generation where relevant, and whether values are LIVE or DRAFT.
- Persistent footer: `number/name = open; /back; /home; /help; /exit`. `/undo` and `/redo` apply to current draft operations. `/find <text>` preserves current draft.
- Unknown input does nothing except show short contextual help. No numeric menu input leaks into a game command or vice versa.
- `!<game command>` explicitly routes through the ordinary permission-checked interpreter. Ordinary combat, room events, pager, chat output, and disconnect remain safe. Menu use grants no invulnerability and no combat advantage.
- Before entering, reject nested string editor conflicts or offer to finish the existing editor. Existing pager/string-editor priority is retained. Add a dedicated artifact-menu input route only after the higher-priority pager/string editor and before ordinary playing commands; label its latency metrics. No mutation of nanny/account-creation routing.
- Text fields use the existing string editor with escaped/untrusted output. On returning, redraw the artifact screen without duplicating the prompt.
- Draft autosave is asynchronous with `Saving...` versus `Saved generation N`; navigation does not discard unsaved memory. Explicit `/exit` offers Save draft / Discard unpersisted edits / Stay. No promise to restore keystrokes never durably saved.
- Link loss saves already submitted draft edits; reconnect lists recent drafts and supports `artctl resume <draft-id>`. Copyover does not restore a borrowed pointer/menu callback; reconstruct from durable draft IDs.
- After 15 minutes idle, close the menu after saving acknowledged draft state; retain drafts until explicitly archived. Active published revisions/audit are protected state; mutable draft cleanup is a separately documented retention policy.

### Home example

```text
ARTIFACT CONTROL                         Applied r42 | SQL healthy
Search: [all]                            169 definitions | page 1/15
 VNUM   Name                     Type     Player       Wild NPC
 31514  Tsunami                   unique   legacy       legacy
 19730  Avernus                   major    legacy       legacy
   922  Mirrored ioun             ioun     legacy       legacy

[1] Browse/search    [2] My drafts       [3] Publications
[4] Live instances   [5] Health/audit    [6] Import baseline
Enter a number, a vnum, or /find <name>. /help /exit
```

Mode labels come from actual supported variant IDs. Mirrored ioun's revised mode is displayed as synchronous interception, not a promised telegraph. If a definition has no new variant, show `legacy only` with capability details.

### Artifact overview example

```text
ARTIFACT / Tsunami #31514                 LIVE r42
Category: unique       Family: tsunami    World limit: 1
Player: legacy         Wild NPC: legacy
Source: seakngdm.obj    Powers: 2 migrated, native hum retained
Placement: Poseidon #31528 -> room #31722, equipped slot 17
Chance: 50% effective   Reset: boot only   Existing item: UID 851002
Expiry: 6d 03h          Binding: separate; use Instances for details

[1] Modes and holders   [2] Powers/balance  [3] Placements
[4] Lifetime/binding    [5] Base equipment [6] Script/adapter map
[7] Preview as holder   [8] Live instances [9] History
[E] Create draft       /back /help /exit
```

Do not show fake counts or existence from source data alone. If custody lookup is pending/unavailable, say that. Zero instances and unavailable lookup are distinct.

## 3. Editing flows

### Modes and holders

The home browser also offers `Register artifact from prototype` to authorized drafters: search an existing object vnum, inspect its current flags/bindings, choose explicit classification/family and lifecycle, choose supported adapter/variant and initial placements, then validate the complete candidate. Missing prototypes require the established area-authoring/build workflow; the workbench must not invent a vnum or silently install executable code. Category/family changes requiring migration are labeled restart-required. This supplies a guided central onboarding path for newly authored artifacts.

Select Player, Wild NPC, or Controlled NPC, then show only supported variants with concise mechanical differences and resource requirements. Controlled NPC defaults to `inherit player`. `Advanced: placement/instance overrides` explains precedence. A choice affecting a linked form shows cleanup implications. A separate power suppression control never labels itself `legacy`.

Preview offers `player level/class/race`, `NPC prototype`, and `live UID` contexts. It shows selected rule/provenance, effective numbers, required slot, automatic/active/reactive invocation, legal target requirements, and empty-pool readiness. Preview is a deterministic policy/effect-budget summary, not a promise of PvP DPS. It invokes no live spells and does not simulate outcomes as measured data.

### Powers/balance

List each semantic power, its trigger, implementation ownership, timing, cost, cooldown, power values, and capability. Example Tsunami rows: group vitality tap; wave thrust/raise; native hum. Open a field to show current value, inherited value, unit, allowed range, and when the edit takes effect. Display seconds/minutes and mana points; serialize exact integer milliseconds/milliunits. Input `2.5 MP` converts exactly to 2500 milliunits; reject excess precision. Avernus and sword chance controls expose exact fractions, preserving imported random behavior.

Shared-profile edits must offer `change shared profile (N consumers)` or `copy balance profile for this artifact`. Cloning a balance profile is safe; cloning/changing resource identity on an enrolled UID is not. Changing mana capacity/rate prompts for automatic increment of the resource revision in the draft and explains clamp/no-refill behavior. Base cost and ability multipliers are visible separately. No hidden fallback to an old free power.

Legacy-only hard-coded fields show `Requires adapter parameterization` plus source, rather than a nonfunctional text box. The delivered pilots' descriptors must expose their currently relevant constants listed in the work packages. Newly authored typed Studio spell abilities can be edited through the same power pane with validated spell/target pickers.

### Placements

Wizard steps:

1. Choose existing placement to edit or Add placement.
2. Search zone by name/vnum; choose loaded/resettable zone.
3. Choose room, NPC reset entry in that room, or container reset entry. Display ambiguity if multiple reset entries exist for the same vnum.
4. Choose Give/inventory or Equip and a compatible slot. Check body/wear compatibility. Show active-command/AI requirements.
5. Choose boot-only, zone-reset, or manual; set explicit final chance and limit. Show family-wide uniqueness separately.
6. Choose holder-following, pinned variant, or encounter-only override with post-loot behavior.
7. Review old/new path, dependencies, nominal/imported chance versus final chance, future-spawn-only effect, and possible source reset conflicts.

The wizard must not require an immortal to know reset-file field order. Provide room/mob/container previews without teleporting the user or spawning objects. Changing placement does not recall an existing item; the screen says when it will next take effect.

### Lifetime/binding and base equipment

Lifetime screen separates initial lifetime, maximum remaining lifetime, future feed rates/caps, merge/loot allowances, war penalties, and scheduler interval. Show each unit and the existing-instance scope. An existing UID's remaining time can be changed only from its State operation screen, with expected revision and preview.

Base equipment editor provides typed fields appropriate to the prototype; no freeform `value[0..7]` editing. Show values derived from class-specific native equipment adapters separately from template defaults. Unsafe structural changes are labeled `restart required`. Publishing a mixed candidate containing restart-required fields requires either splitting the draft or scheduling the entire bundle for boot; do not partially apply the hot subset automatically.

## 4. Review, publication, conflicts, and rollback

```text
DRAFT D17 generation 6                    Base r42 | NOT LIVE
Tsunami / holder.wildNpc: legacy -> telegraphic
Tsunami / tap.cooldown: 300s -> 240s
Placement poseidon-tsunami: boot-only -> zone-reset

Validation: PASS         Affected families: 1
Existing expiry/reserve: unchanged
Pending actions: cancel when applied; accepted costs remain spent
Apply: online            New profile identity: none
Reason: [required, 8-512 characters]

[P] Publish    [S] Save draft    [B] Back    [X] Discard draft
```

After Publish: `Request Q19 queued; active remains r42`. Later: `Q19 applied r43; 1 family changed; audit A81`. If failure: status, exact fields/errors, corrective action, and retained draft. If conflict: show base/current/proposed values and Resolve mine / Resolve theirs per overlapping field; non-overlapping merge is proposed for review. No blind Force option.

History lists time, principal, reason, scopes, old/new revision, result. Rollback opens a draft restoring selected configuration from an old revision, with new monotonic publication and resource revisions. It displays that custody, expiry, reserve, and committed costs will not be rewound. Publish uses the same review flow. Do not expose a direct `UPDATE head` recovery action.

## 5. Command equivalents and CLI contract

Every menu operation has a text-command equivalent under `artctl`, using the same service; document accepted abbreviated names without ambiguity. Required command families: `list`, `show`, `explain`, `edit`, `resume`, `drafts`, `validate`, `diff`, `publish`, `status`, `history`, `rollback`, `instances`, `instance preview`, `instance submit`, and `health`. `artctl publish` requires draft ID, generation, expected base revision and reason; interactive mode supplies them from its immutable review snapshot.

Build `bin/tools/artifactctl` as a C++ CLI using the production pure parser/policy/repository libraries. Do not reimplement validation in Python. Human output defaults to a table; `--json` emits a stable machine-readable envelope with result, operation ID, diagnostics, revisions, and retryability. Command paths and file names below are deliverables to implement.

```sh
# Source/config work; no server login and no state mutation.
bin/tools/artifactctl validate-files --catalog lib/artifacts/catalog.json
bin/tools/artifactctl export --revision active --out bin/artifact-export
bin/tools/artifactctl import --catalog bin/artifact-export/catalog.json --draft
bin/tools/artifactctl draft show D17
bin/tools/artifactctl draft diff D17
bin/tools/artifactctl draft validate D17 --generation 6

# Publication is a request, not an unverified immediate live edit.
bin/tools/artifactctl draft publish D17 --generation 6 --expected-revision 42 \
  --reason 'Tsunami encounter configuration' --operation-id <uuid>
bin/tools/artifactctl status <uuid> --wait 30 --json

# Running game is not required for authored config or boot scheduling.
bin/tools/artifactctl draft schedule-boot D17 --generation 6 --expected-revision 42 \
  --reason 'Approved prototype update' --operation-id <uuid>
bin/tools/artifactctl activate-offline --submission <uuid> \
  --world <built-world-path> --expected-revision 42

# Instance change is separate and defaults to preview.
bin/tools/artifactctl instance mode --uid 851002 --mode telegraphic --preview
bin/tools/artifactctl instance submit --preview <preview-id> \
  --expected-instance-revision 9 --reason 'Canary instance' --operation-id <uuid>
```

`activate-offline` refuses a live service lease/file lock, checks correct backend/database identity, and uses actual intended world/binary validation. It may activate validated configuration only; physical item-state operations wait for the server and are never simulated by editing owner files. `--wait 30` times out with request ID and pending state, not a false failure or automatic resubmission.

CLI exit codes: 0 success/read/validated/applied; 2 usage/invalid; 3 conflict; 4 forbidden; 5 unavailable/retryable; 6 accepted but pending/timeout; 7 restart required; 8 unsupported. JSON error codes remain more specific. Supply shell completion and `help <command>` with examples. Noninteractive input supports files/stdin and explicit IDs; do not require a terminal prompt.

Database credentials come from the established secret/config mechanism or a protected client option file, never command-line passwords or generated logs. Local file mode authenticates through OS permissions and records OS principal. The server never executes the CLI or a shell command to perform an immortal action.

## 6. Safe database editing without game login

Provide `docs/operations/ARTIFACT_CONTROL_SQL.md` and tested SQL examples. Required user story: an operator using a restricted account can create a draft from current revision, edit one supported field with JSON functions or replace the document, submit it, and inspect success/rejection/application status.

The procedure signatures are part of the implementation contract:

```sql
-- Proposed APIs. Run only after the implementation/migration exists.
SET @draft = UUID();
CALL artifact_draft_create(@draft, 42, 'Tsunami encounter tuning');

-- The draft document is a complete resolved bundle. Find the array index
-- from a SELECT; do not assume order is stable between drafts.
SELECT draft_id, generation, document
FROM artifact_config_draft WHERE draft_id = @draft;

-- Illustrative index only; the operator first verifies definitions[0].id.
UPDATE artifact_config_draft
SET document = JSON_SET(document,
    '$.definitions[0].holderPolicy.wildNpc', 'telegraphic')
WHERE draft_id = @draft AND generation = 1;
-- Trigger increments generation and authenticates/stamps the edit.
-- ROW_COUNT() must be 1; otherwise refresh before proceeding.

SET @validation = UUID();
CALL artifact_draft_submit(@draft, 2, 'validate', 42, @validation,
    'Check NPC mode change');
SELECT * FROM artifact_config_status_v WHERE submission_id = @validation;

SET @publish = UUID();
CALL artifact_draft_submit(@draft, 2, 'publish', 42, @publish,
    'Apply validated NPC mode');
SELECT * FROM artifact_config_status_v WHERE submission_id = @publish;
```

`artifact_draft_replace(draft_id, expected_generation, document, reason)` is the convenient parameterized alternative to UPDATE. Document validation and current-head checks still repeat at publication; a prior `VALIDATED` result is not a permission to bypass later world changes. `artifact_draft_create` rejects a base revision no longer active unless explicitly creating a historical rollback draft through the service. Procedure changes are idempotent under caller-generated UUIDs and compare exact request content.

The SQL account is allowed to write draft content, not `artifact_config_head`, `artifact_config_revision`, `artifact_config_result`, existing `artifacts`/`artifact_bind`, UID custody, or `artifact_mana`. The supplied SQL examples must include tests proving attempts against those tables are denied. Changing a JSON field to an illegal enum or missing vnum must produce an understandable rejected request while existing game behavior continues.

For lifecycle/instance operations use `artifact_request_submit(operation_id, operation_type, payload_json, expected_catalog_revision, reason)`. Allowed payloads are typed and schema-checked; `expectedInstanceRevision` and preview hash are required for destructive/state edits. SQL cannot submit arbitrary SQL, creature commands, callbacks, or unsafe pointer identifiers. Instance preview is an asynchronous read-only request returning the same preview as the UI; it runs against current game-thread state and must precede an edit.

When the server is stopped, draft edit/export/validation against files works immediately; queued publish/state requests remain visibly queued. Configuration may be scheduled/activated using the offline validator/lease protocol. Actual live-item state changes apply after the server starts and validates current custody. The UI and CLI must never label queued or committed-but-not-applied changes as effective gameplay.

## 7. Failure and recovery operator guide

| Situation | Visible result | Required safe action |
| --- | --- | --- |
| Bad field or unsupported variant | REJECTED with JSON path and supported choices | Repair the same draft, validate, resubmit new immutable request. |
| Other operator published | CONFLICT with base/current revisions | Rebase and review conflicting fields. |
| Database unavailable while running | Current revision retained; publication unavailable | Repair storage; retry same operation ID if outcome unknown. |
| Server offline | QUEUED or scheduled for boot | Start validated server or run exclusive offline config activation. |
| Crash after durable head commit | COMMITTED until boot applies/reports | Restart intended binary/world; recovery applies committed revision before listener. |
| Invalid committed head at boot | Listener refuses startup with exact incompatibility | Restore compatible binary/world or use offline validated rollback; do not delete mana/custody tables. |
| Instance moved/expired since preview | INSTANCE_MOVED or state conflict | Inspect current UID and request a fresh preview. |
| Mana empty/not ready | Power suppressed; estimated readiness in operator preview | Wait for legitimate regeneration/readiness or choose an approved configuration; no refill by toggling. |
| Restart-required edit mixed with hot edits | READY_RESTART, no partial application | Schedule bundle or split draft explicitly. |
| Publication cleanup stalled | Affected powers/spawns suppressed, progress/error visible | Diagnose reported cleanup failure; do not enable legacy automatically. |

Backups include active revisions/head, immutable submissions/results needed for retry, audit, instance policy/cooldowns, and existing resource/custody/lifecycle state as a coherent generation. Restore checks hashes, referential integrity, and committed-head/application recovery. A file export is useful for authoring but is not a complete game-state backup.

## 8. Required usability journeys

U01. A new immortal finds Tsunami by name, understands its unique classification, finds Poseidon's exact load point, and distinguishes give/equip, expiry/binding/cooldown without reading source files.

U02. They create a draft, change wild-NPC mode while retaining player legacy mode, tune a supported cooldown, preview both holders, publish once, and observe the applied revision and actual behavior.

U03. They change a load point through pickers, see the effective chance and future-only effect, and observe a disposable reset load at that destination without duplication.

U04. They interrupt with chat/game commands, use pager/string editor, disconnect/reconnect, and resume an acknowledged draft without losing it or executing accidental game commands.

U05. Two immortals edit the same base; the second gets a legible conflict and can rebase without losing either nonoverlapping edit.

U06. An operator with no game login performs file export/edit/import/validate/publish/status through CLI, then performs draft UPDATE/submit/status from SQL; both produce identical policy results.

U07. An invalid SQL change and a denied live-table UPDATE leave gameplay intact and provide actionable diagnostics.

U08. An operator pins an individual UID, loots/transfers/restarts it, and confirms variant identity, cooldown, expiry, and reserve do not reset.

U09. They roll back a published balance change and observe old parameters under a new revision, with no restoration of spent resources.

U10. Color-off, 80-column and narrow terminal flows are readable; all actions work by keyboard with contextual help. Capture real Telnet transcripts for these journeys, not only renderer snapshots.
