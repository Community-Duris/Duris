# Help System

How in-game help works end to end: source material, import pipeline, and
runtime serving.

## Runtime path (what players see)

The `help` command (`do_help`, `src/cmd/actinf.c`) does two things:

1. **`wiki_help()`** (`src/cmd/wikihelp.c`) searches an in-memory catalog. In
   database-backed mode, `help_cache.c` loads `pages` on a background worker
   after world boot, borrows a pool connection, and publishes a complete
   immutable catalog on the game loop. It refreshes automatically every 60
   seconds when idle. Failed loads retain the previous generation. Before the
   first successful publication, players receive a temporary-unavailable
   message. Help lookup/rendering performs no database query.

   Titles match case-insensitively with MySQL-style `%`/`_` wildcards in the
   database-backed build. One match renders the full entry; multiple matches
   render the exact title when present, including beyond the first result
   batch, followed by at most `WIKIHELP_RESULTS_LIMIT` (100) related titles.
   Lists explain how to read a topic and refine a capped search. Leading and
   trailing input whitespace is ignored. A miss suggests a shorter keyword,
   `help index`, `help commands`, or `commands`, and is logged to `lib/etc/help`
   (`logit(LOG_HELP, ...)`). The log records a timestamp and query, without
   character identity; it records unresolved searches with no stored title match
   or registered live topic. It does not measure ambiguity or whether a page
   answered the player's question.
   Entries are stored wiki-formatted; `dewikify()` converts `[[...]]` markup
   into ANSI-colored output. Help imposes no character wait or browsing
   cooldown; the obsolete `help.cooldown.secs` property is ignored.

   Two page features are applied at render time (`src/cmd/wikihelp.c`,
   `wiki_help_single()`):

   - **Redirects**: a row with `category_id` 1 whose text starts with
     `Redirect: <target>` is followed to the complete multiword title, with
     an eight-lookup bound. An exact live provider can supply a missing target;
     other missing targets and excessive chains return errors.
   - **Dynamic sections**: the shared renderer selects live code/property
     providers. Explicit SQL categories select 25 race (classes,
     racial stats, innates), 9 class (allowed races, innates, specs),
     16 spec (races, innates, skills, spells), 10 class-skillset (innates,
     skills, spells). Category-zero imports and flat files also bind exact
     registered race, class, specialization, and class-skillset names.
     `<class> Skills` and `SKILL_<class>` are both recognized. `Multiclass`,
     `Races`, and `Index` bind case-insensitively in any category. Active specializations
     take precedence over the retired Assassin/Thief class names in category 0;
     category 9 can explicitly select the historical class.

     Known providers replace their captured generated sections, older plain
     tables (`Class list:`, `Statistics:`, `Skills`, etc.), and embedded edit
     header at render time, then append current facts once. Descriptions,
     strengths/weaknesses, equipment notes, and See also sections remain.
     Race/class/spec titles and result links use their canonical game-table
     colors; live headings and values are colored. Author only the static part.

     An exact registered topic without a stored page renders current facts and
     clearly states that its narrative has not yet been authored. This takes
     precedence over unrelated substring matches (for example, `Mentalist`
     must not open `Elementalist`). Partial searches/indexes now enumerate
     the combined stored/live topic catalog, so `help ment` offers Mentalist
     even when its narrative is missing.

   A shared, ASCII-normalized topic index merges stored titles with registered
   live subjects and both class-skillset naming conventions. It is built once
   per successfully published MySQL cache generation, or once for the flat
   catalog. Failed refreshes retain the existing index; no command-time SQL is
   added. Stored entries take precedence over generated placeholders.

   `HELP INDEX [all|races|classes|specs|skillsets] [page]` lists 50 topics per
   page and supplies the next-page command. The old captured `Index` page now
   renders this current index. `[live]` identifies a current-data provider;
   `[live; narrative missing]` identifies facts without an authored page.
   Groups classify topic identity, including legacy/restricted subjects; their
   presence does not establish current creation eligibility.

   Unresolved queries of 3-64 bytes can receive up to five typo suggestions,
   including adjacent transpositions. Suggestions use at most one edit for
   queries up to four bytes and two edits for longer queries. Wildcard/escape
   queries are excluded from suggestions. Suggestions show the exact HELP
   command and never select a topic automatically. Existing MySQL wildcard
   and flat literal-substring behavior remains.

   `TOGGLE TERMINAL GEN` now removes color attributes at the final output step
   before terminal rendering, including help, paging, and prompts. Text and
   normal CRLF handling remain; switching back to ANSI retains configured colors.

2. **`attrib_help()`** - appends per-command attributes (stat usage)
    loaded at boot from `docs/lib/information/command_attributes.txt`
    (`src/cmd/wikihelp.c`, boot loader). If the file is missing, only a debug log
    line notes it. This is a separate stat-usage index, including legacy ability
    names that are not commands (`apply poison`, `parry`, ...). An attribute
    entry does not establish that a narrative help page exists, or that its
    explanation is current. Commands whose handlers consult no stats carry
    just the header line. The loader holds
    up to `CMD_ATTRIB_MAX` entries (1024, `src/cmd/wikihelp.h`) and bounds-checks
    the count, logging and skipping anything beyond the cap.

Without MySQL (`make -C src PERSISTENCE_BACKEND=flatfile`, which defines
`__NO_MYSQL__`), the same command loads and caches the
tracked source files that feed the database importer. It applies the importer's
precedence - individual `lib/information` pages, then `help_index`, then
`duris_help_parsed.hlp` - and provides case-insensitive exact and substring
searches without a database connection. Missing or structurally invalid source
catalogs fail closed with the normal help-system error instead of silently
returning the former disabled stub. Restart after changing these sources;
`page help` reports that the flat-file build uses its startup catalog.

The flat narrative catalog initializes on the first help lookup and remains
fixed until restart. It uses literal substrings rather than SQL wildcards and
has no category/edit metadata. It calls the same dynamic renderer as MySQL;
properties, current lists, and availability are evaluated on each read rather
than frozen in the cached text. Its redirects recognize a leading `Redirect:` without
category metadata; invalid chains can fall back to displaying redirect text.
`page help` reports this load-once behavior. The database-backed `page help`
queues refresh and `page help status` reports generation, readiness, pending
work, and the last error; see [Help catalog operation](../operations/help-cache.md).

Other informational content has separate serving paths. Flat-file builds read
allow-listed `lib/information/` files instead of `mud_info`; `credits`, `faq`,
and `wizlist` use a separate background cache on both backends. See
[Informational page cache](../operations/information-cache.md) for its refresh
commands and [Help catalog operation](../operations/help-cache.md) for help
cache limits and failure handling.

## Content pipeline

```
lib/information/* ------------------+
docs/lib/information/hints.txt ------+--> scripts/import_help_to_prod.sh
help/duris_help_parsed.hlp ----------+                 |
                                                      +--> pages
                                                      +--> mud_info
```

`scripts/import_help_to_prod.sh`:

> [!WARNING]
> Despite its name, this script can write to any database selected by `.env`
> or to a remote host supplied with `--remote`. Run `--dry-run` first, verify
> `DB_HOST`, `DB_PORT`, and `DB_NAME`, and take a database backup before a live
> import. `--clean` deletes all rows from `pages` before re-importing content.
> The script prompts for confirmation for live and clean operations.

- Maps files into `mud_info` (motd, news, wizmotd, credits, FAQ, wizlist) and
  `pages` (help, help.1/2, ships, kingdoms, chaos craft pouch, FAQ, rules,
  credits, wizlist, hints). The `rules` command uses `do_help("rules")` and
  therefore reads `pages`; a `mud_info.rules` edit alone does not update it.
- `hints.txt` now lives at `docs/lib/information/hints.txt`; the script reads
  it from there (the login screen streams it via `src/account/nanny.c`).
- Parses `lib/information/help_index` and `help/duris_help_parsed.hlp` inline.
  `help/duris_help.hlp` is a raw historical capture, not a runtime/import input.
  `areas/**/dehelp.hlp` is area-editor help, also outside the player catalog.
- **Import order matters**: Section 2 (`help_index`) runs before Section 3
  (`duris_help_parsed.hlp`). Both write with DELETE-by-title + INSERT, so a
  title present in both files ends up owned by `duris_help_parsed.hlp`.
  Titles compare case-insensitively (MySQL default collation). Check both
  sources before adding an entry to `help_index`.
- All three import sections write `category_id=0`. Importing a race/class/spec
  page now activates a provider when its title matches an exact registered
  subject. Importing `Redirect:` text still does not activate MySQL redirect
  behavior. The source/export formats do not retain categories. The flat loader
  additionally registers the `chaos pouch` title.
- Title parsing in `help_index`: unquoted titles are truncated at `(` -
  `PURGE (Spell)` stores page title `PURGE`; quoted titles keep everything
  inside the quotes - `"ECHO (IMMORTAL)"` stores the full string. Use
  quoting whenever you need parentheses in a page title.
- In MySQL builds, greater gods run `page help` after an import to queue a
  help-catalog refresh, then `page help status` to confirm that the generation
  advances. Queuing is not completion. An idle worker also refreshes
  automatically every 60 seconds; a blocked load can delay freshness.
- motd/news/wizmotd are cached into memory at boot (`src/world/db.c`) and re-read
  by the bare `page` command (greater god, `src/cmd/actcomm.c`). After importing
  new copies, run `page` or restart. `page help` and `page help status` return
  before this legacy reload and do not refresh those boot-cached strings.
- Content is hex-encoded into SQL so arbitrary text survives. The script stages
  all three import sections and optional cleanup in one transaction, requires
  existing InnoDB `pages` and `mud_info` tables, and commits only after generation
  and SQL execution succeed. A SQL error rolls back the import on disconnect.
  The help loader's single query sees the old or new committed catalog, not an
  intermediate delete/insert state. `--dry-run` generates a preview without
  applying the transaction.
- `lib/information/help_index` carries many immortal-command entries
  (see the audit for current registration gaps), written against the
  command implementations in `src/`. Bare-command titles
  (`POOFIN (Immortal Command)` -> page `POOFIN`) are exact-match
  discoverable; names colliding with spells/skills are quoted so the
  parentheses survive (`"ECHO (IMMORTAL)"`). Because import deletes by
  title, new entries must never re-use an existing bare title.

## Editing help

- Authoring rules and formatting conventions:
  [`docs/content/HELP_STYLE_GUIDE.md`](HELP_STYLE_GUIDE.md). The style
  guide is spell-oriented (stat header block); immortal command entries in
  `help_index` instead use a `Syntax:` line, a rank/level line, a short
  description, and `See also:` - follow the existing entries there.
- In-game topic text lives in `pages`. There is no in-game page editor in this
  server. The importer writes source files to SQL; DurisWeb also has permission
  controlled CRUD and approved suggestion publication. Website edits are not
  exported back to tracked files. A later import can overwrite those edits;
  `--clean` also removes database-only titles. Capture and reconcile changes
  before reimporting. See the warning above before importing to a shared or
  remote database.
- Flat-file help changes require a restart instead of a database import.
- Command attribute changes go in
  `docs/lib/information/command_attributes.txt` and require a server restart
  (loaded once at boot).

## DurisWeb integration

DurisWeb's `backend/src/routes/help.ts` serves race/class text, `guide.ts`
provides a public title index/search and page-by-ID reads, and `content.ts` plus
`contentService.ts` provide administration. `helpSuggestionService.ts` publishes
approved suggestions. These paths use the website's primary database `pool`.
The website can configure a separate `mudPool`; the help paths still use `pool`.
The MUD observes website edits only when its own database connection reads the
same `pages` catalog, usually on the next successful refresh. Deployment database
ownership must be checked explicitly in separate-database mode.

There is no help fetch/update hook in the authenticated MUD bridge, no help
hook toggle in the website registry, and no SQL-to-flat-file help synchronization.
Website guide/chargen views render stored text; they do not invoke the game's
dynamic race/class/spec rendering or its redirect resolver. Other website wiki
object/mob/zone publication systems are separate from player help.

The code inspection, pinned website source links, gap inventory, and proposed
publication/dynamic-help work are recorded in the
[help-system audit and implementation plan](../design/HELP_SYSTEM_AUDIT.md).

## Auditing and contribution checks

Run the read-only auditor from any directory; it defaults to its repository:

```bash
python3 scripts/audit_help.py --output bin/help-audit.json
python3 scripts/audit_help.py --format csv --output bin/help-coverage.csv
python3 scripts/audit_help.py --require-term "Human" --require-term "help" --output bin/help-audit.json
```

The JSON contains title/body fingerprints, source/line provenance, overwrite
events, registered-keyword coverage, body mentions, spelling suggestions, redirect
diagnostics, and unresolved cross-reference candidates. The CSV lists every
discovered command, spell, skill, song, innate, race, class, class skillset,
specialization, attribute keyword, and explicitly quoted help hint in game
messages. `exact`, `partial`, `ambiguous`, and
`missing` describe title retrieval, not factual accuracy or feature availability.
The auditor excludes commented registrations, distinguishes staff/social/trigger
commands, and reports unregistered names separately. It is a source-layout-aware
inventory, not a C++ compiler or runtime permission evaluator.

This report audits stored/authored entries. A missing title can still have a
generated-only runtime view when it is an exact registered dynamic topic; that
does not fill the missing explanation or satisfy the authored-topic gate.

`--require-term` is repeatable and exits 1 for absent/nonexact or unusable topics.
Use it for the keyword introduced by a change; existing gaps do not make every
contribution fail. Normal reports exit 0 even with gaps; malformed input/source
layout errors exit 2. The Python reconstruction is compared byte-for-byte with
the actual C++ loader by `test_flatfile_help_catalog.py`.

For a database inventory, supply an explicit UTF-8 JSON array or JSONL export:

```bash
python3 scripts/audit_help.py --pages-json /path/to/pages.jsonl --output bin/database-help-audit.json
```

Export only `title`, `text`, `category_id`, and optional public edit metadata.
For example, run this SELECT with an already configured read-only mysql client
using `--batch --raw --skip-column-names`, and redirect stdout to a private file:

```sql
SELECT JSON_OBJECT('title', title, 'text', text, 'category_id', category_id)
FROM pages ORDER BY title, id;
```

The auditor never reads `.env`, acquires credentials, executes SQL, imports,
or publishes. A pages export reports duplicate titles, malformed records, and
inactive category-dependent redirects; duplicate SQL rows have no guaranteed
runtime winner. Matching uses ASCII title equality/literal substrings and does
not emulate database collation, `%`/`_` wildcard matching, the loader's complete
size/health contract, or the live cache's generation. Keep private/live exports
out of Git. The committed [coverage snapshot](../reports/help-coverage.csv) was
generated solely from tracked source material.

## Related

- Login hints: `docs/lib/information/hints.txt` read by `nanny.c`.
- Wiki export helpers and formatting utilities live alongside the loader in
  `wikihelp.c`.

## Live providers and extension point

`dynamic_topic` and `render_help_content` in `src/cmd/wikihelp.c` are the shared
binding/rendering path for both persistence builds. New providers should bind
explicit identities, own a defined set of sections, and reuse gameplay helpers.
Keep mutable game reads on the game loop; do not add command-time SQL.

| Topic | Live information and existing helpers |
| --- | --- |
| Race | Configured racial statistics and combat/spell pulses (`wiki_racial_stats`); current creation class/spec choices (`wiki_classes`); innate unlocks (`wiki_innates`). |
| Class | Current creation races (`wiki_races`), innate unlocks (`wiki_innates`), and colored specializations (`wiki_specs`). |
| Specialization | Current creation races, innate unlocks, skills, spells, and bard songs/instruments (`wiki_races`, `wiki_innates`, `wiki_skills`, `wiki_spells`). |
| Class skillset | Innates, skill levels, spell circles, and bard song/instrument unlocks (`wiki_innates`, `wiki_skills`, `wiki_spells`). |
| Races | Canonical creation roster, configured enabled flags, and separately identified progression/restricted races (`wiki_pcraces`). |
| Multiclass | Current secondary-class combinations and names (`wiki_multiclass`). |
| Index | Current stored and generated topics, grouped and paginated (`help_index`, `render_help_index`). |

Class/race availability sections describe character-creation choices through
`creation_class_enabled`, `creation_class_align`, `creation_race_enabled`, the
normal roster, and `CREATION_ALL_RACES` / `CREATION_ALL_CLASSES`. They do not
revoke existing characters or certify account-specific admission. Racial values
come from `stats.*.<race-key>` and pulse properties, not the querying character's
rolled/buffed stats. Creation configuration is still loaded at boot; property
reload and existing override mechanisms keep their own normal lifecycle.

Related information commands already read current game state: `do_skills`,
`do_spells`, and `do_practice` in `src/guild/guild.c`, and `do_innate` /
`do_list_innates` in `src/classes/innates.c`. Their character-specific views
provide useful authorities for a future contextual help provider. Today,
`wiki_help` receives only a query string; the shared help providers present
race/class/spec facts without a character context. `attrib_help` remains a
separate boot-loaded text appendix, not a live command-requirement evaluator.

## Reviewed source additions

`lib/information/help_index` now supplies exact pages for `Named Equipment`,
`Namedreport`, `Racewar`, `Introduce`, `Refine`, `Soulbind`, and `Prestige`.
These pages include syntax, availability/requirements, behavior, feedback where
applicable, examples, and exact related titles. They describe their feature
gates, including build-dependent introductions and accounting-disabled refining
and soulbound restoration. `Named Equipment` sends players to the existing
current-data `NAMEDREPORT` command rather than maintaining a captured zone list.

Flat builds load these sources at startup/first use. A MySQL deployment needs
its reviewed content import followed by help-cache publication; a server build
alone does not insert the new authored pages into `pages`. The runtime index
and registry-based providers do not require that import. Follow the existing
import procedure and target checks; this PR performs no operational import.
