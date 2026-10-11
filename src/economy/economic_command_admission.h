#ifndef DURIS_ECONOMIC_COMMAND_ADMISSION_H
#define DURIS_ECONOMIC_COMMAND_ADMISSION_H

#include "persistence/critical_command.h"

// Stateless schema-2 route validation for admission and durable replay. SQL
// supports bank, coin, item and singleton collector purchase roots; flatfile
// supports bank and item roots. Other collector actions remain unregistered.
// Repository owners still enforce source entitlement and current authority.
bool economic_command_admission_supported(const critical_command &) noexcept;
// Mirrors the central SHOP route registration. There is no independent toggle;
// producers must not prepare native checkpoints for an unregistered route.
bool economic_shop_trade_admission_available() noexcept;
bool economic_flatfile_command_admission_supported(const critical_command &) noexcept;

// Complete ROOM-only projections of the selected original callback. SQL uses
// the genuine full bounded command decoder; flat preserves the original
// bank/item allowlist and refuses type22. Every other command is outside this
// provider's scope. These pure proofs grant no source, execution or activation.
// Caller retains complete command/current globals/callback context in outer.
bool economic_room_command_admission_supported_bounded(const critical_command &,
						       bool (*reserve)(size_t, void *) noexcept,
						       void *context, size_t outer_live) noexcept;
bool economic_flatfile_room_command_admission_supported_bounded(
	const critical_command &, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept;

// Complete original NMB4 ordinary immutable admission projection only. The
// actual selected backend allowlist remains authoritative; this proof does not
// register a route, grant a source, construct a mobile or confer activation.
// Caller owns command and prior capacities; the full prospective decoder owns
// its actual image/recipe/role and canonical temporary allocations.
bool economic_ordinary_native_command_admission_supported_bounded(
	const critical_command &, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept;

#include "economy/economic_currency_adapter.h"

// Complete additive projection of the original immutable account-bank branch.
// SQL and flat preserve their original allowlists and all intent byte checks.
// This never selects today's projection, grants a source, submits a command,
// changes activation, or retags retained evidence. Originals remain unchanged.
// Caller retains the authentic complete command/capacities and enclosing owners
// in outer_live. Reserve synchronously admits the full absolute peak under that
// owner's existing limit; it must not mutate inputs or reenter this provider.
// Every refusal preserves command; reserve keeps its owner bookkeeping. Unsupported selected
// library profiles fail closed. Native/library implementation qualification is
// separate from these prospective declaration/storage contracts.
bool economic_bank_command_admission_supported_bounded(const critical_command &,
						       bool (*)(size_t, void *) noexcept, void *,
						       size_t) noexcept;
bool economic_flatfile_bank_command_admission_supported_bounded(const critical_command &,
								bool (*)(size_t, void *) noexcept,
								void *, size_t) noexcept;

// Pure strong-output profiles: query first, then Source+INITIAL transiently
// before genuine entry. The provider self-admits its own Source and retains its
// unchanged children's real uncovered supplements; its supplement is zero.
bool economic_bank_command_admission_source_frame_bytes(size_t *) noexcept;
bool economic_bank_command_admission_source_supplement_frame_bytes(size_t *) noexcept;
bool economic_bank_command_admission_initial_inline_bytes(size_t *) noexcept;

constexpr size_t economic_bank_command_admission_source_query_frame_bytes() noexcept
{
	// Real full getter output/result, five named size_t locals, the selected
	// checked-add declaration (P+N+B), and four genuine dependency queries.
	// INITIAL/supp queries are smaller P+B; this accessor is constant evaluation.
	return economic_intent_decode_source_query_frame_bytes() +
	       currency_command_decode_payload_source_query_frame_bytes() +
	       currency_command_build_source_query_frame_bytes() +
	       economic_currency_intent_source_query_frame_bytes() + 2 * sizeof(void *) +
	       6 * sizeof(size_t) + 2 * sizeof(bool);
}

#endif
