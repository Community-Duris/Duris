#ifndef ZONE_RESET_ITEM_ORIGIN_H
#define ZONE_RESET_ITEM_ORIGIN_H

#include "economy/zone_reset_item_command.h"
#include "item/item_transfer_command.h"

#include <array>
#include <span>
#include <vector>

constexpr uint16_t ZONE_RESET_ITEM_ORIGIN_VERSION = 1;
constexpr size_t ZONE_RESET_ITEM_ORIGIN_HEADER_BYTES = 16;
constexpr size_t ZONE_RESET_ITEM_ORIGIN_MAX_BYTES = ZONE_RESET_ITEM_ORIGIN_HEADER_BYTES +
						    CRITICAL_COMMAND_MAX_ENCODED_BYTES +
						    ITEM_TRANSFER_RESULT_BYTES;

struct zone_reset_item_retained_origin
{
	// Observation only; neither persisted nor a publication permission.
	bool present = false;
	critical_command original{};
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> result{};
};

// Preserve every original command byte, including accepted time, source,
// season, literals and factory recipes, together with its canonical typed48.
// Pure correlation is not proof of a committed root or physical publication.
// No mutation/admission/SQL/ACK authority; every refusal preserves output.
economic_accounting_error zone_reset_item_origin_encode(const critical_command &,
							std::span<const uint8_t>,
							std::vector<uint8_t> *) noexcept;
economic_accounting_error zone_reset_item_origin_decode(std::span<const uint8_t>,
							zone_reset_item_retained_origin *) noexcept;

// Complete original correlation and canonical wire checks with prospective
// storage admission. Caller owns wire/command/result spans, prior output and
// context in outer_live. Hold peaks through output transfer; no authority follows.
economic_accounting_error
zone_reset_item_origin_encode_bounded(const critical_command &, const std::span<const uint8_t> &,
				      std::vector<uint8_t> *,
				      bool (*reserve_scratch_peak)(size_t, void *) noexcept,
				      void *context, size_t outer_live) noexcept;
economic_accounting_error zone_reset_item_origin_decode_bounded(
	const std::span<const uint8_t> &, zone_reset_item_retained_origin *,
	bool (*reserve_scratch_peak)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_origin_heap_bytes = nullptr) noexcept;

#endif
