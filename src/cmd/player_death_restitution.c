#include "core/prototypes.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "player/player_death_restitution_staff.h"

#include <cstdio>
#include <cstring>

namespace
{
constexpr const char *RESTITUTION_SYNTAX =
	"Syntax: restitution begin | restitution chunk <hex> | restitution commit | restitution abort | restitution submit <hex>\r\n";

bool only_spaces(const char *text)
{
	if (!text)
		return true;
	while (*text)
	{
		if (*text != ' ' && *text != '\t' && *text != '\r' && *text != '\n')
			return false;
		++text;
	}
	return true;
}

void send_restitution_result(P_char ch, player_death_restitution_runtime_result result,
			     const player_death_restitution_runtime_submission &submission)
{
	char operation[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
	const bool have_operation =
		critical_operation_id_to_hex(submission.operation_id, operation, sizeof(operation));
	switch (result)
	{
	case player_death_restitution_runtime_result::accepted:
	case player_death_restitution_runtime_result::attached:
		if (have_operation)
		{
			char message[MAX_STRING_LENGTH] = {};
			std::snprintf(
				message, sizeof(message),
				"Restitution submission accepted for operation %s; the recipient remains fenced until completion.\r\n",
				operation);
			send_to_char(message, ch);
		}
		else
			send_to_char(
				"Restitution submission accepted; the recipient remains fenced until completion.\r\n",
				ch);
		return;
	case player_death_restitution_runtime_result::journal_uncertain:
		if (have_operation)
		{
			char message[MAX_STRING_LENGTH] = {};
			std::snprintf(
				message, sizeof(message),
				"Restitution submission for operation %s is journal-uncertain; the recipient remains fenced and do not retry it.\r\n",
				operation);
			send_to_char(message, ch);
		}
		else
			send_to_char(
				"Restitution submission is journal-uncertain; the recipient remains fenced and do not retry it.\r\n",
				ch);
		return;
	case player_death_restitution_runtime_result::unauthorized:
		send_to_char("You are not authorized to submit restitution plans.\r\n", ch);
		return;
	case player_death_restitution_runtime_result::identity_conflict:
		send_to_char("The approved plan actor does not match your staff identity.\r\n", ch);
		return;
	case player_death_restitution_runtime_result::recipient_online:
		send_to_char("The restitution recipient must be offline.\r\n", ch);
		return;
	case player_death_restitution_runtime_result::pending_save:
		send_to_char("The restitution recipient still has a pending save.\r\n", ch);
		return;
	case player_death_restitution_runtime_result::fence_unavailable:
		send_to_char(
			"The recipient save/login fence is unavailable; nothing was submitted.\r\n",
			ch);
		return;
	case player_death_restitution_runtime_result::coordinator_unavailable:
		send_to_char(
			"The critical command coordinator is unavailable; nothing was submitted.\r\n",
			ch);
		return;
	case player_death_restitution_runtime_result::overloaded:
		send_to_char("Too many restitution submissions are pending; try again later.\r\n",
			     ch);
		return;
	case player_death_restitution_runtime_result::journal_failure:
		send_to_char(
			"The restitution journal rejected the submission; nothing was submitted.\r\n",
			ch);
		return;
	case player_death_restitution_runtime_result::invalid_plan:
	default:
		send_to_char("That is not a canonical approved restitution plan.\r\n", ch);
		return;
	}
}
} // namespace

ACMD(do_restitution)
{
	if (!ch)
		return;
	if (IS_NPC(ch) || GET_LEVEL(ch) < FORGER)
	{
		send_to_char("You are not authorized to submit restitution plans.\r\n", ch);
		return;
	}

	char subcommand[MAX_INPUT_LENGTH] = {};
	char canonical_hex[MAX_INPUT_LENGTH] = {};
	char *remaining = one_argument(argument, subcommand);
	if (!remaining || !*subcommand)
	{
		send_to_char(RESTITUTION_SYNTAX, ch);
		return;
	}

	if (!str_cmp(subcommand, "begin"))
	{
		if (!only_spaces(remaining))
		{
			send_to_char(RESTITUTION_SYNTAX, ch);
			return;
		}
		const auto result =
			player_death_restitution_staff_begin(GET_NAME(ch), GET_LEVEL(ch));
		if (result == player_death_restitution_runtime_result::accepted)
			send_to_char(
				"Restitution payload staging started; append canonical chunks, then commit.\r\n",
				ch);
		else
			send_restitution_result(ch, result, {});
		return;
	}

	if (!str_cmp(subcommand, "chunk"))
	{
		remaining = one_argument(remaining, canonical_hex);
		if (!remaining || !*canonical_hex || !only_spaces(remaining))
		{
			send_to_char(RESTITUTION_SYNTAX, ch);
			return;
		}
		const auto result = player_death_restitution_staff_append_hex(
			GET_NAME(ch), GET_LEVEL(ch), canonical_hex, std::strlen(canonical_hex));
		if (result == player_death_restitution_runtime_result::accepted)
			send_to_char("Restitution payload chunk accepted.\r\n", ch);
		else
			send_restitution_result(ch, result, {});
		return;
	}

	if (!str_cmp(subcommand, "commit"))
	{
		if (!only_spaces(remaining))
		{
			send_to_char(RESTITUTION_SYNTAX, ch);
			return;
		}
		player_death_restitution_runtime_submission submission = {};
		const auto result = player_death_restitution_staff_commit(
			GET_NAME(ch), GET_LEVEL(ch), &submission);
		send_restitution_result(ch, result, submission);
		return;
	}

	if (!str_cmp(subcommand, "abort"))
	{
		if (!only_spaces(remaining))
		{
			send_to_char(RESTITUTION_SYNTAX, ch);
			return;
		}
		const auto result =
			player_death_restitution_staff_abort(GET_NAME(ch), GET_LEVEL(ch));
		if (result == player_death_restitution_runtime_result::accepted)
			send_to_char("Restitution payload staging aborted.\r\n", ch);
		else
			send_restitution_result(ch, result, {});
		return;
	}

	if (str_cmp(subcommand, "submit"))
	{
		send_to_char(RESTITUTION_SYNTAX, ch);
		return;
	}
	remaining = one_argument(remaining, canonical_hex);
	if (!remaining || !*canonical_hex || !only_spaces(remaining))
	{
		send_to_char(RESTITUTION_SYNTAX, ch);
		return;
	}

	player_death_restitution_runtime_submission submission = {};
	const player_death_restitution_runtime_result result =
		player_death_restitution_staff_submit_hex(GET_NAME(ch), GET_LEVEL(ch),
							  canonical_hex, std::strlen(canonical_hex),
							  &submission);
	send_restitution_result(ch, result, submission);
}
