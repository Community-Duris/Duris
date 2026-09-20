#include "core/prototypes.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "player/player_death_restitution_staff.h"

#include <cstdio>
#include <cstring>

namespace
{
constexpr const char *RESTITUTION_SYNTAX =
	"Syntax: restitution help | guide | begin | chunk <hex> | commit | abort | "
	"status <operation-id> | submit <hex>\r\n";

constexpr const char *RESTITUTION_GUIDE =
	"Player-death restitution is a protected operator workflow; this game command "
	"does not inspect SQL or approve a hand-written payload.\r\n"
	"phase=read-only-preparation: run the protected inspect, review the plan, then "
	"explicitly approve and export the canonical payload.\r\n"
	"phase=staging: use restitution begin, submit every exported chunk in order, "
	"then restitution commit. Staging is actor-bound, in memory, bounded, and not durable.\r\n"
	"phase=admission: commit validates the canonical payload before acquiring the "
	"recipient fence or entering the critical-command coordinator.\r\n"
	"phase=durability: admission/queueing is not delivery. Use the operation identity "
	"and restitution status <operation-id>; do not submit the same operation again.\r\n"
	"phase=verification: this runtime cannot claim exact target readback in-band. "
	"Use the protected verify/reconciliation path from the same plan; only that read-only "
	"check may confirm delivery.\r\n"
	"retry rule: journal-uncertain operations must remain fenced; DO NOT RETRY them.\r\n"
	"commands: restitution begin; restitution chunk <hex>; restitution commit; "
	"restitution status <operation-id>; restitution abort.\r\n";

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

void send_staging_progress(P_char ch, const char *prefix, bool commit_ready)
{
	player_death_restitution_staff_staging_status status = {};
	if (!player_death_restitution_staff_get_staging_status(GET_NAME(ch), GET_LEVEL(ch),
							       &status))
	{
		send_to_char("phase=staging; progress is unavailable for this staff identity.\r\n",
			     ch);
		return;
	}
	char message[MAX_STRING_LENGTH] = {};
	std::snprintf(
		message, sizeof(message),
		"%s phase=staging; accepted_chunks=%zu/%zu accepted_hex_bytes=%zu/%zu; "
		"next: %s; retry: safe only by continuing this actor-bound staging session.\r\n",
		prefix ? prefix : "Restitution staging.", status.accepted_chunks, status.max_chunks,
		status.accepted_hex_bytes, status.max_hex_bytes,
		commit_ready ? "restitution commit when all exported chunks are present" :
			       "send the next exported chunk in order, then commit");
	send_to_char(message, ch);
}

void send_restitution_result(P_char ch, player_death_restitution_runtime_result result,
			     const player_death_restitution_runtime_submission &submission)
{
	char operation[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
	const bool have_operation =
		critical_operation_id_to_hex(submission.operation_id, operation, sizeof(operation));
	char message[MAX_STRING_LENGTH] = {};
	switch (result)
	{
	case player_death_restitution_runtime_result::accepted:
		std::snprintf(
			message, sizeof(message),
			"phase=admission; operation %s admitted but durability is not confirmed; "
			"delivery=not complete; recipient fence=held; next: restitution status %s; "
			"retry: do not submit this operation again.\r\n",
			have_operation ? operation : "(identity unavailable)",
			have_operation ? operation : "<operation-id>");
		send_to_char(message, ch);
		return;
	case player_death_restitution_runtime_result::awaiting_durability:
		std::snprintf(message, sizeof(message),
			      "phase=durability; operation %s is queued for journal durability; "
			      "durability=awaiting; delivery=not complete; recipient fence=held; "
			      "next: restitution status %s; retry: do not retry or resubmit.\r\n",
			      have_operation ? operation : "(identity unavailable)",
			      have_operation ? operation : "<operation-id>");
		send_to_char(message, ch);
		return;
	case player_death_restitution_runtime_result::attached:
		std::snprintf(
			message, sizeof(message),
			"phase=idempotency; operation %s already exists; no new submission was made; "
			"delivery=not complete; next: restitution status %s; retry: do not resubmit.\r\n",
			have_operation ? operation : "(identity unavailable)",
			have_operation ? operation : "<operation-id>");
		send_to_char(message, ch);
		return;
	case player_death_restitution_runtime_result::journal_uncertain:
		std::snprintf(
			message, sizeof(message),
			"phase=durability; operation %s is journal-uncertain; durability=uncertain; "
			"delivery=not complete; recipient fence=held; next: use reconciliation/recovery "
			"and restitution status %s; retry: DO NOT RETRY this operation.\r\n",
			have_operation ? operation : "(identity unavailable)",
			have_operation ? operation : "<operation-id>");
		send_to_char(message, ch);
		return;
	case player_death_restitution_runtime_result::unauthorized:
		send_to_char(
			"phase=authorization; refused: staff authorization is required; next: use an "
			"approved staff identity; retry: safe after authorization; no operation was admitted.\r\n",
			ch);
		return;
	case player_death_restitution_runtime_result::identity_conflict:
		send_to_char(
			"phase=admission; refused: actor or protected operation identity conflicts with "
			"the approved payload; next: obtain a matching protected export; retry: do not "
			"reuse the conflicting payload.\r\n",
			ch);
		return;
	case player_death_restitution_runtime_result::recipient_online:
		send_to_char(
			"phase=fence; refused: recipient is online; delivery=not complete; next: wait "
			"for the recipient to be fully offline, then commit; retry: safe only after the "
			"offline fence can be acquired.\r\n",
			ch);
		return;
	case player_death_restitution_runtime_result::pending_save:
		send_to_char(
			"phase=fence; refused: recipient has a pending save; delivery=not complete; "
			"next: wait for the pending save to drain, then commit; retry: safe only after "
			"the save fence is clear.\r\n",
			ch);
		return;
	case player_death_restitution_runtime_result::fence_unavailable:
		send_to_char(
			"phase=fence; refused: recipient save/login fence is unavailable; nothing was "
			"submitted; next: resolve the target fence and commit; retry: safe after the "
			"fence is available.\r\n",
			ch);
		return;
	case player_death_restitution_runtime_result::coordinator_unavailable:
		send_to_char(
			"phase=admission; refused: critical-command coordinator is unavailable; "
			"nothing was submitted; next: restore the coordinator, then commit; retry: safe "
			"after availability is confirmed.\r\n",
			ch);
		return;
	case player_death_restitution_runtime_result::overloaded:
		send_to_char(
			"phase=admission; refused: bounded restitution capacity is full; nothing was "
			"submitted; next: wait for pending work to drain; retry: safe later, not as a "
			"duplicate while an operation identity is active.\r\n",
			ch);
		return;
	case player_death_restitution_runtime_result::journal_failure:
		send_to_char(
			"phase=durability; refused: the journal rejected admission; durability=failed; "
			"nothing was submitted; next: preserve the evidence and resolve the journal "
			"failure; retry: do not retry unchanged until it is resolved.\r\n",
			ch);
		return;
	case player_death_restitution_runtime_result::duplicate_staging:
		send_to_char(
			"phase=staging; refused: existing staged data was retained; nothing was discarded; "
			"next: continue with the existing chunks or use restitution abort; retry: do not "
			"begin again over staged work.\r\n",
			ch);
		return;
	case player_death_restitution_runtime_result::no_staging:
		send_to_char(
			"phase=staging; refused: no active staging session exists and no staged data exists; "
			"no restitution was submitted; next: restitution begin; retry: safe after beginning "
			"a new actor-bound session.\r\n",
			ch);
		return;
	case player_death_restitution_runtime_result::malformed_chunk:
		send_to_char(
			"phase=staging; refused: chunk is empty or contains non-hex characters; no staged "
			"data changed; next: resend the exact exported hex chunk; retry: safe after correction.\r\n",
			ch);
		return;
	case player_death_restitution_runtime_result::incomplete_chunk:
		send_to_char(
			"phase=staging; refused: chunk is odd-length or the staged payload is incomplete; "
			"no staged data changed; next: send complete byte-aligned exported chunks; retry: "
			"safe after correction.\r\n",
			ch);
		return;
	case player_death_restitution_runtime_result::chunk_limit:
		send_to_char(
			"phase=staging; refused: chunk count or byte bound would be exceeded; no staged "
			"data changed; next: regenerate a bounded canonical export; retry: do not split or "
			"edit the payload by hand.\r\n",
			ch);
		return;
	case player_death_restitution_runtime_result::invalid_plan:
	default:
		send_to_char(
			"phase=canonical-validation; refused: payload is not the exact canonical approved "
			"plan; no recipient fence or delivery claim; next: rerun inspect -> plan -> export "
			"and approve the new evidence; retry: do not retry unchanged.\r\n",
			ch);
		return;
	}
}

const char *phase_name(player_death_restitution_runtime_operation_phase phase)
{
	switch (phase)
	{
	case player_death_restitution_runtime_operation_phase::awaiting_durability:
		return "awaiting-durability";
	case player_death_restitution_runtime_operation_phase::admitted:
		return "admitted-not-confirmed";
	case player_death_restitution_runtime_operation_phase::durable_execution:
		return "durable-execution";
	case player_death_restitution_runtime_operation_phase::journal_uncertain:
		return "journal-uncertain";
	case player_death_restitution_runtime_operation_phase::durable_receipt_unverified:
		return "durable-receipt-unverified";
	case player_death_restitution_runtime_operation_phase::terminal_failure:
		return "terminal-failure";
	case player_death_restitution_runtime_operation_phase::unknown:
	default:
		return "unknown";
	}
}

const char *durability_name(critical_command_durability durability)
{
	switch (durability)
	{
	case critical_command_durability::awaiting_durability:
		return "awaiting";
	case critical_command_durability::durable:
		return "durable";
	case critical_command_durability::uncertain:
		return "uncertain";
	case critical_command_durability::failed:
		return "failed";
	case critical_command_durability::unknown:
	default:
		return "unknown";
	}
}

void send_operation_status(P_char ch, const char *operation_text)
{
	critical_operation_id operation_id = {};
	if (!operation_text || !critical_operation_id_from_hex(operation_text, &operation_id))
	{
		send_to_char(
			"phase=status; refused: operation identity is malformed; next: use the exact "
			"operation identity from commit; retry: safe after correction, without resubmitting.\r\n",
			ch);
		return;
	}
	player_death_restitution_runtime_operation_status status = {};
	if (!player_death_restitution_runtime_operation_status_copy(GET_NAME(ch), GET_LEVEL(ch),
								    operation_id, &status))
	{
		send_to_char(
			"phase=status; operation status is unavailable for this staff identity or runtime "
			"retention window; no delivery claim is made; next: use the protected read-only "
			"verify/reconciliation path from the same plan; retry: do not resubmit this operation.\r\n",
			ch);
		return;
	}
	char operation[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
	if (!critical_operation_id_to_hex(status.operation_id, operation, sizeof(operation)))
	{
		send_to_char(
			"phase=status; operation identity could not be rendered; no delivery claim is made.\r\n",
			ch);
		return;
	}
	const char *next = "restitution status again; do not submit a duplicate";
	const char *retry = "do not retry this operation";
	if (status.phase == player_death_restitution_runtime_operation_phase::journal_uncertain)
		next = "use the reconciliation/recovery path, then status again";
	else if (status.phase ==
		 player_death_restitution_runtime_operation_phase::durable_receipt_unverified)
		next = "run protected exact read-only verify from the same plan";
	else if (status.phase == player_death_restitution_runtime_operation_phase::terminal_failure)
		next = "preserve the failure and rebuild evidence/plan only after reconciliation";
	char message[MAX_STRING_LENGTH] = {};
	std::snprintf(message, sizeof(message),
		      "phase=status; operation=%s; state=%s; durability=%s; fence=%s; "
		      "durable_receipt=%s; delivery=not_verified; exact_verification=required; "
		      "next: %s; retry: %s.\r\n",
		      operation, phase_name(status.phase), durability_name(status.durability),
		      status.target_save_login_fence_held ? "held" : "released",
		      status.durable_receipt_recorded ? "recorded" : "not_confirmed", next, retry);
	send_to_char(message, ch);
}
} // namespace

ACMD(do_restitution)
{
	if (!ch)
		return;
	if (IS_NPC(ch) || GET_LEVEL(ch) < FORGER)
	{
		send_to_char(
			"phase=authorization; refused: staff authorization is required; no operation was admitted.\r\n",
			ch);
		return;
	}

	char subcommand[MAX_INPUT_LENGTH] = {};
	char canonical_hex[MAX_INPUT_LENGTH] = {};
	char *remaining = one_argument(argument, subcommand);
	if (!remaining || !*subcommand)
	{
		send_to_char(RESTITUTION_GUIDE, ch);
		return;
	}

	if (!str_cmp(subcommand, "help") || !str_cmp(subcommand, "guide"))
	{
		if (!only_spaces(remaining))
		{
			send_to_char(RESTITUTION_SYNTAX, ch);
			return;
		}
		send_to_char(RESTITUTION_GUIDE, ch);
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
			send_staging_progress(ch, "Restitution staging started.", false);
		else
		{
			send_restitution_result(ch, result, {});
			if (result == player_death_restitution_runtime_result::duplicate_staging)
				send_staging_progress(ch, "Existing restitution staging retained.",
						      false);
		}
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
			send_staging_progress(ch, "Restitution chunk accepted.", false);
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
		send_to_char(
			"phase=canonical-validation; validating the complete staged payload before any "
			"recipient fence or coordinator submission; delivery=not complete.\r\n",
			ch);
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
		player_death_restitution_staff_staging_status before = {};
		const bool had_staging =
			player_death_restitution_staff_get_staging_status(GET_NAME(ch),
									  GET_LEVEL(ch), &before) &&
			before.state == player_death_restitution_staff_staging_state::active;
		const auto result =
			player_death_restitution_staff_abort(GET_NAME(ch), GET_LEVEL(ch));
		if (result == player_death_restitution_runtime_result::accepted)
			send_to_char(
				had_staging ?
					"phase=staging; aborted: staged data was discarded; no restitution was submitted; "
					"next: restart with inspect -> plan -> export -> restitution begin; retry: safe.\r\n" :
					"phase=staging; abort completed: no staged data existed and no restitution was "
					"submitted; next: restitution begin; retry: safe.\r\n",
				ch);
		else
			send_restitution_result(ch, result, {});
		return;
	}

	if (!str_cmp(subcommand, "status"))
	{
		remaining = one_argument(remaining, canonical_hex);
		if (!remaining || !*canonical_hex || !only_spaces(remaining))
		{
			send_to_char(RESTITUTION_SYNTAX, ch);
			return;
		}
		send_operation_status(ch, canonical_hex);
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
