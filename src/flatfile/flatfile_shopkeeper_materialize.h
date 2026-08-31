#ifndef DURIS_FLATFILE_SHOPKEEPER_MATERIALIZE_H
#define DURIS_FLATFILE_SHOPKEEPER_MATERIALIZE_H

#include "flatfile/flatfile_shopkeeper_repository.h"
#include "player_load_items.h"

#include <cstdint>
#include <string>

struct char_data;
typedef struct char_data *P_char;

enum class flatfile_shopkeeper_materialize_result
{
	ok,
	not_found,
	invalid,
	unknown_mobile,
	unknown_room,
	allocation_failure,
	item_failure,
	io_error,
};

struct flatfile_materialized_shopkeeper
{
	P_char character = nullptr;
	int room_rnum = -1;
	uint64_t owner_revision = 0;
};

flatfile_shopkeeper_materialize_result
flatfile_shopkeeper_materialize(const std::string &root, const flatfile_shopkeeper_record &record,
				flatfile_materialized_shopkeeper *materialized,
				player_load_item_materialize_metrics *item_metrics,
				std::string *error);

#endif
