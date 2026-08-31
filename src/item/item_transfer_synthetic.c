#include "item/item_transfer_synthetic.h"

#include <utility>

bool item_transfer_synthetic_submit(const item_transfer_payload &payload,
				    critical_operation_id *operation_id_out)
{
	if (!operation_id_out)
		return false;
	critical_operation_id operation_id = {};
	critical_command command = {};
	if (!critical_operation_id_generate(&operation_id) ||
	    !item_transfer_command_build(&command, operation_id, payload,
					 critical_source_site::operator_repair,
					 critical_deadline_class::background))
		return false;
	const critical_submit_result submitted =
		critical_command_coordinator_submit(std::move(command));
	if (submitted != critical_submit_result::accepted &&
	    submitted != critical_submit_result::attached)
		return false;
	*operation_id_out = operation_id;
	return true;
}

bool item_transfer_synthetic_decode_completion(const critical_completion &completion,
					       item_transfer_result *result, bool *committed)
{
	if (!result || !committed ||
	    !item_transfer_command_decode_result(completion.result_payload.data(),
						 completion.result_size, result))
		return false;
	*committed = completion.outcome == critical_apply_outcome::applied ||
		     completion.outcome == critical_apply_outcome::already_applied;
	return true;
}
