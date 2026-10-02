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
   `help commands`, or `commands`, and is logged to `lib/etc/help`
   (`logit(LOG_HELP, ...)`). The log records a timestamp and query, without
   character identity; it records zero title matches, not ambiguity or whether
   a page answered the player's question.
   Entries are stored wiki-formatted; `dewikify()` converts `[[...]]` markup
   into ANSI-colored output. Help imposes no character wait or browsing
   cooldown; the obsolete `help.cooldown.secs` property is ignored.

   Two page features are applied at render time (`src/cmd/wikihelp.c`,
   `wiki_help_single()`):

   - **Redirects**: a row with `category_id` 1 whose text starts with
     `Redirect: <target>` is followed to the complete multiword title, with
     an eight-lookup bound. Missing targets and excessive chains return errors.
   - **Dynamic sections**: rows with certain `category_id`s get content
     appended from code/properties, not from stored text: 25 race (classes,
     racial stats, innates), 9 class (allowed races, innates, specs),
     16 spec (races, innates, skills, spells), 10 class-skillset (innates,
     skills, spells). Titles `Multiclass` and `Races` also get hardcoded
     sections. Editing those pages means authoring only the static part.

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
returning the former disabled stub.

The flat help catalog initializes on the first help lookup and remains fixed
until restart. It uses literal substrings rather than SQL wildcards, has no
category/edit metadata, and does not append the category-driven dynamic
sections above. Dynamic-looking sections already present in exported text
are stored snapshots. Its redirects recognize a leading `Redirect:` without
category metadata; invalid chains can fall back to displaying redirect text.
`page help` reports this load-once behavior. The database-backed `page help`
queues refresh and `page help status` reports generation, readiness, pending
work, and the last error; see [Help catalog operation](../operations/help-cache.md).

The same client-free content path serves the existing `mud_info` callers for
motd, news, wizmotd, credits, FAQ, rules, and wizlist directly from their
allow-listed `lib/information/` files. This keeps boot/login and information
commands functional without changing the database-backed lookup path.

## Content pipeline

```
lib/information/*          help/                      database
|- motd, news, faq    -+   |- duris_help.hlp          +-------------+
|- help, rules, ...   |-->|- duris_help_parsed.hlp ->| pages       |
+- hints.txt, help_index  +- (parsed inline by the    | mud_info    |
                             import script)           +-------------+
        scripts/import_help_to_prod.sh
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
  page does not activate the dynamic renderer; importing `Redirect:` text does
  not activate MySQL redirect behavior. The source/export formats do not retain
  categories. The flat loader additionally registers the `chaos pouch` title.
- Title parsing in `help_index`: unquoted titles are truncated at `(` -
  `PURGE (Spell)` stores page title `PURGE`; quoted titles keep everything
  inside the quotes - `"ECHO (IMMORTAL)"` stores the full string. Use
  quoting whenever you need parentheses in a page title.
- motd/news/wizmotd are cached into memory at boot (`src/world/db.c`) and re-read
  only by the greater-god `page` path (`src/cmd/actcomm.c`). After
  importing new copies, run `page` or restart; otherwise players keep
  seeing the old text.
- Content is hex-encoded into `DELETE`+`INSERT` SQL so arbitrary text survives;
  supports `--dry-run`. The whole import is staged into one InnoDB transaction,
  including optional cleanup; errors roll it back. No migrations are needed for
  ordinary content imports.
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
