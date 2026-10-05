#ifndef SHOP_TRADE_RUNTIME_H
#define SHOP_TRADE_RUNTIME_H

#include "flatfile/flatfile_shopkeeper_repository.h"
#include "economy/shop_trade_command.h"
#include "core/structs.h"

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
shop_trade_runtime_build_payload(P_char player, P_char keeper, P_obj selected, P_obj stock,
				 P_obj destination, uint32_t shop_id, shop_trade_action action,
				 int64_t price, shop_trade_payload *payload);
// Explicit v6 source builder. These are original checkpoint preimage facts,
// never a grant of native authority; the producer owns their exact tokens and
// the native transaction independently verifies status/revision/full payload.
struct player_shop_checkpoint_stage;
shop_trade_payload_build_result shop_trade_runtime_build_accounted_payload(
	P_char player, P_char keeper, P_obj selected, P_obj stock, P_obj destination,
	uint32_t shop_id, shop_trade_action action, int64_t price,
	const player_shop_checkpoint_stage &player_status, uint64_t native_shop_revision,
	shop_trade_payload *payload);
bool shop_trade_runtime_object_matches_accounted_payload(P_obj selected,
							 const shop_trade_payload &payload);
bool shop_trade_runtime_object_matches_payload(P_obj selected, const shop_trade_payload &payload);

#ifndef __NO_MYSQL__
struct st_mysql;
struct economic_sql_shop_trade_publication;
class shop_trade_native_publication_owner;
// Only the integrated native publication owner can consume its original locked
// projection. An observed revision or caller-supplied boolean grants no access.
class shop_trade_current_runtime_owner final
{
    private:
	friend class shop_trade_native_publication_owner;
	static bool publish(st_mysql *, const shop_trade_payload &,
			    const economic_sql_shop_trade_publication &) noexcept;
};
#endif

#endif
