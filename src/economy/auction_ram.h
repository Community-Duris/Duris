#ifndef DURIS_AUCTION_RAM_H
#define DURIS_AUCTION_RAM_H

#include "economy/auction_command.h"
#include "persistence/critical_command_coordinator.h"

#include <string>
#include <vector>

struct auction_ram_item
{
	uint64_t uid = 0;
	uint64_t revision = 0;
	int32_t vnum = 0;
	uint32_t claimant = 0;
	bool claimed = false;
};

struct auction_ram_listing
{
	uint32_t id = 0;
	uint32_t seller = 0;
	uint32_t winner = 0;
	uint32_t status = 0;
	int64_t price = 0;
	int64_t buy_price = 0;
	uint64_t revision = 0;
	uint64_t end_time = 0;
	std::string seller_account;
	std::vector<auction_ram_item> items;
	std::vector<uint8_t> object_blob{};
};

struct auction_ram_pickup
{
	int64_t amount = 0;
	uint64_t revision = 0;
};

// All inputs are RAM snapshots, including money claim and item custody clocks.
// The function has no storage, clock, or gameplay side effects.
unsigned int auction_ram_prepare(const critical_command &command,
				 const auction_ram_listing *current,
				 const currency_command_result &balances,
				 const auction_ram_pickup &pickup, uint64_t player_owner_revision,
				 uint64_t auction_owner_revision, auction_ram_listing *next,
				 auction_command_result *result);

bool auction_ram_seed(const auction_ram_listing &listing);
bool auction_ram_seed_pickup(uint32_t pid, const auction_ram_pickup &pickup);
void auction_ram_set_catalog_revision(uint64_t revision, bool flatfile);
void auction_ram_set_policy(uint32_t closing_fee_basis_points, uint32_t bid_extension_seconds);
bool auction_ram_find(uint32_t id, auction_ram_listing *listing);
bool auction_ram_find_pickup(uint32_t pid, auction_ram_pickup *pickup);
size_t auction_ram_expired(uint64_t now, uint32_t *ids, size_t capacity) noexcept;
// Fill the next claim from the authoritative catalog, preserving actor identity.
// Returns zero when a claim exists, ENOENT when nothing is staged.
unsigned int auction_ram_next_claim(uint32_t pid, auction_command_payload *payload);
critical_apply_result auction_ram_prepare_command(const critical_command &command);
// Invoked immediately after durable admission, before completion delivery. All
// nodes and vectors were allocated during prepare; commit cannot allocate.
bool auction_ram_can_publish(const critical_command &command) noexcept;
bool auction_ram_publish(const critical_command &command) noexcept;
void auction_ram_reset();

#endif
