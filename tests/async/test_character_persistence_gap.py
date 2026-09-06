#!/usr/bin/env python3
"""Source contracts for character persistence, load, and death recovery.

Pins these previously repaired boundaries:
  1. new characters take the synchronous first save that INSERTs into player_data
  2. a failed terminal death save schedules recovery that completes extraction
  3. item extra descriptions and affects are deleted before being re-inserted
  4. the typed load path reports why it refused a character
  5. display_account_menu tolerates a NULL argument
  6. enter_game converts fallback room vnums before indexing the mini world
"""

from _paths import SRC
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ACCOUNT = (SRC / "account.c").read_text()
DEFINES = (SRC / "defines.h").read_text()
FIGHT = (SRC / "fight.c").read_text()
FILES = (SRC / "files.c").read_text()
LOAD_MATERIALIZE = (SRC / "player_load_materialize.c").read_text()
LOAD_REPOSITORY = (SRC / "player_load_repository.c").read_text()
LOAD_REPOSITORY_H = (SRC / "player_load_repository.h").read_text()
NANNY = (SRC / "nanny.c").read_text()
SAVE_PIPELINE = (SRC / "player_save_pipeline.c").read_text()
SQL_PLAYER = (SRC / "sql_player.c").read_text()
MIGRATION = (
    ROOT / "migrations/immutable/0002_player_item_metadata_uniqueness.sql"
).read_text()


def section(text: str, start: str, end: str) -> str:
    first = text.index(start)
    return text[first : text.index(end, first)]


def require(condition: bool, message: str) -> None:
    assert condition, message


# --- 1. initial player_data insert ------------------------------------------------
require(
    "#define CHAR_RFLAG_NO_DB_BASELINE" in DEFINES,
    "runtime flag marking a character with no player_data row is missing",
)

init_char = section(NANNY, "void init_char(P_char ch)", "\n}\n")
require(
    "getNewPCidNumb()" in init_char
    and "SET_BIT(ch->runtime_flags, CHAR_RFLAG_NO_DB_BASELINE)" in init_char,
    "init_char must mark the file-allocated pid as having no database baseline",
)

write_character = section(FILES, "int writeCharacter(P_char ch, int type, int room)", "\n}\n")
async_branch = section(write_character, "player_save_pipeline_request", "return queued")
gate = write_character[: write_character.index("player_save_pipeline_request")]
require(
    "!IS_SET(ch->runtime_flags, CHAR_RFLAG_NO_DB_BASELINE)" in gate,
    "writeCharacter must not route a character without a baseline row to the async pipeline",
)
require("player_save_pipeline_is_nonterminal_type(type)" in gate, "async gate changed shape")
require(async_branch, "async pipeline branch disappeared")

status_save = section(
    SQL_PLAYER,
    "// status save (main player data)",
    "\n// skills save",
)
require(
    "REMOVE_BIT(ch->runtime_flags, CHAR_RFLAG_NO_DB_BASELINE)" not in status_save,
    "the status component must not clear the flag before the complete save commits",
)
require(
    "component=baseline outcome=inserted" in status_save,
    "the baseline INSERT must be logged",
)
require(
    "INSERT INTO player_data (" in status_save,
    "the synchronous path must still be the one that inserts the baseline row",
)
complete_save = section(
    SQL_PLAYER,
    "// master save function\n\nbool sql_save_player(P_char ch, int type, int room)",
    "\n// status save",
)
require(
    "REMOVE_BIT(ch->runtime_flags, CHAR_RFLAG_NO_DB_BASELINE)" in complete_save,
    "the complete synchronous save must clear the baseline flag after success",
)

pulse = section(SAVE_PIPELINE, "void player_save_pipeline_pulse(void)", "\n}\n")
require(
    "player_save_apply_outcome::terminal_failure" in pulse and "ENOENT" in pulse,
    "the pulse must detect the missing-baseline apply failure",
)
require(
    "SET_BIT(ch->runtime_flags, CHAR_RFLAG_NO_DB_BASELINE)" in pulse
    and "outcome=missing_baseline" in pulse,
    "a missing baseline row must re-arm the synchronous fallback and be logged",
)

# --- 2. death terminal-save recovery ----------------------------------------------
require(
    "static void event_death_extract_retry(" in FIGHT
    and "static void schedule_death_extract_retry(" in FIGHT,
    "death recovery event is missing",
)
die_body = section(FIGHT, "void die(P_char ch, P_char killer)", "\nvoid ")
require(
    "!persistence_save_character_terminal(ch, RENT_DEATH)" in die_body
    and "schedule_death_extract_retry(ch, death_corpse_uid," in die_body,
    "die() must schedule the recovery when the terminal save fails",
)
retry = section(FIGHT, "static void event_death_extract_retry(P_char ch, P_char victim", "\nvoid die(")
require(
    "persistence_save_character_terminal(ch, RENT_DEATH)" in retry,
    "the recovery event must re-attempt the terminal save",
)
release = section(
    FIGHT,
    "static void release_after_terminal_death(P_char ch, const char *outcome)",
    "\n/** Record the refused death disposition",
)
require(
    "release_after_terminal_death(ch," in retry
    and "extract_char_after_terminal_save(ch)" in release,
    "the recovery event must complete saved-item extraction",
)
require(
    "GET_STAT(ch) != STAT_DEAD" in retry,
    "the recovery event must abandon extraction after resurrection",
)
require(
    "schedule_death_extract_retry(ch, context.corpse_uid, previous_delay * 2)" in retry,
    "a failed retry must reschedule with backoff",
)
require(
    "NULL, NULL, 0, &context" in FIGHT and "uint64_t corpse_uid;" in FIGHT,
    "the recovery event must retain corpse identity without binding its lifetime",
)
require(
    "#define DEATH_EXTRACT_RETRY_MAX" in FIGHT,
    "the retry backoff must be clamped",
)

# --- 4. extra descriptions / affects are replaced, not appended -------------------
extra_descr = section(
    SQL_PLAYER,
    "static bool sql_save_item_extra_descr(int item_id, P_obj obj, const char *table)",
    "\n// save a single item",
)
require(
    'DELETE FROM %s WHERE item_id = %d' in extra_descr,
    "extra descriptions must be cleared before they are re-inserted",
)
require(
    extra_descr.index("DELETE FROM %s") < extra_descr.index("INSERT INTO %s"),
    "the delete must precede the inserts",
)
require(
    "if (!obj->ex_description)\n\t\treturn true;" in extra_descr
    and "if (!obj || !obj->ex_description || !DB)" not in extra_descr,
    "an object that lost all its descriptions must still clear the stored rows",
)
require(
    "description_keys" in extra_descr,
    "the synchronous item save must skip duplicate in-memory descriptions",
)

item_affects = section(SQL_PLAYER, "static bool sql_save_item_affects(int item_id, P_obj obj)", "\n}\n")
require(
    "DELETE FROM player_item_affects WHERE item_id" in item_affects,
    "item affects must be cleared before they are re-inserted",
)
pet_affects = section(SQL_PLAYER, "static bool sql_save_pet_item_affects(int item_id, P_obj obj)", "\n}\n")
require(
    "DELETE FROM player_pet_item_affects WHERE item_id" in pet_affects,
    "pet item affects must be cleared before they are re-inserted",
)
require("is_dup" in pet_affects, "pet item affects must skip in-memory duplicates too")

batch = section(SQL_PLAYER, "// ------ Step 5: save affects and extra descriptions", "\tfree(flat);")
require(
    "if (obj->ex_description)" not in batch,
    "the batch path must call the description save unconditionally so stale rows are cleared",
)

for index in (
    "uk_item_descr",
    "uk_pet_item_descr",
    "uk_item_affect",
    "uk_pet_item_affect",
):
    require(index in MIGRATION, f"migration must add {index}")
require(
    MIGRATION.count("information_schema.statistics") == 4,
    "each unique key must be added behind an existence guard so the migration is re-runnable",
)
require(
    MIGRATION.count("MIN(id) AS keep_id") == 4,
    "the migration must deduplicate before adding each unique key",
)

# load tolerates the duplicates that already exist
require(
    "bool duplicate_description(" in LOAD_REPOSITORY,
    "the load path must recognise an exact duplicate description",
)
require(
    LOAD_REPOSITORY.count("if (duplicate_description(item.extra_descriptions, row[5], row[6]))") == 2,
    "player and pet item loads must both drop exact duplicate descriptions",
)

# --- 5. load diagnostics ----------------------------------------------------------
require(
    "const char *failed_component" in LOAD_REPOSITORY_H,
    "the load result must carry the failing repository stage back to the game thread",
)
execute = section(
    LOAD_REPOSITORY,
    "player_load_result player_load_repository_execute(MYSQL *connection,",
    "\n\tresult.metrics.transaction_usec",
)
for stage in ("status", "components", "items", "pets", "gameplay_reads", "bank", "budget"):
    require(f'stage("{stage}"' in execute, f"repository must name the {stage} stage on failure")

require(
    "component=snapshot" in LOAD_MATERIALIZE and "repository_component=%s" in LOAD_MATERIALIZE,
    "a refused snapshot must be logged with the failing repository component",
)
require(
    "component=item_graph" in LOAD_MATERIALIZE,
    "the SESSION03 item graph failure must be logged",
)
require(
    LOAD_MATERIALIZE.count("component=ownership") == 3,
    "all three ownership hydration failures must be logged",
)

# Full loads include one batched query for authoritative coin payloads.
require(
    "PLAYER_LOAD_QUERY_MAX = 23" in LOAD_REPOSITORY_H,
    "load query budget changed; re-check the 23-query ceiling",
)

# --- 7. account menu NULL guard ---------------------------------------------------
menu = section(ACCOUNT, "void display_account_menu(P_desc d, char *arg)", "\n}\n")
require(
    menu.lstrip().startswith("void display_account_menu(P_desc d, char *arg)\n{\n\tif (!arg)")
    or "if (!arg)" in menu.split("\n")[2],
    "display_account_menu must guard against a NULL arg, not dereference it",
)

# --- 8. saved-room fallback bounds ------------------------------------------------
enter_game = section(NANNY, "void enter_game(P_desc d)", "\n}\n")
require(
    "r_room = real_room(GET_ORIG_BIRTHPLACE(ch));" in enter_game,
    "enter_game must convert the original birthplace vnum to a world index",
)
require(
    enter_game.index("if (r_room < 0 || r_room > top_of_world)")
    < enter_game.index("if (zone_table[world[r_room].zone].flags & ZONE_CLOSED)"),
    "enter_game must bounds-check a restored room before indexing world and zone_table",
)

# --- 9. post-entry save routing ---------------------------------------------------
entry_save = NANNY[NANNY.index("auction_transaction_player_ready(ch);") :]
entry_save = entry_save[: entry_save.index("sql_connectIP(ch);")]
require(
    "if (!writeCharacter(ch, 1, NOWHERE))" in entry_save,
    "post-entry persistence must check the backend-neutral character save result",
)
require(
    entry_save.index("#ifndef __NO_MYSQL__")
    < entry_save.index("sql_save_player_core(ch)"),
    "client-free entry must not report the unavailable SQL-only core writer as a save failure",
)

newbie_grant = NANNY[NANNY.index("if (!GET_LEVEL(ch))") :]
newbie_grant = newbie_grant[: newbie_grant.index("else if (IS_SET(ch->specials.act2")]
require(
    "do_start_deferred_newbie_kit(ch, 0)" in newbie_grant
    and newbie_grant.index("writeCharacter(ch, 1, NOWHERE)")
    < newbie_grant.index("load_obj_to_newbies(ch)"),
    "new players need complete initialization and a durable authority baseline before one starter grant",
)
require(
    newbie_grant.count("load_obj_to_newbies(ch)") == 1,
    "the new-character entry branch must submit exactly one starter grant sequence",
)
require(
    "starter kit withheld" in newbie_grant,
    "starter-item publication must fail closed when the player baseline cannot be saved",
)

flat_terminal_save = section(
    write_character,
    "#ifdef __NO_MYSQL__\n\tif (!is_locker_char &&",
    "\n#endif",
)
require(
    "const bool establishing_baseline" in flat_terminal_save
    and "flatfile_item_repository_load_owner(" in flat_terminal_save,
    "the first flat-file save must reload the authoritative item-owner revision",
)
require(
    "item_ownership_runtime_hydrate_owner(owner, owner_revision)" in flat_terminal_save,
    "starter grants must use the persisted player-owner revision, not the pre-save revision",
)
require(
    "ownership_revision_sync_failed" in flat_terminal_save,
    "a failed ownership revision publication must fail closed and remain observable",
)

print("character persistence gap contracts ok")
