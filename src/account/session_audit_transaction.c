#include "account/session_audit_transaction.h"

#include "persistence/critical_command_coordinator.h"
#include "prototypes.h"
#include "utils.h"

#include <ctime>

bool session_audit_transaction_submit(P_char character, session_audit_event event)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0)
		return false;
	critical_operation_id operation_id = {};
	critical_command command = {};
	const session_audit_payload payload = { static_cast<uint32_t>(GET_PID(character)), event,
						time(nullptr) };
	if (!critical_operation_id_generate(&operation_id) ||
	    !session_audit_command_build(&command, operation_id, payload))
		return false;
	const critical_submit_result submitted =
		critical_command_coordinator_submit(std::move(command));
	return submitted == critical_submit_result::accepted ||
	       submitted == critical_submit_result::attached;
}
