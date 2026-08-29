#ifndef SHOP_TRADE_RUNTIME_H
#define SHOP_TRADE_RUNTIME_H

#include "flatfile_shopkeeper_repository.h"
#include "shop_trade_command.h"
#include "structs.h"

#include <cstddef>
#include <cstdint>
#include <vector>

enum class shop_trade_payload_build_result : uint8_t
{
	ok,
	invalid,
	unavailable,
	capture_failure,
};

bool shop_trade_runtime_replace_revisions(const std::vector<flatfile_shopkeeper_record> &records);
bool shop_trade_runtime_revision(uint32_t shop_id, uint64_t *revision);
bool shop_trade_runtime_can_advance(uint32_t shop_id, uint64_t expected_revision,
				    uint64_t new_revision);
bool shop_trade_runtime_advance(uint32_t shop_id, uint64_t expected_revision,
				uint64_t new_revision);
void shop_trade_runtime_reset_for_tests(void);

shop_trade_payload_build_result
shop_trade_runtime_build_payload(P_char player, P_obj selected, P_obj stock, P_obj destination,
				 uint32_t shop_id, shop_trade_action action, int64_t price,
				 shop_trade_payload *payload);
bool shop_trade_runtime_object_matches_payload(P_obj selected, const shop_trade_payload &payload);

#endif
