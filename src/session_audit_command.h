#ifndef SESSION_AUDIT_COMMAND_H
#define SESSION_AUDIT_COMMAND_H

#include "critical_command.h"

#include <array>

constexpr uint16_t SESSION_AUDIT_PAYLOAD_VERSION = 1;
constexpr size_t SESSION_AUDIT_RESULT_BYTES = 16;

enum class session_audit_event : uint8_t
{
	login = 1,
	logout = 2,
};

struct session_audit_payload
{
	uint32_t pid;
	session_audit_event event;
	int64_t occurred_at;
};

using session_audit_result = session_audit_payload;

bool session_audit_command_build(critical_command *command, critical_operation_id operation_id,
				 const session_audit_payload &payload);
bool session_audit_command_decode_payload(const critical_command &command,
					  session_audit_payload *payload);
bool session_audit_command_encode_result(const session_audit_result &result,
					 std::array<uint8_t, SESSION_AUDIT_RESULT_BYTES> *encoded);
bool session_audit_command_decode_result(const uint8_t *encoded, size_t size,
					 session_audit_result *result);

#endif
