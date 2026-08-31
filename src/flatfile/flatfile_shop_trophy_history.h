#ifndef DURIS_FLATFILE_SHOP_TROPHY_HISTORY_H
#define DURIS_FLATFILE_SHOP_TROPHY_HISTORY_H

#include <stdint.h>

#include <string>

enum class flatfile_shop_trophy_result
{
	ok,
	invalid,
	corrupt,
	io_error
};

flatfile_shop_trophy_result flatfile_shop_trophy_record(const char *root, int item, int value,
							int seller, int64_t occurred_at,
							std::string *error);
flatfile_shop_trophy_result flatfile_shop_trophy_count(const char *root, int item, int64_t now,
						       int *count, std::string *error);

#endif
