#!/usr/bin/env python3
"""Contracts for clean-shutdown ownership found by the Valgrind command sweep."""

from _paths import SRC
from pathlib import Path

root = Path(__file__).resolve().parents[2]
actcomm = (SRC / "actcomm.c").read_text()
comm = (SRC / "comm.c").read_text()
db = (SRC / "db.c").read_text()
debug = (SRC / "debug.c").read_text()
shop = (SRC / "shop.c").read_text()
sql = (SRC / "sql.c").read_text()


def body(source, start, end):
    section = source[source.index(start):]
    return section[:section.index(end)]


free_world = body(db, "void free_world(", "/* read direction data */")
assert "std::unordered_set<char *> freed_room_strings" in free_world
assert "freed_room_strings.insert(world[room].name).second" in free_world
assert "freed_room_strings.insert(world[room].description).second" in free_world
assert "while (world[room].ex_description)" in free_world
assert "world[room].ex_description = description->next;" in free_world
assert "free_social_messages();" in free_world
assert "free_shops();" in free_world
assert "fclose(mob_f);" in free_world and "fclose(obj_f);" in free_world

setup_dir = body(db, "void setup_dir(", "void renum_world(")
invalid_exit = setup_dir[:setup_dir.index("CREATE(world[room].dir_option[dir]")]
assert "fscanf(fl" in invalid_exit and "dir >= NUM_EXITS" in invalid_exit
assert "if (general_description)" in invalid_exit and "if (keyword)" in invalid_exit
assert "FREE(general_description);" in invalid_exit
assert "FREE(keyword);" in invalid_exit

renum_world = body(db, "void renum_world(", "void renum_zone_table(")
assert "struct room_direction_data *invalid_exit =" in renum_world
assert "world[room].dir_option[door];" in renum_world
assert "FREE(invalid_exit->general_description);" in renum_world
assert "FREE(invalid_exit->keyword);" in renum_world
assert "FREE(invalid_exit);" in renum_world

socials = body(actcomm, "void free_social_messages(", "// Standard log n hunt")
for field in ("char_no_arg", "others_no_arg", "char_found", "others_found", "vict_found",
              "not_found", "char_auto", "others_auto"):
    assert f"str_free(soc_mess_list[social].{field});" in socials
assert "FREE(soc_mess_list);" in socials and "list_top = -1;" in socials
social_boot = body(actcomm, "void boot_social_messages(", "void free_social_messages(")
assert "soc_mess_list[list_top] = {};" in social_boot

boot_shops = body(shop, "void boot_the_shops(", "void free_shops(")
assert "char *shop_record = fread_string(shop_f);" in boot_shops
assert boot_shops.count("FREE(shop_record);") >= 2
free_shops = body(shop, "void free_shops(", "void assign_the_shopkeepers(")
assert "str_free(shop_index[shop].type[type].keywords);" in free_shops
assert "FREE(shop_index[shop].type);" in free_shops
assert "FREE(shop_index);" in free_shops and "number_of_shops = 0;" in free_shops

game_loop = body(comm, "void game_loop(", "int get_from_q(")
assert "close_sockets(s);" in game_loop
assert "close(S);" in game_loop
assert "websocket_shutdown();" in game_loop
main = body(comm, "int main(", "// all text meant")
assert main.index("run_the_game(port, sslport);") < main.index("shutdown_mysql();")
assert main.index("shutdown_mysql();") < main.index("close_cmdlog();")

close_log = body(debug, "void close_cmdlog(", "void cmdlog(")
assert "fclose(cmdfile);" in close_log and "cmdfile = NULL;" in close_log
mysql_shutdown = sql[sql.rindex("void shutdown_mysql("):]
assert "sql_pool_shutdown();" in mysql_shutdown
assert "mysql_close(persistenceDB);" in mysql_shutdown
assert "mysql_close(DB);" in mysql_shutdown

print("Valgrind clean-shutdown ownership contracts passed")
