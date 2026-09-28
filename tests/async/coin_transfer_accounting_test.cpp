#include "economy/coin_transfer_accounting.h"

#include <cassert>
#include <cerrno>

int main()
{
	critical_command root = {};
	coin_transfer_payload payload = {};
	coin_transfer_result result = {};
	coin_transfer_accounting_context context = {};

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
