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

   One match renders the full entry; multiple matches render the exact match
   plus "see also" links. User input is escaped (`escape_str()` wraps
   `mysql_real_escape_string`). Misses are logged to the help log
   (`logit(LOG_HELP, ...)`), which is useful for spotting missing topics.
   Entries are stored wiki-formatted; `dewikify()` converts `[[...]]` markup
   into ANSI-colored output.

2. **`attrib_help()`** — appends per-command attributes (syntax, level,
   position requirements) loaded at boot from
   `docs/lib/information/command_attributes.txt`
   (`src/wikihelp.c`, boot loader). If the file is missing, only a debug log
   line notes it.

Without MySQL (`-D__NO_MYSQL__` builds) help is disabled and returns a stub
message — the help system is database-backed by design.

## Content pipeline

```
lib/information/*        help/, docs/help/          database
├─ motd, news, faq  ─┐   ├─ duris_help.hlp          ┌─────────────┐
├─ help, rules, ... │──▶├─ duris_help_parsed.hlp ─▶│ pages       │
└─ hints.txt        ─┘   └─ parse_help_index.py     │ mud_info    │
        scripts/import_help_to_prod.sh                 └─────────────┘
```

`scripts/import_help_to_prod.sh`:

- Maps files from `lib/information/` into `mud_info` (motd, news, wizmotd)
  and `pages` (help, help.1/2, guild/ship/kingdom helps, faq, rules, info,
  credits, wizlist, hints).
- `hints.txt` now lives at `docs/lib/information/hints.txt`; the script reads
  it from there (the login screen streams it via `src/nanny.c`).
- Imports the parsed help entries from `help/duris_help_parsed.hlp` (~500+
  topics); the help index is parsed inline by an embedded Python heredoc in
  the script.
- Content is hex-encoded into `DELETE`+`INSERT` SQL so arbitrary text survives;
  supports `--dry-run`.

## Editing help

- Authoring rules and formatting conventions:
  [`docs/help/HELP_STYLE_GUIDE.md`](help/HELP_STYLE_GUIDE.md).
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
