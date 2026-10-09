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

#endif
