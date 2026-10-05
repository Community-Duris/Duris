#ifndef SHOP_TRADE_RECOVERY_MANIFEST_H
#define SHOP_TRADE_RECOVERY_MANIFEST_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

constexpr uint16_t SHOP_TRADE_RECOVERY_MANIFEST_VERSION = 8;
constexpr size_t SHOP_TRADE_RECOVERY_MAX_UIDS = 4096;
constexpr size_t SHOP_TRADE_RECOVERY_FOREST_COUNT = 6;
constexpr size_t SHOP_TRADE_RECOVERY_MANIFEST_HEADER_BYTES = 8;
constexpr size_t SHOP_TRADE_RECOVERY_FOREST_HEADER_BYTES = 40;
constexpr size_t SHOP_TRADE_RECOVERY_MANIFEST_MIN_BYTES =
	SHOP_TRADE_RECOVERY_MANIFEST_HEADER_BYTES +
	SHOP_TRADE_RECOVERY_FOREST_COUNT * SHOP_TRADE_RECOVERY_FOREST_HEADER_BYTES;
constexpr size_t SHOP_TRADE_RECOVERY_MANIFEST_MAX_BYTES =
	SHOP_TRADE_RECOVERY_MANIFEST_MIN_BYTES +
	SHOP_TRADE_RECOVERY_FOREST_COUNT * SHOP_TRADE_RECOVERY_MAX_UIDS * sizeof(uint64_t);

enum class shop_trade_recovery_forest_role : uint8_t
{
	unknown = 0,
	player_before = 1,
	player_after = 2,
	keeper_before = 3,
	keeper_after = 4,
	live_target_before = 5,
	live_target_after = 6,
};

// Value bindings only. A digest is not custody, admission, publication or ACK authority.
struct shop_trade_recovery_forest_binding
{
	bool present = false;
	uint32_t canonical_bytes = 0;
	std::array<uint8_t, 32> canonical_digest{};
	std::vector<uint64_t> ordered_item_uids;
	bool operator==(const shop_trade_recovery_forest_binding &) const = default;
};

struct shop_trade_recovery_manifest
{
	shop_trade_recovery_forest_binding player_before;
	shop_trade_recovery_forest_binding player_after;
	shop_trade_recovery_forest_binding keeper_before;
	shop_trade_recovery_forest_binding keeper_after;
	shop_trade_recovery_forest_binding live_target_before;
	shop_trade_recovery_forest_binding live_target_after;
	bool operator==(const shop_trade_recovery_manifest &) const = default;
};

bool shop_trade_recovery_forest_freeze(std::span<const uint8_t> canonical_bytes,
				       shop_trade_recovery_forest_role role,
				       shop_trade_recovery_forest_binding *out);
bool shop_trade_recovery_forest_verify(std::span<const uint8_t> canonical_bytes,
				       shop_trade_recovery_forest_role role,
				       const shop_trade_recovery_forest_binding &binding);
bool shop_trade_recovery_manifest_is_empty(const shop_trade_recovery_manifest &) noexcept;
bool shop_trade_recovery_manifest_shape_valid(const shop_trade_recovery_manifest &) noexcept;
bool shop_trade_recovery_manifest_encode(const shop_trade_recovery_manifest &,
					 std::vector<uint8_t> *out);
bool shop_trade_recovery_manifest_decode(std::span<const uint8_t>,
					 shop_trade_recovery_manifest *out);

#endif
