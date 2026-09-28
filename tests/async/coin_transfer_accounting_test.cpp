#include "economy/coin_transfer_accounting.h"
#include "economy/economic_accounting_plan.h"

#include <cassert>
#include <cerrno>

int main()
{
	critical_command root_cmd = {};
	root_cmd.operation_id.bytes[0] = 0x42;

	coin_transfer_payload payload = {};
	payload.source.change.operation_id.bytes[0] = 0x01;
	payload.destination.change.operation_id.bytes[0] = 0x02;

	// Source: 100 copper before, 50 copper after (transferred 50 copper)
	payload.source.before = { 100, 0, 0, 0 };
	payload.source.after = { 50, 0, 0, 0 };

	// Destination: 0 copper before, 50 copper after
	payload.destination.before = { 0, 0, 0, 0 };
	payload.destination.after = { 50, 0, 0, 0 };

	coin_transfer_result result = {};
	result.wallets[0].wallet_revision = 5;
	result.piles[1].max_item_revision = 10;

#ifdef __NO_MYSQL__
	// In flatfile / NO_MYSQL mode, returns false with ENOTSUP
	bool ok = coin_transfer_accounting_record(nullptr, root_cmd, payload, result);
	assert(!ok);
	assert(errno == ENOTSUP);
#else
	// In MySQL mode, nullptr connection fails with EINVAL
	bool ok = coin_transfer_accounting_record(nullptr, root_cmd, payload, result);
	assert(!ok);
	assert(errno == EINVAL);
#endif

	return 0;
}
