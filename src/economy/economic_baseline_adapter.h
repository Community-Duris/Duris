#ifndef DURIS_ECONOMIC_BASELINE_ADAPTER_H
#define DURIS_ECONOMIC_BASELINE_ADAPTER_H
#include "economy/economic_accounting_plan.h"

constexpr uint32_t ECONOMIC_WRITER_BASELINE = 4;
constexpr uint32_t ECONOMIC_BASELINE_OPERATION_DOMAIN = 0x42415345;
constexpr size_t ECONOMIC_BASELINE_MAX_HOLDINGS = ECONOMIC_ACCOUNTING_MAX_ACCOUNTS - 1;

struct economic_baseline_holding
{
	economic_account_key account;
	economic_coin_vector balance = {};
	uint64_t native_revision = 0;
	// Fingerprint of the exact source identity and its retained contents.
	economic_digest source_digest = {};
};
struct economic_baseline_item
{
	economic_item_snapshot snapshot;
	economic_digest source_digest = {};
};
struct economic_baseline_batch
{
	critical_operation_id lineage = {}, epoch = {}, preparation_id = {};
	uint64_t actor_id = 0, batch_index = 0;
	economic_account_key opening_account;
	// Bind the independently established frozen boundary and complete domain
	// coverage manifest. Nonzero hashes alone do NOT prove either assertion.
	economic_digest boundary_digest = {}, coverage_digest = {};
	std::vector<economic_baseline_holding> holdings;
	// Complete forests, including unchanged ancestors and quarantine/history.
	std::vector<economic_baseline_item> items;
};
class economic_prepared_baseline
{
    public:
	const economic_accounting_plan &plan() const { return plan_; }
	const economic_baseline_batch &witness() const { return witness_; }
	const std::vector<uint8_t> &encoded_plan() const { return encoded_; }

    private:
	economic_baseline_batch witness_;
	economic_accounting_plan plan_;
	std::vector<uint8_t> encoded_;
	economic_prepared_baseline(economic_baseline_batch, economic_accounting_plan,
				   std::vector<uint8_t>);
	friend economic_accounting_error
	economic_baseline_prepare(const economic_baseline_batch &,
				  std::optional<economic_prepared_baseline> *);
};
// Pure genesis preparation, not a native mutation or authority capability.
// Opening effects use accounting revision 0->1; native revisions (including 0
// and UINT64_MAX) remain unchanged in the canonical witness/domain digest.
// Every nonzero ordinary holding has its own exact opposite opening-equity leg;
// zero holdings remain explicit without fabricating zero-value postings.
// No item creation/transfer occurs: before and after snapshots are identical.
// The future cutover owner must prove quiescence, coverage, mapping uniqueness,
// no prior opening for this account/epoch, and atomically retain the witness,
// plan and preparation receipt before activation. Never apply this plan using
// a gameplay currency writer. Output is unchanged on every failure.
economic_accounting_error economic_baseline_prepare(const economic_baseline_batch &,
						    std::optional<economic_prepared_baseline> *);
// Canonical durable witness encoding. A maximum witness exceeds the critical
// command payload bound: persist it beside the original plan/receipt under the
// cutover owner, never force it into an ordinary command or truncate a forest.
constexpr size_t ECONOMIC_BASELINE_HEADER_BYTES = 192;
constexpr size_t ECONOMIC_BASELINE_HOLDING_BYTES = 112;
constexpr size_t ECONOMIC_BASELINE_ITEM_BYTES = 88;
constexpr size_t ECONOMIC_BASELINE_MAX_BYTES =
	ECONOMIC_BASELINE_HEADER_BYTES +
	ECONOMIC_BASELINE_MAX_HOLDINGS * ECONOMIC_BASELINE_HOLDING_BYTES +
	ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES * ECONOMIC_BASELINE_ITEM_BYTES;
// Encode only an already prepared witness; decoding validates all bounds,
// canonical ordering and reserved bytes, then regenerates the immutable plan.
// Neither function authenticates source fingerprints or authorizes cutover.
// All output arguments remain unchanged on failure.
economic_accounting_error economic_baseline_encode(const economic_prepared_baseline &,
						   std::vector<uint8_t> *);
economic_accounting_error economic_baseline_decode(std::span<const uint8_t>,
						   std::optional<economic_prepared_baseline> *);
#endif
