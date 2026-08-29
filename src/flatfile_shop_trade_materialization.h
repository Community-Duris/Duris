#ifndef DURIS_FLATFILE_SHOP_TRADE_MATERIALIZATION_H
#define DURIS_FLATFILE_SHOP_TRADE_MATERIALIZATION_H

#include "flatfile_authority_transaction.h"
#include "flatfile_item_repository.h"
#include "player_snapshot.h"
#include "shop_trade_command.h"

#include <string>
#include <vector>

enum class flatfile_shop_trade_materialization_result
{
	ok,
	unchanged,
	invalid,
	io_error
};

struct flatfile_shop_trade_materialization_mutation
{
	flatfile_authority_after_image after_image;
};

struct flatfile_shop_trade_materialization_health
{
	uint64_t revision = 0;
	uint64_t events = 0;
	uint64_t encoded_bytes = 0;
	uint64_t reclaimable_events = 0;
	uint64_t maximum_events = 0;
	uint64_t maximum_bytes = 0;
	bool near_capacity = false;
};

flatfile_shop_trade_materialization_result flatfile_shop_trade_materialization_prepare(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &operation_id, const shop_trade_payload &payload,
	flatfile_shop_trade_materialization_mutation *mutation, std::string *error);
flatfile_shop_trade_materialization_result flatfile_item_transfer_materialization_prepare(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &operation_id, const item_transfer_payload &payload,
	flatfile_shop_trade_materialization_mutation *mutation, std::string *error);

flatfile_shop_trade_materialization_result flatfile_shop_trade_materialization_reconcile(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t player_pid,
	const std::vector<flatfile_item_ownership_record> &owned, player_snapshot *snapshot,
	std::string *error);
flatfile_shop_trade_materialization_result flatfile_shop_trade_materialization_read_health(
	const std::string &root, const flatfile_authority_lock &lock,
	flatfile_shop_trade_materialization_health *health, std::string *error);
flatfile_shop_trade_materialization_result
flatfile_shop_trade_materialization_prepare_player_remove(const std::string &root,
							  const flatfile_authority_lock &lock,
							  uint32_t player_pid,
							  flatfile_authority_operation *operation,
							  std::string *error);

#endif
