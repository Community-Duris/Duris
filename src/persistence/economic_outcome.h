#ifndef DURIS_ECONOMIC_OUTCOME_H
#define DURIS_ECONOMIC_OUTCOME_H

#include "economy/economic_accounting_plan.h"
#include "persistence/critical_command.h"
#include "item/item_transfer_command.h"
#include <span>
#include <vector>

// Versioned owned values. These bytes do not grant gameplay authority. The
// original domain caller must validate actual RAM identity and custody.
enum class economic_outcome_domain : uint16_t
{
	currency = 1,
	item_transfer,
	player_consumable,
	world_birth,
	shop_trade,
	remove_curse,
	death,
	resurrection,
	quest_reward,
	escrow,
};

enum class economic_outcome_presence : uint8_t
{
	absent = 0,
	present = 1,
	retired = 2
};

// Codec version belongs to the domain adapter. Before/after are complete
// recoverable domain images, never pointers, derived caches or permission.
// Presence is explicit: an existing commissioned base at cut zero is not a
// birth. Native cut versions are derived from the published base/tail, not
// projected SQL item/owner revisions. A birth has absent before and empty image.
// A retirement
// explicitly carries retired plus its new revision, not mere SQL absence.
struct economic_outcome_effect
{
	critical_entity_key key{};
	item_owner_identity owner_before{};
	item_owner_identity owner_after{};
	uint16_t codec_version = 1;
	uint64_t before_revision = 0;
	uint64_t after_revision = 0;
	economic_outcome_presence before_presence = economic_outcome_presence::present;
	economic_outcome_presence after_presence = economic_outcome_presence::present;
	std::vector<uint8_t> before;
	std::vector<uint8_t> after;
};

struct economic_outcome_record
{
	uint16_t version = 1;
	// Assigned by the sole ordered writer before encoding or publication.
	// Contiguous committed sequence is separate from native phase revisions.
	uint64_t sequence = 0;
	uint64_t logical_tick = 0;
	economic_outcome_domain domain{};
	critical_command command{};
	economic_accounting_plan accounting;
	uint16_t result_version = 1;
	std::vector<uint8_t> result;
	std::vector<economic_outcome_effect> effects;
};

enum class economic_outcome_status : uint8_t
{
	durable = 1,
	rejected,
	uncertain,
	conflict,
	unavailable,
};

struct economic_outcome_result
{
	economic_outcome_status status = economic_outcome_status::unavailable;
	uint64_t sequence = 0;
	unsigned int error = 0;
};

// Callbacks bind the real original domain owner. Prepare record, all native
// storage, timers and output before commit. No event dispatch/yield occurs
// between current validation, synchronous append/fsync and native publication.
// Publication is noexcept and must use preallocated storage. A false result
// after durability fences recovery; it must never issue a positive ACK.
struct economic_outcome_publication
{
	bool (*current)(void *) noexcept = nullptr;
	bool (*publish)(const economic_outcome_record &, void *) noexcept = nullptr;
	void (*fence)(uint64_t, void *) noexcept = nullptr;
	void *context = nullptr;
};

// ROOT owns codec and the synchronous journal/commit implementations.
// Strong outputs on rejection; exact body reuse only, no RNG recomputation.
bool economic_outcome_encode(const economic_outcome_record &, std::vector<uint8_t> *) noexcept;
bool economic_outcome_decode(std::span<const uint8_t>, economic_outcome_record *) noexcept;
// durable here means BOTH authoritative durable record and exact once RAM
// publication. journal_append(durable) alone may never authorize client ACK.
economic_outcome_result economic_outcome_commit(economic_outcome_record &,
						const economic_outcome_publication &) noexcept;

// Journal APIs are internal composition seams, not SQL/gameplay permission.
// An exact durable replay returns its original sequence; different bytes for
// an existing operation ID conflict. Unknown append retains the attempted
// bytes and fences new economic admission until exact reconciliation.
economic_outcome_result economic_outcome_journal_append(const economic_outcome_record &) noexcept;
// Caller uses an owned development/runtime namespace and explicit byte/record
// limits; inherit original configured journal bounds, never guessed prices.
bool economic_outcome_journal_init(const char *directory, size_t byte_limit,
				   size_t record_limit) noexcept;
bool economic_outcome_journal_init_namespace(const char *directory, size_t byte_limit,
					     size_t record_limit,
					     const critical_operation_id &lineage,
					     const critical_operation_id &epoch,
					     uint64_t durable_base_sequence) noexcept;
bool economic_outcome_journal_namespace(critical_operation_id *, critical_operation_id *) noexcept;
bool economic_outcome_journal_lookup(const critical_operation_id &,
				     economic_outcome_record *) noexcept;
void economic_outcome_journal_shutdown() noexcept;
using economic_outcome_replay_fn = bool (*)(const economic_outcome_record &, void *) noexcept;
bool economic_outcome_journal_replay(uint64_t durable_base_sequence, economic_outcome_replay_fn,
				     void *) noexcept;
bool economic_outcome_journal_reconcile() noexcept;
uint64_t economic_outcome_journal_next_sequence() noexcept;

struct economic_outcome_journal_health
{
	bool initialized = false;
	bool uncertain = false;
	uint64_t compacted_through = 0;
	uint64_t durable_sequence = 0;
	size_t retained_bytes = 0;
	size_t retained_records = 0;
	size_t byte_limit = 0;
	size_t record_limit = 0;
};
economic_outcome_journal_health economic_outcome_journal_health_copy() noexcept;
// Exact coupled base verification is supplied by the real projector/recovery
// owner while stopped at this ordered boundary. An integer cursor is not proof.
using economic_outcome_base_verify_fn = bool (*)(uint64_t, void *) noexcept;
bool economic_outcome_journal_retire_prefix(uint64_t, economic_outcome_base_verify_fn,
					    void *) noexcept;
// Genuine world-thread capture cut after publication, not journal.next_sequence.
// Refuse if an affected uncertain/unpublished outcome can invalidate the image.
bool economic_outcome_capture_cut(uint64_t *) noexcept;
// Read the retained global cut only inside the world's genuine current check.
// This is an observation clock, not permission to capture a new save or publish.
bool economic_outcome_current_observation_cut(uint64_t *) noexcept;
// Match retained clock facts while idle or in the genuine preappend current
// check. Never grants recovery admission or permits a new save during commit.
bool economic_outcome_clock_matches(const critical_operation_id &lineage,
				    const critical_operation_id &epoch,
				    uint64_t published_cut) noexcept;
bool economic_outcome_entity_cut(const critical_entity_key &, uint64_t *) noexcept;
bool economic_outcome_install_recovered_cut(uint64_t, economic_outcome_base_verify_fn,
					    void *) noexcept;

// Same ordered result, projected in one MySQL transaction. Cursor/archive and
// all required coupled domain effects/receipt advance together. Projection
// never recomputes outcome or uses SQL ownership as live RAM permission.
struct st_mysql;
economic_outcome_result economic_outcome_mysql_project(struct st_mysql *,
						       const economic_outcome_record &) noexcept;

struct economic_outcome_recovery_state
{
	uint64_t durable_base_sequence = 0;
	uint64_t recovered_sequence = 0;
	uint64_t projected_sequence = 0;
	bool ready = false;
	bool uncertain = false;
};

// Plan5 owns startup base/tail/current-readiness; admission stays closed until
// the actual coupled state is recovered. Cursor alone never proves a base.
bool economic_outcome_recover(economic_outcome_recovery_state *) noexcept;
bool economic_outcome_admit(const economic_outcome_recovery_state &) noexcept;
// Actual recovery owner state, not a caller supplied ready flag.
bool economic_outcome_recovery_admission_allowed() noexcept;

#endif
