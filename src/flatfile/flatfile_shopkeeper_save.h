#ifndef DURIS_FLATFILE_SHOPKEEPER_SAVE_H
#define DURIS_FLATFILE_SHOPKEEPER_SAVE_H

#include <cstdint>
#include <string>

struct char_data;
typedef struct char_data *P_char;

enum class flatfile_shopkeeper_save_result
{
	ok,
	not_found,
	stale,
	invalid,
	custody_mismatch,
	capture_failure,
	io_error,
};

flatfile_shopkeeper_save_result flatfile_shopkeeper_save(const std::string &root, P_char shopkeeper,
							 uint32_t shop_id, int64_t saved_at,
							 std::string *error);
bool flatfile_shopkeeper_save_dirty(const std::string &root, int64_t saved_at, std::string *error);

#endif
