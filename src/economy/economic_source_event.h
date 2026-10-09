#ifndef DURIS_ECONOMIC_SOURCE_EVENT_H
#define DURIS_ECONOMIC_SOURCE_EVENT_H

#include "persistence/critical_command.h"

#include <cstddef>
#include <cstdint>
#include <array>
#include <span>

enum class economic_accounting_error : uint8_t;

enum class economic_source_kind : uint16_t
{
	quest_completion = 1,
	npc_generation = 2,
	starter_grant = 3,
	boon = 4,
	achievement = 5,
	gambling_round = 6,
	world_generation = 7,
	crafting = 8,
	administrator = 9,
	baseline = 10,
	legacy_import = 11,
	shop_stock = 12,
	auction = 13,
	corpse = 14,
	correction = 15,
	lifecycle = 16,
	service = 17,
	item_action = 18,
	quest_action = 19,
	spell_creation = 20,
	spell_consumption = 21,
	intentional_destruction = 22,
	loot = 23,
};

constexpr size_t ECONOMIC_SOURCE_EVENT_BYTES = 48;
struct economic_source_event
{
	economic_source_kind kind = {};
	critical_operation_id source = {};
	critical_operation_id generation = {};
	uint64_t sequence = 0;
	uint32_t slot = 0;
};

// Pure original source-event value codecs. No allocation, source issuance or
// current authority follows from a decoded source/generation identity.
bool economic_source_event_valid(const economic_source_event &event);
// Named decoder objects, including nested returned identity/span objects when
// optional NRVO is absent. Scalar call frames and caller output are excluded.
size_t economic_source_event_decode_object_bytes() noexcept;
economic_accounting_error
economic_source_event_encode(const economic_source_event &event,
			     std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> *encoded);
economic_accounting_error economic_source_event_decode(std::span<const uint8_t> encoded,
						       economic_source_event *event);

#endif
