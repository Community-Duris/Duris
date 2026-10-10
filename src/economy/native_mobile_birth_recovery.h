#ifndef NATIVE_MOBILE_BIRTH_RECOVERY_H
#define NATIVE_MOBILE_BIRTH_RECOVERY_H

#include "economy/native_mobile_birth_command.h"
#include "persistence/critical_command_completion.h"
#include "persistence/critical_command_journal.h"

#include <array>
#include <span>
#include <vector>

// Actual original once-only action observations. Started/unreturned is retained
// uncertainty, never permission to rerun. Returned failure cannot become success.
struct native_mobile_birth_recovery_action
{
	bool started = false, returned = false, succeeded = false;
	bool operator==(const native_mobile_birth_recovery_action &) const = default;
};
struct native_mobile_birth_recovery_effect
{
	bool started = false, returned = false, succeeded = false, periodic = false;
	bool operator==(const native_mobile_birth_recovery_effect &) const = default;
};
struct native_mobile_birth_recovery_mobile
{
	// Native ownership is consumed before the original room/special callbacks.
	// Consumed does not imply those callbacks returned, or permit their replay.
	bool started = false, returned = false, consumed = false;
	bool operator==(const native_mobile_birth_recovery_mobile &) const = default;
};
enum class native_mobile_birth_recovery_mobile_step : uint8_t
{
	ownership = 0,
	room = 1,
	mundane = 2,
	special_probe = 3,
	special_event = 4,
	patrol = 5,
	skin = 6,
	maintenance = 7,
};
enum class native_mobile_birth_recovery_mobile_choice : uint8_t
{
	mundane = 0,
	special_event = 1,
	patrol = 2,
	skin = 3,
};
struct native_mobile_birth_recovery_choice
{
	bool chosen = false, requested = false;
	int32_t delay = 0;
	bool operator==(const native_mobile_birth_recovery_choice &) const = default;
};
struct native_mobile_birth_recovery_item
{
	uint64_t object_uid = 0;
	uint32_t next_step = 0;
	bool current_step_started = false, admitted = false, published = false;
	// succeeded means the original detached item ownership was published.
	native_mobile_birth_recovery_action publication;
	// succeeded means the original local stock enrollment really returned success.
	native_mobile_birth_recovery_action enrollment;
	// Exact original count/order: 2*libraries+3, from the immutable recipe.
	std::vector<native_mobile_birth_recovery_effect> effects;
	bool operator==(const native_mobile_birth_recovery_item &) const = default;
};
enum class native_mobile_birth_recovery_stage : uint8_t
{
	captured = 0,
	publishing = 1,
	physically_proven = 2,
};
struct native_mobile_birth_recovery_context
{
	bool receipt_present = false;
	// All genuine completion fields survive, including delivery metadata. This
	// value does not authenticate receipt delivery or current SQL/world proof.
	critical_completion receipt{};
	native_mobile_birth_recovery_stage stage = native_mobile_birth_recovery_stage::captured;
	// succeeded means committed/installed respectively.
	native_mobile_birth_recovery_action whole_binding, reference_install;
	native_mobile_birth_recovery_mobile mobile_publication;
	// Original fixed publication order. Only special_probe's periodic flag may
	// hold its actual returned callback decision; all other periodic flags are zero.
	std::array<native_mobile_birth_recovery_effect, 8> mobile_effects{};
	// Retain the actual chosen scheduling decisions/delays BEFORE schedule intent.
	// Skip decisions are chosen=true/requested=false/delay=0, then no-op success.
	std::array<native_mobile_birth_recovery_choice, 4> mobile_choices{};
	// Actual original stock construction/publication order, which may differ
	// from the final image's equipment/carry forest order. UID binds each recipe.
	// Every successor preserves this complete unique UID order exactly.
	std::vector<native_mobile_birth_recovery_item> items;
	// Original successful cache projection observation only. Cold owners must
	// prove and rebuild the current projection; this flag grants no bypass.
	bool runtime_applied = false;
};

constexpr uint16_t NATIVE_MOBILE_BIRTH_RECOVERY_VERSION = 1;

// Complete canonical attachment bound to every byte of the original v2 birth
// command, including its final literal image and constructor recipe. No pointer,
// process generation, later native image or new NPC-stat ledger is persisted.
// The existing native recovery journal owns persistence. These pure functions
// grant no admission, SQL, cleanup, world publication or ACK capability.
// Strong output guarantee: every refusal leaves the output unchanged.
economic_accounting_error
native_mobile_birth_recovery_encode(const critical_command &,
				    const native_mobile_birth_recovery_context &,
				    std::vector<uint8_t> *) noexcept;
economic_accounting_error
native_mobile_birth_recovery_decode(const critical_command &, std::span<const uint8_t>,
				    native_mobile_birth_recovery_context *) noexcept;

// Read only the full original command actually retained inside a canonical
// recovery attachment, through the existing bounded recovery/command codecs.
// Requires actual payload v3 constructor inputs and the successful physically
// proven terminal BODY. Historical v2 remains readable through the original API.
// The attachment contains no envelope phase/revision: callers must separately
// validate those ACTUAL envelope fields and authenticate current lifetime storage.
// No unseen metadata, prototype reconstruction, admission, SQL/world or ACK
// authority follows. Every refusal leaves the output unchanged.
economic_accounting_error
native_mobile_birth_recovery_original_command_decode(std::span<const uint8_t>,
						     critical_command *) noexcept;

bool native_mobile_birth_recovery_valid(const critical_native_recovery_envelope &) noexcept;
bool native_mobile_birth_recovery_initial(const critical_native_recovery_envelope &) noexcept;
bool native_mobile_birth_recovery_successor(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept;
// Retained full receipt and CURRENT successful receipt must have the same
// immutable economic core. Replay may genuinely change applied/already_applied,
// attempts, times and diagnostic correlation. The shared private ACK owner must
// independently authenticate EVERY current field against coordinator delivery;
// this predicate neither synthesizes a receipt nor authenticates that authority.
bool native_mobile_birth_recovery_publication(const critical_native_recovery_envelope &,
					      const critical_completion &) noexcept;
bool native_mobile_birth_recovery_terminal(const critical_native_recovery_envelope &) noexcept;

// Distinct prospective ORDINARY NMB4 / MBR4 recovery family. Shared/unknown
// NBC4 roles refuse. Reuses the original NMR1 version1 framing, full original
// embedded command, action/choice/UID ordering and retained uncertainty rules.
// Successful receipt validation recompiles and binds the complete original
// NMB4 image/role/plan/owner/result. Historical APIs remain v2/v3 only.
// Extraction requires the successful physically proven BODY; terminal also
// requires the actual continuation_pending envelope and revision greater than1.
// No factory, admission, storage, world publication or ACK authority; every
// refusing codec preserves its output. Current lifetime proof remains separate.
economic_accounting_error
native_mobile_birth_cash_role_recovery_encode(const critical_command &,
					      const native_mobile_birth_recovery_context &,
					      std::vector<uint8_t> *) noexcept;
economic_accounting_error
native_mobile_birth_cash_role_recovery_decode(const critical_command &, std::span<const uint8_t>,
					      native_mobile_birth_recovery_context *) noexcept;
economic_accounting_error
native_mobile_birth_cash_role_recovery_original_command_decode(std::span<const uint8_t>,
							       critical_command *) noexcept;
bool native_mobile_birth_cash_role_recovery_valid(
	const critical_native_recovery_envelope &) noexcept;
bool native_mobile_birth_cash_role_recovery_initial(
	const critical_native_recovery_envelope &) noexcept;
bool native_mobile_birth_cash_role_recovery_successor(
	const critical_native_recovery_envelope &,
	const critical_native_recovery_envelope &) noexcept;
bool native_mobile_birth_cash_role_recovery_publication(const critical_native_recovery_envelope &,
							const critical_completion &) noexcept;
bool native_mobile_birth_cash_role_recovery_terminal(
	const critical_native_recovery_envelope &) noexcept;

// Shared original initial SHOP birth only. The existing journal envelope keeps
// command and complete canonical DURSHOPv2 checkpoint in one durable frame.
// Framing revision1 is not a whole-catalog clock or file mutation capability.
// Original time/roaming/affects survive byte-for-byte across every successor.
// Pure codecs grant no source, worker execution, SQL/publication or ACK authority.
struct native_mobile_birth_shared_shop_recovery_context
{
	native_mobile_birth_recovery_context progress;
	std::vector<uint8_t> original_checkpoint;
};
economic_accounting_error native_mobile_birth_shared_shop_recovery_encode(
	const critical_command &, const native_mobile_birth_shared_shop_recovery_context &,
	std::vector<uint8_t> *) noexcept;
economic_accounting_error native_mobile_birth_shared_shop_recovery_decode(
	const critical_command &, std::span<const uint8_t>,
	native_mobile_birth_shared_shop_recovery_context *) noexcept;
economic_accounting_error
native_mobile_birth_shared_shop_recovery_original_command_decode(std::span<const uint8_t>,
								 critical_command *) noexcept;
bool native_mobile_birth_shared_shop_recovery_valid(
	const critical_native_recovery_envelope &) noexcept;
bool native_mobile_birth_shared_shop_recovery_initial(
	const critical_native_recovery_envelope &) noexcept;
bool native_mobile_birth_shared_shop_recovery_successor(
	const critical_native_recovery_envelope &,
	const critical_native_recovery_envelope &) noexcept;
bool native_mobile_birth_shared_shop_recovery_publication(const critical_native_recovery_envelope &,
							  const critical_completion &) noexcept;
bool native_mobile_birth_shared_shop_recovery_terminal(
	const critical_native_recovery_envelope &) noexcept;

// Passive BODY/revision check for a zero-copy original executing-state view.
// The private worker separately authenticates phase, lifetime and execution.
bool native_mobile_birth_shared_shop_recovery_execution_valid(const critical_command &,
							      std::span<const uint8_t>,
							      uint64_t revision) noexcept;

// Genuine prospective ORDINARY/shared companions. The caller includes all
// already-live input capacities, inline spans/outputs and prior output heaps in
// outer_live, and retains each admitted absolute high-water allowance through
// transfer. Complete original canonical command/SHOP checkpoint/receipt compiler
// and result/progress/phase/revision predicates remain authoritative. No source,
// execution, publication or ACK authority. GCC13 libstdc++ C++11 ABI requests
// only; unsupported policy refuses. Codec outputs and optional transferred heap
// scalars remain unchanged on every refusal. Scalars exclude inline context.
// Bool validators report false on any refusal; callers may retain callback
// rejection independently when distinguishing resource and semantic refusal.
economic_accounting_error native_mobile_birth_cash_role_recovery_encode_bounded(
	const critical_command &, const native_mobile_birth_recovery_context &,
	std::vector<uint8_t> *, bool (*)(size_t, void *) noexcept, void *,
	size_t outer_live) noexcept;
economic_accounting_error native_mobile_birth_cash_role_recovery_decode_bounded(
	const critical_command &, const std::span<const uint8_t> &,
	native_mobile_birth_recovery_context *, bool (*)(size_t, void *) noexcept, void *,
	size_t outer_live, size_t *retained_context_heap_bytes = nullptr) noexcept;
bool native_mobile_birth_cash_role_recovery_valid_bounded(const critical_native_recovery_envelope &,
							  bool (*)(size_t, void *) noexcept, void *,
							  size_t outer_live) noexcept;
bool native_mobile_birth_cash_role_recovery_initial_bounded(
	const critical_native_recovery_envelope &, bool (*)(size_t, void *) noexcept, void *,
	size_t outer_live) noexcept;
bool native_mobile_birth_cash_role_recovery_terminal_bounded(
	const critical_native_recovery_envelope &, bool (*)(size_t, void *) noexcept, void *,
	size_t outer_live) noexcept;
economic_accounting_error native_mobile_birth_shared_shop_recovery_encode_bounded(
	const critical_command &, const native_mobile_birth_shared_shop_recovery_context &,
	std::vector<uint8_t> *, bool (*)(size_t, void *) noexcept, void *,
	size_t outer_live) noexcept;
economic_accounting_error native_mobile_birth_shared_shop_recovery_decode_bounded(
	const critical_command &, const std::span<const uint8_t> &,
	native_mobile_birth_shared_shop_recovery_context *, bool (*)(size_t, void *) noexcept,
	void *, size_t outer_live, size_t *retained_context_heap_bytes = nullptr) noexcept;
bool native_mobile_birth_shared_shop_recovery_valid_bounded(
	const critical_native_recovery_envelope &, bool (*)(size_t, void *) noexcept, void *,
	size_t outer_live) noexcept;
bool native_mobile_birth_shared_shop_recovery_initial_bounded(
	const critical_native_recovery_envelope &, bool (*)(size_t, void *) noexcept, void *,
	size_t outer_live) noexcept;
bool native_mobile_birth_shared_shop_recovery_terminal_bounded(
	const critical_native_recovery_envelope &, bool (*)(size_t, void *) noexcept, void *,
	size_t outer_live) noexcept;

// Complete original NMB4 shared-shop execution domain predicate. Nonzero
// revision, full canonical shared recovery decode, and original revision1
// no-receipt/no-progress proof; it does not impose a phase/terminal gate.
// Caller outer owns original command/attachment capacities and inline input
// span. Genuine nested decode owns/prospectively admits all temporary storage.
bool native_mobile_birth_shared_shop_recovery_execution_valid_bounded(
	const critical_command &, const std::span<const uint8_t> &, uint64_t revision,
	bool (*)(size_t, void *) noexcept, void *, size_t outer_live) noexcept;

// Historical INITIAL-only bounded validator for genuine v2/v3 commands.
// Requires revision1/execution_pending, complete canonical attachment and
// original recipe/UID/effect correlation, absent receipt and no progress.
// Every receipt refuses before canonical/image allocation. This does not add a
// general historical bounded decoder or successful-receipt result support.
// Caller outer owns the original envelope/command/attachment and prior heaps;
// the same prospective supported-policy contract above applies.
bool native_mobile_birth_recovery_initial_bounded(const critical_native_recovery_envelope &,
						  bool (*)(size_t, void *) noexcept, void *,
						  size_t outer_live) noexcept;

// Full passive ordinary context decode with resource/profile errors preserved
// through command, successful receipt, intent, compiler and result proof.
// Output context and retained heap scalar change only on complete success.
// Caller owns original input/output capacities; GCC13/CXX11 supported storage
// and prospective nested admission contract is identical to bounded decode.
economic_accounting_error native_mobile_birth_cash_role_recovery_decode_status_bounded(
	const critical_command &, const std::span<const uint8_t> &,
	native_mobile_birth_recovery_context *, bool (*)(size_t, void *) noexcept, void *,
	size_t outer_live, size_t *retained_context_heap_bytes = nullptr) noexcept;

// Complete original ordinary recovery validity with decoded error retained.
// Resource/profile refusal stays distinct from semantic invalidity; passive,
// no execution/phase advancement, mutation, acquisition or authority granted.
economic_accounting_error
native_mobile_birth_cash_role_recovery_validate_bounded(const critical_native_recovery_envelope &,
							bool (*)(size_t, void *) noexcept, void *,
							size_t outer_live) noexcept;

#endif
