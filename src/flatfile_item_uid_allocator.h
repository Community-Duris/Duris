#ifndef DURIS_FLATFILE_ITEM_UID_ALLOCATOR_H
#define DURIS_FLATFILE_ITEM_UID_ALLOCATOR_H

#include <cstdint>
#include <string>

enum class flatfile_item_uid_result
{
	ok,
	invalid,
	exhausted,
	io_error
};

flatfile_item_uid_result flatfile_item_uid_reserve(const std::string &root, uint64_t count,
						   uint64_t *first, std::string *error);
flatfile_item_uid_result flatfile_item_uid_current(const std::string &root, uint64_t *next_uid,
						   uint64_t *revision, std::string *error);

#endif
