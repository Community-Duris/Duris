#ifndef DURIS_FLATFILE_SHOPKEEPER_RESTORE_H
#define DURIS_FLATFILE_SHOPKEEPER_RESTORE_H

#include <string>

enum class flatfile_shopkeeper_restore_result
{
	ok,
	not_found,
	invalid,
	materialize_failure,
	publish_failure,
	io_error,
};

flatfile_shopkeeper_restore_result flatfile_shopkeeper_restore_catalog(const std::string &root,
								       std::string *error);

#endif
