#include "item/economic_accounting_item_reference.h"
#include <cassert>
#include <cerrno>

int main()
{
	economic_accounting_item_reference ref = {};
	ref.operation_id.bytes[0] = 1;
	ref.line_index = 0;
	ref.event_index = 0;
	ref.child_index = 1;
	ref.item_uid = 100;
	ref.before_revision = 1;
	ref.after_revision = 2;
	ref.legacy_operation_id.bytes[0] = 2;
	ref.legacy_event_index = 0;

	// Valid reference passes
	assert(economic_accounting_item_reference_validate(ref));

	// line_index >= 3000 fails
	ref.line_index = 3000;
	ref.event_index = 3000;
	assert(!economic_accounting_item_reference_validate(ref));
	ref.line_index = 0;
	ref.event_index = 0;

	// event_index != line_index fails
	ref.event_index = 1;
	assert(!economic_accounting_item_reference_validate(ref));
	ref.event_index = 0;

	// child_index > 64 fails
	ref.child_index = 65;
	assert(!economic_accounting_item_reference_validate(ref));
	ref.child_index = 64;
	assert(economic_accounting_item_reference_validate(ref));

	// item_uid == 0 fails
	ref.item_uid = 0;
	assert(!economic_accounting_item_reference_validate(ref));
	ref.item_uid = 100;

	// after_revision <= before_revision fails
	ref.after_revision = 1;
	assert(!economic_accounting_item_reference_validate(ref));
	ref.after_revision = 0;
	assert(!economic_accounting_item_reference_validate(ref));
	ref.after_revision = 2;

	// Zero operation_id fails
	ref.operation_id = {};
	assert(!economic_accounting_item_reference_validate(ref));
	ref.operation_id.bytes[15] = 9;

	// Zero legacy_operation_id fails
	ref.legacy_operation_id = {};
	assert(!economic_accounting_item_reference_validate(ref));
	ref.legacy_operation_id.bytes[15] = 9;

	// Now valid again
	assert(economic_accounting_item_reference_validate(ref));

#ifdef __NO_MYSQL__
	errno = 0;
	assert(!economic_accounting_item_reference_insert(nullptr, ref));
	assert(errno == ENOTSUP);

	errno = 0;
	assert(!economic_accounting_item_reference_insert_batch(nullptr, {}));
	assert(errno == ENOTSUP);

	errno = 0;
	assert(!economic_accounting_item_reference_find_by_legacy(nullptr, ref.legacy_operation_id,
								  0, nullptr));
	assert(errno == ENOTSUP);

	errno = 0;
	assert(!economic_accounting_item_reference_find_by_operation(nullptr, ref.operation_id,
								     nullptr));
	assert(errno == ENOTSUP);

	errno = 0;
	assert(!economic_accounting_item_reference_find_history(nullptr, 100, nullptr));
	assert(errno == ENOTSUP);
#endif

	return 0;
}
