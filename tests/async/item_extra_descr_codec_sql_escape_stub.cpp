// The focused item-transfer harness does not link the legacy sql_player.c module.
// Fail closed if the codec ever reaches its legacy non-spellbook escape path;
// native spellbook encoding used by crafting does not call this symbol.
char *sql_escape_string(const char *)
{
	return nullptr;
}
