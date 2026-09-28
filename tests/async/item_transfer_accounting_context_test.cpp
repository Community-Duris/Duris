#include "item/item_transfer_repository.h"
#include <cassert>
#include <cerrno>

bool critical_command_repository_begin_inbox_in_transaction(MYSQL *, const critical_command &)
{
	return false;
}

bool critical_command_repository_finish_item_transfer_in_transaction(MYSQL *,
								     const critical_command &,
								     const item_transfer_result &)
{
	return false;
}

int main()
{
	item_transfer_accounting_context ctx = {};
	assert(ctx.child_index == 0);
	assert(ctx.line_index_base == 0);
	for (auto byte : ctx.root_operation_id.bytes)
		assert(byte == 0);

	ctx.child_index = 2;
	ctx.line_index_base = 10;
	ctx.root_operation_id.bytes[0] = 0xab;

	critical_command cmd = {};
	item_transfer_result res = {};
	unsigned int code = 0;
	bool mutation = false;

	// With nullptr connection, returns false and EINVAL or EPROTONOSUPPORT
	errno = 0;
	bool ok = item_transfer_repository_execute(nullptr, cmd, &res, &code, &mutation, nullptr,
						   &ctx);
	assert(!ok);
	assert(errno == EINVAL || errno == EPROTONOSUPPORT);

	return 0;
}
