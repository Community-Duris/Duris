#ifndef FLATFILE_CRAFT_PROGRESSION_H
#define FLATFILE_CRAFT_PROGRESSION_H

#include "flatfile/flatfile_player_snapshot_file.h"
#include "item/craft_recipe_continuation.h"

inline std::string flatfile_craft_receipt_filename(uint32_t pid,
						   const critical_operation_id &operation,
						   bool obligation = false)
{
	constexpr char hex[] = "0123456789abcdef";
	std::string filename = std::to_string(pid) + "-";
	for (uint8_t byte : operation.bytes)
	{
		filename += hex[byte >> 4];
		filename += hex[byte & 15];
	}
	return filename + (obligation ? ".craft-obligation" : ".craft");
}

inline flatfile_player_load_result
flatfile_craft_receipt_read(const std::string &root, uint32_t pid,
			    const critical_operation_id &operation, bool obligation,
			    player_snapshot *stored, std::string *error)
{
	const auto read = flatfile_player_snapshot_read_file(
		flatfile_player_snapshot_file::player_directory(root),
		flatfile_craft_receipt_filename(pid, operation, obligation), pid, stored, error);
	if (read != flatfile_player_load_result::ok)
		return read;
	if (stored->schema_version != PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION ||
	    stored->death || !stored->quest_xp_receipts.empty() ||
	    !stored->spell_effect_receipts.empty() || stored->craft_receipts.size() != 1 ||
	    stored->craft_receipts[0].operation_id.bytes != operation.bytes)
		return flatfile_player_load_result::invalid;
	return flatfile_player_load_result::ok;
}

inline bool flatfile_craft_receipt_equal(const player_craft_receipt_snapshot &left,
					 const player_craft_receipt_snapshot &right)
{
	return left.operation_id.bytes == right.operation_id.bytes &&
	       left.discipline == right.discipline && left.experience == right.experience;
}

#endif
