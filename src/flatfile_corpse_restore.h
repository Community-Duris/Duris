#ifndef DURIS_FLATFILE_CORPSE_RESTORE_H
#define DURIS_FLATFILE_CORPSE_RESTORE_H

#include <string>

enum class flatfile_corpse_restore_result
{
	ok,
	not_found,
	invalid,
	unknown_prototype,
	unknown_room,
	allocation_failure,
	item_failure,
	publish_failure,
	io_error,
};

flatfile_corpse_restore_result flatfile_corpse_restore_catalog(const std::string &root,
							       std::string *error);

#endif
