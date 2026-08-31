#ifndef DURIS_FLATFILE_CHARACTER_DELETE_H
#define DURIS_FLATFILE_CHARACTER_DELETE_H

#include <cstdint>
#include <string>

enum class flatfile_character_delete_result
{
	ok,
	already_deleted,
	not_found,
	conflict,
	invalid,
	io_error
};

flatfile_character_delete_result flatfile_character_delete(const std::string &root, int32_t pid,
							   const std::string &expected_name,
							   std::string *error);

#endif
