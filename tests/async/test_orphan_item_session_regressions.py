#!/usr/bin/env python3
"""Regressions for the defects found in the 2026-08-29 minimal-boot Memcheck sweep.

The sweep's headline finding was an orphan `player_items` row - a payload row the
ownership ledger had never heard of - which made the owning character permanently
unloadable. The load path was made tolerant of that separately; these are the
remaining defects the same session turned up, each of which is a source contract
rather than a runtime check because the code paths need a live world.

1. do_load handed a wizard-created object straight over with obj_to_char() and
   submitted no transfer, so it arrived carrying a uid with no ledger row. That is
   how the orphan was created in the first place.
2. extract_obj() deliberately does not retire the ledger row; the reasoning has to
   stay written down or someone will "fix" it onto a teardown path.
3. generate_modif() leaked its scratch copy and generate_desc() dropped every
   string its helpers returned - once per descriptor in the game, via ztestdesc.
4. free_char() released every player string except long_descr, so each login that
   set one leaked it.
5. do_build() leaked the Building it declined to keep.
6. The world prototype files stay open on purpose; a reader who "fixes" the
   descriptors Valgrind reports at exit breaks mob and object instantiation.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
actwiz = (root / "src/actwiz.c").read_text()
handler = (root / "src/handler.c").read_text()
utility = (root / "src/utility.c").read_text()
db = (root / "src/db.c").read_text()
buildings = (root / "src/buildings.c").read_text()
capture = (root / "src/player_snapshot_capture.c").read_text()

failures = []


def check(name, ok):
    print(("[PASS] " if ok else "[FAIL] ") + name)
    if not ok:
        failures.append(name)


# 1. Wizard-loaded objects reach the ownership ledger.
load = actwiz[actwiz.index("void do_load("):]
load = load[:load.index("void do_purge(")]
check("do_load establishes ownership instead of a bare obj_to_char",
      "submit_wizard_load_establish(ch, obj, to_room)" in load)
check("do_load discards the object when ownership will not commit",
      "extract_obj(obj, FALSE)" in load)
establish = actwiz[actwiz.index("bool submit_wizard_load_establish("):]
establish = establish[:establish.index("\n}")]
check("the establish is a same-owner creation transfer",
      "item_transfer_reason::creation" in establish
      and "owner, owner," in establish)
check("a room-bound object is established against the room, not the wizard",
      "item_owner_type::room" in establish and "item_owner_type::player" in establish)
completion = actwiz[actwiz.index("void wizard_load_completion("):]
completion = completion[:completion.index("\n}")]
check("the live move happens only once the transfer commits",
      "if (!committed)" in completion
      and "obj_to_char(object, actor)" in completion
      and "obj_to_room(object, context.room)" in completion)
check("an uncommitted establish does not leave the object in play",
      "extract_obj(object, FALSE)" in completion)

# 2. extract_obj's silence about the ledger is deliberate and documented.
extract_preamble = handler[:handler.index("void extract_obj(")]
check("extract_obj records why it leaves the ownership row alone",
      "does not retire" in extract_preamble[-900:]
      and "missing_payload_rows" in extract_preamble[-900:])

# 3. The description generators own and release their scratch strings.
modif = utility[utility.index("char *generate_modif("):]
modif = modif[:modif.index("\n}")]
check("generate_modif frees a rejected candidate", "str_free(buf);" in modif)
check("generate_modif hands back its own copy rather than duplicating again",
      "return str_dup(buf);" not in modif and "return buf;" in modif)
desc = utility[utility.index("void generate_desc("):]
desc = desc[:desc.index("\n}")]
check("generate_desc releases every generated string",
      desc.count("str_free(shape);") == 1
      and desc.count("str_free(appear);") == 1
      and desc.count("str_free(modif);") == 1)
check("generate_desc never passes a generator straight into snprintf",
      "generate_shape(ch))" not in desc.replace("shape = generate_shape(ch);", "")
      and "generate_modif(ch));" not in desc.replace("modif = generate_modif(ch);", ""))
check("generate_desc frees the description it replaces",
      "str_free(ch->player.short_descr);" in desc)

# 4. free_char releases long_descr on the player path.
free_char = db[db.index("void free_char("):]
free_char = free_char[:free_char.index("\n/* release memory allocated for an obj struct */")]
check("free_char frees a player's long_descr",
      "str_free(ch->player.long_descr);" in free_char.split("if (IS_NPC(ch))")[1])

# 5. do_build does not leak the building it rejects.
build = buildings[buildings.index("void do_build("):]
build = build[:build.index("\n// Called in place of die()")]
check("do_build deletes a building nothing took ownership of", "delete building;" in build)

# 6. The world prototype files are held open on purpose.
check("boot records why mob_f and obj_f are never closed",
      "read_object() fseek into them" in db and "not descriptors to close after boot" in db)

# The save-time detector that turns the remaining grant-path audit into data.
check("the save path reports an object the ownership ledger does not know",
      "outcome=unowned_object" in capture
      and "item_ownership_runtime_lookup(object->obj_uid, &ownership)" in capture)

if failures:
    print("\nFailed regression checks:")
    for f in failures:
        print("- " + f)
    raise SystemExit(1)
print("orphan-item session regressions passed")
