#include "economy/coin_transfer_accounting.h"

#include <cassert>
#include <cerrno>

int main()
{
	critical_command root = {};
	coin_transfer_payload payload = {};
	coin_transfer_result result = {};
	coin_transfer_accounting_context context = {};
	economic_source_event lifecycle = {};
	assert(!coin_transfer_accounting_source_event(economic_account_kind::wallet, 0, 4, false,
						      true, &lifecycle));
	assert(!coin_transfer_accounting_source_event(economic_account_kind::wallet, 17, UINT64_MAX,
						      false, true, &lifecycle));
	assert(!coin_transfer_accounting_source_event(economic_account_kind::wallet, 17, 4, false,
						      false, &lifecycle));
	assert(coin_transfer_accounting_source_event(economic_account_kind::wallet, 17, 4, false,
						     true, &lifecycle));
	const auto first_lifecycle = lifecycle;
	assert(lifecycle.kind == economic_source_kind::lifecycle && lifecycle.sequence == 0 &&
	       lifecycle.slot == 2);
	assert(coin_transfer_accounting_source_event(economic_account_kind::wallet, 17, 4, false,
						     true, &lifecycle));
	assert(lifecycle.source.bytes == first_lifecycle.source.bytes &&
	       lifecycle.generation.bytes == first_lifecycle.generation.bytes &&
	       lifecycle.sequence == first_lifecycle.sequence &&
	       lifecycle.slot == first_lifecycle.slot);
	assert(coin_transfer_accounting_source_event(economic_account_kind::wallet, 17, 5, false,
						     true, &lifecycle));
	assert(lifecycle.source.bytes == first_lifecycle.source.bytes &&
	       lifecycle.generation.bytes != first_lifecycle.generation.bytes);
	assert(coin_transfer_accounting_source_event(economic_account_kind::pile, 17, 4, true, true,
						     &lifecycle));
	assert(lifecycle.source.bytes != first_lifecycle.source.bytes && lifecycle.slot == 3);

#ifdef __NO_MYSQL__
	assert(coin_transfer_accounting_lock(nullptr, root, payload, &context) == ENOTSUP);
	assert(coin_transfer_accounting_record(nullptr, root, result, 0, context) == ENOTSUP);
	assert(coin_transfer_accounting_verify_retained(nullptr, root, 0, nullptr, 0) == ENOTSUP);
#else
	assert(coin_transfer_accounting_lock(nullptr, root, payload, &context) == EINVAL);
	assert(coin_transfer_accounting_record(nullptr, root, result, 0, context) == ENOTCONN);
	assert(coin_transfer_accounting_verify_retained(nullptr, root, 0, nullptr, 0) == EINVAL);
#endif
	return 0;
}
