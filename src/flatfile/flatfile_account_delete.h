#ifndef DURIS_FLATFILE_ACCOUNT_DELETE_H
#define DURIS_FLATFILE_ACCOUNT_DELETE_H

#include <string>

enum class flatfile_account_delete_result
{
	ok,
	already_deleted,
	not_found,
	conflict,
	invalid,
	io_error
};

flatfile_account_delete_result flatfile_account_delete(const std::string &root,
						       const std::string &account_name,
						       std::string *error);

#endif
