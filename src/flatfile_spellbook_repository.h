#ifndef DURIS_FLATFILE_SPELLBOOK_REPOSITORY_H
#define DURIS_FLATFILE_SPELLBOOK_REPOSITORY_H

#include "flatfile_authority_transaction.h"

#include <cstdint>
#include <string>
#include <vector>

struct flatfile_spellbook_record
{
	uint32_t pid = 0;
	std::vector<int32_t> mobs;
	bool operator==(const flatfile_spellbook_record &) const = default;
};

enum class flatfile_spellbook_result
{
	ok,
	not_found,
	already_exists,
	unchanged,
	invalid,
	io_error
};

flatfile_spellbook_result
flatfile_spellbook_establish(const std::string &root,
			     const std::vector<flatfile_spellbook_record> &records,
			     std::string *error);
flatfile_spellbook_result flatfile_spellbook_list(const std::string &root, uint32_t pid,
						  std::vector<int32_t> *mobs, std::string *error);
flatfile_spellbook_result flatfile_spellbook_contains(const std::string &root, uint32_t pid,
						      int32_t mob_vnum, bool *contains,
						      std::string *error);
flatfile_spellbook_result flatfile_spellbook_add(const std::string &root, uint32_t pid,
						 int32_t mob_vnum, std::string *error);
flatfile_spellbook_result flatfile_spellbook_remove(const std::string &root, uint32_t pid,
						    int32_t mob_vnum, std::string *error);
flatfile_spellbook_result flatfile_spellbook_clear(const std::string &root, uint32_t pid,
						   std::string *error);
flatfile_spellbook_result flatfile_spellbook_prepare_clear(const std::string &root,
							   const flatfile_authority_lock &lock,
							   uint32_t pid,
							   flatfile_authority_operation *operation,
							   std::string *error);

#endif
