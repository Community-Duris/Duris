#include "world/zone_touch_command.h"
#include "world/zone_touch_repository.h"

#include <cassert>
#include <cerrno>

int main()
{
	critical_operation_id op = {};
	op.bytes[0] = 0x11;
	op.bytes[15] = 0x22;

	zone_touch_payload payload = {};
	payload.zone_number = 42;
	payload.toucher_pid = 100;
	payload.boot_time = 1000;
	payload.touched_at = 2000;
	payload.group_size = 2;
	payload.participant_pids[0] = 100;
	payload.participant_pids[1] = 101;
	payload.epic_value = 50;
	payload.alignment_delta = 1;
	payload.reset_requested = 0;
	payload.record_zone = 1;

	critical_command command = {};
	assert(zone_touch_command_build(&command, op, payload));
	assert(command.type == critical_command_type::zone);

	zone_touch_result result = {};
	unsigned int result_code = 0;
	bool mutation_applied = false;

	zone_touch_accounting_context ctx = { op, 1, 100 };
	assert(critical_operation_id_equal(ctx.root_operation_id, op));
	assert(ctx.child_index == 1);
	assert(ctx.line_index_base == 100);

	critical_operation_id derived1 = {}, derived2 = {};
	assert(zone_touch_derive_award_id(op, 100, &derived1));
	assert(zone_touch_derive_award_id(op, 101, &derived2));
	assert(!critical_operation_id_is_zero(derived1));
	assert(!critical_operation_id_equal(derived1, derived2));

#ifdef __NO_MYSQL__
	errno = 0;
	bool ok = zone_touch_repository_execute(nullptr, command, &result, &result_code,
						&mutation_applied);
	assert(!ok);
	assert(errno == ENOTSUP);
	assert(!mutation_applied);
#endif

	return 0;
}
