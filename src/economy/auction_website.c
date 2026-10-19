#include "economy/auction_website.h"
#include "economy/economic_state.h"

#include <algorithm>
#include <climits>
#include <cstring>
#include <openssl/sha.h>
#include <strings.h>

bool auction_website_build_bid(const char *account, uint32_t pid, uint32_t auction_id,
			       int64_t copper, const char *request_id, critical_command *command)
{
	if (!command || !account || !*account ||
	    strlen(account) > CURRENCY_ACCOUNT_NAME_MAX_BYTES || !request_id || !*request_id ||
	    strlen(request_id) > 128 || !auction_id || copper <= 0 || copper > UINT_MAX)
		return false;
	economic_player_state player;
	if (!economic_state_find(pid, &player) || strcasecmp(account, player.account.c_str()))
		return false;
	std::string canonical = account;
	for (auto &c : canonical)
		if (c >= 'A' && c <= 'Z')
			c += 'a' - 'A';
	critical_entity_key key{};
	if (!currency_account_key(canonical.c_str(), player.racewar, &key))
		return false;
	std::string identity = "duris:web:auction:v1:" + canonical + ":" + request_id;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> hash{};
	SHA256(reinterpret_cast<const uint8_t *>(identity.data()), identity.size(), hash.data());
	critical_operation_id id{};
	std::copy_n(hash.begin(), id.bytes.size(), id.bytes.begin());
	auction_command_payload payload{};
	payload.action = auction_action::bid;
	payload.actor_pid = pid;
	payload.auction_id = auction_id;
	payload.racewar = player.racewar;
	std::copy(canonical.begin(), canonical.end(), payload.account_name.begin());
	payload.expected_wallet_revision = UINT64_MAX;
	payload.expected_bank_revision = UINT64_MAX;
	payload.value = copper;
	return auction_command_build(command, id, payload, critical_source_site::website,
				     critical_deadline_class::interactive);
}
