#ifndef DURIS_GITHYANKI_ITEM_POLICY_H
#define DURIS_GITHYANKI_ITEM_POLICY_H

#include <cstddef>
struct char_data;

// Shared original class precedence and messages. These values grant no level
// transition, native factory, source, UID, admission or publication authority.
struct githyanki_level_item_choice
{
	int vnum = 0;
	const char *name_format = nullptr;
	const char *player_message = nullptr;
	const char *room_message = nullptr;
};
githyanki_level_item_choice githyanki_level_item_select(char_data *) noexcept;
// Original constant-literal keyword policy shared by ordinary and owned paths.
// snprintf length/truncation semantics; an unsupported choice leaves output intact.
int githyanki_level_item_format_keywords(char *, size_t, int vnum, const char *name) noexcept;

#endif
