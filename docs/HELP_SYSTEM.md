# Help System

How in-game help works end to end: source material, import pipeline, and
runtime serving.

## Runtime path (what players see)

The `help` command (`do_help`, `src/actinf.c:6527`) does two things:

1. **`wiki_help()`** (`src/wikihelp.c`) — queries the `pages` table on the
   main MySQL connection:

   ```sql
   SELECT title FROM pages WHERE title LIKE '%<term>%'
     ORDER BY title ASC LIMIT <N>
   ```

   (`WIKIHELP_RESULTS_LIMIT` is 100, `src/wikihelp.h`.) One match renders
   the full entry; multiple matches render the exact match plus "see also"
   links. User input is escaped (`escape_str()` wraps
   `mysql_real_escape_string`). Misses are logged to `lib/etc/help`
   (`logit(LOG_HELP, ...)`), which is useful for spotting missing topics.
   Entries are stored wiki-formatted; `dewikify()` converts `[[...]]` markup
   into ANSI-colored output. Requests are rate-limited by the
   `help.cooldown.secs` property (default 2s).

   Two page features are applied at render time (`src/wikihelp.c`,
   `wiki_help_single()`):

   - **Redirects**: a row with `category_id` 1 whose text starts with
     `Redirect: <target>` is followed to `<target>`.
   - **Dynamic sections**: rows with certain `category_id`s get content
     appended from code/properties, not from stored text: 25 race (classes,
     racial stats, innates), 9 class (allowed races, innates, specs),
     16 spec (races, innates, skills, spells), 10 class-skillset (innates,
     skills, spells). Titles `Multiclass` and `Races` also get hardcoded
     sections. Editing those pages means authoring only the static part.

2. **`attrib_help()`** — appends per-command attributes (syntax, level,
    position requirements) loaded at boot from
    `docs/lib/information/command_attributes.txt`
    (`src/wikihelp.c`, boot loader). If the file is missing, only a debug log
    line notes it. Coverage is partial: ~382 commands are listed (loader
    caps at 400 of ~850 registered commands), so many commands show no
    attribute block.

Without MySQL (`-D__NO_MYSQL__` builds) help is disabled and returns a stub
message — the help system is database-backed by design.

## Content pipeline

```
lib/information/*          help/                      database
├─ motd, news, faq    ─┐   ├─ duris_help.hlp          ┌─────────────┐
├─ help, rules, ...   │──▶├─ duris_help_parsed.hlp ─▶│ pages       │
└─ hints.txt, help_index  └─ (parsed inline by the    │ mud_info    │
                             import script)           └─────────────┘
        scripts/import_help_to_prod.sh
```

`scripts/import_help_to_prod.sh`:

- Maps files from `lib/information/` into `mud_info` (motd, news, wizmotd)
  and `pages` (help, help.1/2, guild/ship/kingdom helps, faq, rules, info,
  credits, wizlist, hints).
- `hints.txt` now lives at `docs/lib/information/hints.txt`; the script reads
  it from there (the login screen streams it via `src/nanny.c`).
- Imports the parsed help entries from `help/duris_help_parsed.hlp` (~500+
  topics); the help index is parsed inline by an embedded Python heredoc in
  the script (~1500 entries as of the immortal-command and full
  spell/skill coverage).
- **Import order matters**: Section 2 (`help_index`) runs before Section 3
  (`duris_help_parsed.hlp`). Both write with DELETE-by-title + INSERT, so a
  title present in both files ends up owned by `duris_help_parsed.hlp`.
  Titles compare case-insensitively (MySQL default collation). Check both
  sources before adding an entry to `help_index`.
- Title parsing in `help_index`: unquoted titles are truncated at `(` —
  `PURGE (Spell)` stores page title `PURGE`; quoted titles keep everything
  inside the quotes — `"ECHO (IMMORTAL)"` stores the full string. Use
  quoting whenever you need parentheses in a page title.
- motd/news/wizmotd are cached into memory at boot (`src/db.c`) and re-read
  only by the immortal `page` command (level 60+, `src/actcomm.c`). After
  importing new copies, run `page` or restart; otherwise players keep
  seeing the old text.
- Content is hex-encoded into `DELETE`+`INSERT` SQL so arbitrary text survives;
  supports `--dry-run`.
- `lib/information/help_index` carries one entry per immortal command
  (everything listed by `wizhelp`, levels 57-62), written against the
  command implementations in `src/`. Bare-command titles
  (`POOFIN (Immortal Command)` -> page `POOFIN`) are exact-match
  discoverable; names colliding with spells/skills are quoted so the
  parentheses survive (`"ECHO (IMMORTAL)"`). Because import deletes by
  title, new entries must never re-use an existing bare title.

## Editing help

- Authoring rules and formatting conventions:
  [`docs/help/HELP_STYLE_GUIDE.md`](help/HELP_STYLE_GUIDE.md). The style
  guide is spell-oriented (stat header block); immortal command entries in
  `help_index` instead use a `Syntax:` line, a rank/level line, a short
  description, and `See also:` — follow the existing entries there.
- In-game topic text lives in `pages`; the import script is the only write
  path (no in-game editor exists — nothing in `src/` inserts into or updates
  `pages`). Update the source files and re-run the import script; editing the
  database directly would silently diverge from the flat-file sources.
- Command attribute changes go in
  `docs/lib/information/command_attributes.txt` and require a server restart
  (loaded once at boot).

## Related

- Login hints: `docs/lib/information/hints.txt` read by `nanny.c`.
- Wiki export helpers and formatting utilities live alongside the loader in
  `wikihelp.c`.
