#ifndef QUEST_MOBILE_NATIVE_BIRTH_H
#define QUEST_MOBILE_NATIVE_BIRTH_H

#include "core/structs.h"
#include <string>
#include "persistence/critical_command_coordinator.h"

// Passive replay and game-thread lifecycle entry points. Neither values nor
// these observations authorize a native birth, SQL mutation or publication ACK.
bool quest_mobile_native_birth_restore(const critical_command &) noexcept;
bool quest_mobile_native_birth_restore(const critical_native_recovery_envelope &) noexcept;
// Complete passive NMB4 shared SHOP restoration only. Original mixed dispatcher
// selects its real family; other birth versions retain their original routes.
// Full caller prefix owns inputs/inline outputs and CURRENT coordinator via its
// actual mutex-owning lender; the authentic budget owns existing birth registry
// retention separately. No native constructor, RNG, UID, SQL, world mutation,
// publication, retry, source admission or ACK follows from passive values.
bool quest_mobile_native_birth_restore_shared_shop_bounded(const critical_native_recovery_envelope &,
							   bool (*)(size_t, void *) noexcept,
							   void *, size_t outer_live) noexcept;

// Complete passive ordinary NMB4 registration from the genuine configured-root
// startup owner. Caller retains actual init-lock/lender/input prefix; its real
// observer owns existing registry storage once. No warm source pin, native
// reconstruction, UID/RNG, world publication, activation or ACK is issued here.
bool quest_mobile_native_birth_restore_ordinary_bounded(const critical_native_recovery_envelope &,
							const std::string &configured_recovery_root,
							bool (*)(size_t, void *) noexcept, void *,
							size_t outer_live) noexcept;

void quest_mobile_native_birth_replay_ready(bool) noexcept;
void quest_mobile_native_birth_completions(const critical_completion *, size_t) noexcept;
void quest_mobile_native_birth_pulse(bool prepare_original_resets) noexcept;
// Game-thread startup recovery only, after original replay registration and the
// existing active-authority/replay-ready gates. Reuses genuine original carrier,
// receipt/current SQL/world, constructor and once-only publication/retirement
// proof; never prepares fresh warm commands or runs deferred reset requests.
// True means only this registered original-journal birth work has drained. False
// also covers unavailable/refusing/error state. Neither value proves full startup
// readiness or restores SQL-only published origins. Inactive mode performs no work.
bool quest_mobile_native_birth_recovery_pulse() noexcept;
bool quest_mobile_native_birth_lifecycle_ready() noexcept;

struct native_mobile_birth_recovery_context;
struct native_mobile_birth_recovery_effect;
enum class economic_accounting_error : uint8_t;
struct flatfile_ordinary_native_birth_projection;
struct quest_mobile_native_image;
struct native_mobile_wallet_origin;
struct quest_mobile_native_constructor_recipe;
class quest_mobile_native_npc_flat_factory_scope;
struct quest_mobile_native_reference;
struct economic_source_event;
// RAM-only original dispatcher values. The private owning entry point below
// is the sole source of a retained frame; this DTO grants no admission authority.
struct quest_mobile_original_reset_locals
{
	int cmd_no = 0, last_cmd = 1, last_mob_load = 0, respawn = 0;
	int temp = 0, ival = 0, configured_shop = -1, replicated_shop = -1;
	P_char mob = nullptr, last_mob = nullptr, tmp_mob = nullptr, last_mob_followable = nullptr;
	P_obj obj = nullptr, obj_to = nullptr;
	arti_data artidata{};
	char buf[MAX_STRING_LENGTH]{};
	bool initialized = false, command_entered = false;
	struct original_p_progress
	{
		bool begun = false, eligible = false;
		// Actual non-mobile room-P route only. Original warm factory owns all
		// constructor/load/discard/nest phases; no lower effect bits are guessed.
		bool room_path = false, room_prepared = false;
		int room_result = 0;
		bool factory_started = false, factory_returned = false;
		bool artifact_started = false, artifact_returned = false, artifact_owned = false;
		bool target_returned = false, load_started = false, load_returned = false,
		     load_passed = false;
		bool discard_started = false, discard_returned = false, nest_started = false,
		     nest_returned = false, nest_succeeded = false;
		uint64_t object_uid = 0, target_uid = 0;
	} p;
	// Original opened O cut only. Factory progress remains in its original warm
	// owner; these retained values never authorize a constructor or placement.
	struct original_o_progress
	{
		bool begun = false, eligible = false;
		bool incumbent_returned = false, incumbent_take = false;
		P_obj incumbent = nullptr;
		uint64_t incumbent_uid = 0;
		bool root_returned = false;
		int root_result = 0;
		uint64_t object_uid = 0;
	} o;
};
class item_native_quest_publication_owner;
// Closed original producer/source capsule. Only the actual private factory
// owner can mint it while its real reset invocation and detached stage exist.
class quest_mobile_native_birth_ordinary_source_pin final
{
	friend class quest_mobile_native_birth_owner;
	friend class quest_mobile_native_birth_ordinary_execution_lease;
	struct implementation;
	std::unique_ptr<implementation> state_;
	explicit quest_mobile_native_birth_ordinary_source_pin(std::unique_ptr<implementation>);

    public:
	~quest_mobile_native_birth_ordinary_source_pin();
	quest_mobile_native_birth_ordinary_source_pin(
		const quest_mobile_native_birth_ordinary_source_pin &) = delete;
	quest_mobile_native_birth_ordinary_source_pin &
	operator=(const quest_mobile_native_birth_ordinary_source_pin &) = delete;
};

// Worker-stack request only. Constructor/request do not grant a world cut.
// The real game-thread owner validates the exact retained producer and holds
// its world interval until this noncopyable lease releases on the same worker.
class quest_mobile_native_birth_ordinary_execution_lease final
{
	friend class critical_ordinary_native_flat_execution_owner;
	friend class critical_shared_native_execution_dispatch;
	friend class quest_mobile_native_birth_owner;
	enum class phase : uint8_t
	{
		idle,
		requested,
		inspecting,
		granted,
		refused,
		released
	};
	quest_mobile_native_birth_ordinary_execution_lease() noexcept = default;
	~quest_mobile_native_birth_ordinary_execution_lease() noexcept;
	quest_mobile_native_birth_ordinary_execution_lease(
		const quest_mobile_native_birth_ordinary_execution_lease &) = delete;
	quest_mobile_native_birth_ordinary_execution_lease &
	operator=(const quest_mobile_native_birth_ordinary_execution_lease &) = delete;
	bool request(const critical_ordinary_native_execution_owner &) noexcept;
	bool current() const noexcept;
	const std::string *selected_root() const noexcept;
	static void cancel_pending() noexcept;
	static size_t fixed_storage_bytes() noexcept;
	quest_mobile_native_birth_ordinary_execution_lease *next_ = nullptr;
	const critical_ordinary_native_execution_owner *worker_ = nullptr;
	const quest_mobile_native_birth_ordinary_source_pin *source_ = nullptr;
	const void *producer_ = nullptr;
	uint64_t generation_ = 0;
	unsigned int attempt_ = 0;
	phase phase_ = phase::idle;
	bool game_holds_request_ = false;
};

// Pure CURRENT complete birth and warm ownership; includes warm_retained_size once.
// Genuine existing ROOT literal_pool_owned chooses the native/private partition.
bool quest_mobile_native_birth_retained_storage_bytes(size_t *) noexcept;
// Pure fixed named-source scratch; does not scan, register or grant authority.
bool quest_mobile_native_birth_current_source_frames(size_t *) noexcept;

class quest_mobile_native_birth_owner final
{
	struct ordinary_flat_factory_budget;
	struct ordinary_flat_item_factory_budget;
	struct ordinary_flat_publication_budget;
	static bool ordinary_flat_factory_globals(size_t *output, void *) noexcept;
	static bool reserve_ordinary_flat_factory_root(size_t full, void *opaque) noexcept;
	static bool reserve_ordinary_flat_factory_globals(size_t full, void *opaque) noexcept;
	static bool ordinary_flat_mobile_factory_selected(int rnum, int room, uint32_t slot,
							  int shop, size_t outer_live,
							  bool *refused) noexcept;
	static P_char prepare_mobile_ordinary_flat(int rnum, int room, uint32_t slot,
						   int shop) noexcept;
	static bool ordinary_flat_item_factory_selected(size_t index, size_t outer_live,
							bool *refused) noexcept;
	static bool reserve_ordinary_flat_item_factory_root(size_t full, void *opaque) noexcept;
	static bool reserve_ordinary_flat_item_factory_globals(size_t full, void *opaque) noexcept;
	static P_obj prepare_item_ordinary_flat(int rnum, size_t outer_live) noexcept;
	// Full original reducing-P native geometry and real listed shell capture.
	// The genuine shared dispatcher selects this only after its complete startup
	// capability is established; these existing domain predicates grant none.
	static bool nest_ordinary_flat(P_obj, P_obj, P_char, size_t outer_live) noexcept;
	static bool
	capture_ordinary_flat_source_pin_bounded(size_t index,
						 bool (*reserve)(size_t, void *) noexcept,
						 void *context, size_t outer_live) noexcept;
	static void seal_mobile_ordinary_flat(size_t outer_live) noexcept;
	static bool ordinary_flat_publication_source_current(size_t index) noexcept;
	static bool reserve_ordinary_flat_publication_root(size_t full, void *opaque) noexcept;
	static bool reserve_ordinary_flat_publication(size_t full, void *opaque) noexcept;
	static bool reserve_ordinary_flat_publication_globals(size_t full, void *opaque) noexcept;
	static bool ordinary_flat_service_live(size_t exclusive, size_t *output) noexcept;
	static bool observe_ordinary_flat_globals(size_t *output, void *opaque) noexcept;
	static bool settle_ordinary_flat_checkpoint(size_t index,
						    ordinary_flat_publication_budget &budget,
						    size_t outer) noexcept;
	static economic_accounting_error
	make_ordinary_flat_writer(size_t index, const critical_native_recovery_envelope &expected,
				  const native_mobile_birth_recovery_context &next,
				  void *writer_slot, ordinary_flat_publication_budget &budget,
				  size_t outer) noexcept;
	static bool checkpoint_ordinary_flat(size_t index,
					     const native_mobile_birth_recovery_context &next,
					     ordinary_flat_publication_budget &budget,
					     size_t outer) noexcept;
	static int prepare_ordinary_flat_action(size_t index, uint8_t kind, size_t row, size_t step,
						ordinary_flat_publication_budget &budget,
						size_t outer) noexcept;
	static bool finish_ordinary_flat_action(size_t index,
						const native_mobile_birth_recovery_effect &actual,
						ordinary_flat_publication_budget &budget,
						size_t outer) noexcept;

	static bool install_ordinary_flat_cash_metadata(
		size_t index, const flatfile_ordinary_native_birth_projection &current,
		ordinary_flat_publication_budget &budget, size_t outer) noexcept;
	static bool cleanup_ordinary_flat_refusal(const critical_command &command,
						  const critical_completion &completion,
						  void *opaque,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer) noexcept;
	static bool ordinary_flat_projection_matches(
		size_t index, const flatfile_ordinary_native_birth_projection &current,
		ordinary_flat_publication_budget &budget, size_t outer, bool copy_custody) noexcept;
	static bool ordinary_flat_runtime_proof(
		size_t index, const flatfile_ordinary_native_birth_projection &current,
		ordinary_flat_publication_budget &budget, size_t outer) noexcept;
	static bool retain_ordinary_flat_origin(const critical_native_recovery_envelope &terminal,
						void *opaque,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer) noexcept;
	static bool resume_ordinary_flat_cold_projection(size_t index,
							 ordinary_flat_publication_budget &budget,
							 size_t outer) noexcept;
	static bool recover_ordinary_flat_cold(size_t index, bool allow_reconstruction,
					       ordinary_flat_publication_budget &budget,
					       size_t outer) noexcept;
	static bool publish_ordinary_flat(size_t index, bool allow_reconstruction,
					  size_t outer_live) noexcept;

	friend bool quest_mobile_native_birth_retained_storage_bytes(size_t *) noexcept;
	static bool current_retained_storage_bytes(size_t *) noexcept;
	friend bool quest_mobile_native_birth_restore_ordinary_bounded(
		const critical_native_recovery_envelope &, const std::string &,
		bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
	static bool restore_ordinary_flat_bounded(const critical_native_recovery_envelope &,
						  const std::string &,
						  bool (*)(size_t, void *) noexcept, void *,
						  size_t) noexcept;

	friend class item_native_quest_publication_owner;
	friend class quest_mobile_published_world_owner;
	// Exact boot owner only, after its full original-session SQL/custody/world
	// cut. Delegations retain original NBC2/NBC3 policy and real native services;
	// no SQL, IDs, fresh birth, coordinator generation or ACK authority.
	static bool
	install_current_published_metadata(P_char, uint64_t expected_runtime_id,
					   const quest_mobile_native_image &,
					   const native_mobile_wallet_origin &) noexcept;
	static bool restore_current_published_enrollment(P_obj, P_char) noexcept;
	static bool restore_current_published_constructor_policy(
		P_char, const quest_mobile_native_constructor_recipe &) noexcept;
	// Distinguishes truly zero/unbound metadata from malformed nonzero metadata;
	// only the former is absence. Strong output on every refusal.
	static bool observe_current_published_identity(P_char, uint64_t expected_runtime_id,
						       bool *present,
						       quest_mobile_native_reference *) noexcept;

	// Pure correlation only. SQL receipt/current-cut and authentic stored-origin
	// ownership remain separate; this never restores an actor or grants admission.
	static bool
	validate_progressed_origin(const critical_native_recovery_envelope &original_birth,
				   const quest_mobile_native_image &current,
				   const native_mobile_wallet_origin &original_origin) noexcept;
	friend void reset_zone(int, int);
	friend bool quest_mobile_native_birth_restore(const critical_command &) noexcept;
	friend bool
	quest_mobile_native_birth_restore(const critical_native_recovery_envelope &) noexcept;
	friend bool quest_mobile_native_birth_restore_shared_shop_bounded(
		const critical_native_recovery_envelope &, bool (*)(size_t, void *) noexcept,
		void *, size_t) noexcept;
	struct shared_shop_restore_attachment;
	static bool restore_shared_shop_command_bounded(const critical_command &,
							const std::vector<uint8_t> &,
							bool (*)(size_t, void *) noexcept, void *,
							size_t,
							shared_shop_restore_attachment *) noexcept;
	static bool restore_shared_shop_bounded(const critical_native_recovery_envelope &,
						bool (*)(size_t, void *) noexcept, void *,
						size_t) noexcept;

	friend void quest_mobile_native_birth_completions(const critical_completion *,
							  size_t) noexcept;
	friend void quest_mobile_native_birth_pulse(bool) noexcept;
	friend bool quest_mobile_native_birth_recovery_pulse() noexcept;
	static bool begin_reset(int zone, int force) noexcept;
	static bool begin_reset_flat(int zone, int force) noexcept;
	static bool prepare_flat_reset_cursor(int zone, int force) noexcept;
	static bool reset_flat_projection_current() noexcept;
	static bool reset_dispatch_identity() noexcept;
	static bool reset_dispatch_current() noexcept;
	static bool original_command_current() noexcept;
	static bool reset_invocation_ready() noexcept;
	static bool reset_dispatch_source(uint32_t, char, int, bool, economic_source_event *,
					  int32_t *) noexcept;
	static quest_mobile_original_reset_locals *original_reset_locals(int, int) noexcept;
	// Genuine open-slot hold only. Retryable means the actual dispatcher has not
	// entered its next effect; unknown/returned-failure cuts are retained closed.
	static bool hold_reset(uint32_t, int, bool retryable) noexcept;
	static bool reset_objects_current() noexcept;
	static bool reset_actor_holds_current() noexcept;
	static bool reset_resume_current() noexcept;
	// Nonallocating observations wired only at the real sequential dispatcher cuts.
	static void observe_reset_command(uint32_t slot, int original_last_cmd) noexcept;
	static void observe_reset_processed(uint32_t slot, int original_last_cmd) noexcept;
	static void observe_reset_abort(uint32_t slot, int original_last_cmd) noexcept;
	static void observe_reset_stop(uint32_t slot, int original_last_cmd) noexcept;
	// Current execution permission is distinct from retained invocation identity.
	// Source observations share the existing lazy invocation with M and grant no
	// source claim, SQL, constructor, publication or ACK authority.
	friend class zone_reset_item_owner;
	// Exact original captured nonce/cursor only. Strong output on refusal;
	// SQL returns null, flat borrows the immutable cursor-owned selected root.
	// Borrow ends when the actual cursor finishes/replaces; caller owns copies.
	static bool capture_reset_backend(const economic_source_event &,
					  const std::string **selected_flat_root) noexcept;
	static bool capture_reset_flat_projection(const economic_source_event &,
						  critical_operation_id *lineage,
						  critical_operation_id *epoch) noexcept;
	static bool capture_room_reset_source(uint32_t command_slot, int room_rnum,
					      economic_source_event *, int32_t *zone_vnum) noexcept;
	static bool capture_retained_room_reset_source(uint32_t, int, economic_source_event *,
						       int32_t *) noexcept;
	static bool capture_reset_child_source(uint32_t, economic_source_event *,
					       int32_t *) noexcept;
	static bool capture_retained_reset_child_source(uint32_t, economic_source_event *,
							int32_t *) noexcept;
	static bool capture_reset_dispatch_scope(economic_source_event *, int32_t *,
						 int *original_force = nullptr) noexcept;
	static bool capture_reset_abort_scope(economic_source_event *, int32_t *, uint32_t *,
					      int *) noexcept;
	static bool capture_reset_boundary(uint32_t, int, economic_source_event *,
					   int32_t *) noexcept;
	static void finish_reset() noexcept;
	static void seal_mobile() noexcept;
	friend class quest_mobile_native_npc_flat_factory_scope;
	static bool
	npc_flat_factory_scope_current(const quest_mobile_native_npc_flat_factory_scope &) noexcept;
	static size_t npc_flat_factory_scope_current_frames() noexcept;
	static size_t npc_flat_projection_source_frames() noexcept;
	static bool capture_npc_flat_factory_scope(size_t, size_t) noexcept;
	static bool
	borrow_npc_flat_factory_scope(P_char, const quest_mobile_native_npc_flat_factory_scope **,
				      size_t) noexcept;
	static bool reserve_npc_binding_scratch(size_t, void *) noexcept;
	static P_obj prepare_item_flat(int, size_t private_live) noexcept;
	static bool capture_alchemist_spawn_flat(P_char, int) noexcept;
	static void seal_mobile_flat(size_t private_live) noexcept;
	static void block_mobile_flat() noexcept;
	static void finish_reset_flat() noexcept;
	static bool ordinary_flat_freeze_source_current(size_t) noexcept;
	static bool freeze_ordinary_flat_command(size_t, size_t);
	struct ordinary_flat_envelope_budget;
	static size_t ordinary_flat_envelope_source_frames() noexcept;
	static bool reserve_ordinary_flat_envelope(size_t, void *) noexcept;
	static bool reserve_ordinary_flat_submission(size_t, void *) noexcept;
	static bool prepare_ordinary_flat_envelope(size_t, size_t);
	static bool ordinary_flat_submission_role_current(size_t, size_t) noexcept;
	static critical_submit_result submit_ordinary_flat_envelope(size_t, size_t,
								    bool *) noexcept;

	struct ordinary_flat_observation_budget;
	static bool reserve_ordinary_flat_observation(size_t, void *) noexcept;
	static bool ordinary_flat_observation_source_current(size_t) noexcept;
	static bool observe_ordinary_flat_post_submit(size_t, size_t) noexcept;

	static bool capture_ordinary_flat_source_pin(size_t) noexcept;
	static bool ordinary_flat_execution_source_current(
		const quest_mobile_native_birth_ordinary_execution_lease &, const void *) noexcept;
	static void service_ordinary_flat_execution_requests() noexcept;
	// Pure owning source capture/carrier only. General shared admission stays closed.
	static bool capture_shared_checkpoint(size_t) noexcept;
	static bool prepare_shared_capture(size_t) noexcept;
	static void block_mobile() noexcept;
	static P_char prepare_mobile(int rnum, int room, uint32_t slot, int shop) noexcept;
	// Private original M constructor/source leaf only, deliberately unselected.
	// Exact entered M cursor/root/projection; final NPC item/binding/seal and
	// producer/postreceipt joins are separate. Values cannot mint a source pin.
	static bool flat_mobile_factory_current(int, int, uint32_t, size_t private_live) noexcept;
	static bool capture_flat_mobile_factory_source(int, int, uint32_t, economic_source_event *,
						       int32_t *, size_t private_live) noexcept;
	static P_char prepare_mobile_flat(int rnum, int room, uint32_t slot, int shop) noexcept;

	static bool capture_alchemist_spawn(P_char, int original_room) noexcept;
	static P_obj prepare_item(int rnum) noexcept;
	static bool owns(P_char) noexcept;
	static bool discard_item(P_obj) noexcept;
	static bool carry(P_obj, P_char) noexcept;
	static bool equip(P_obj, P_char, int) noexcept;
	static bool nest(P_obj, P_obj, P_char) noexcept;
	// False retains output and means an observation/owner gap; true/null is
	// authentic absence. Selection grants no foreign-target custody authority.
	static bool original_object(int rnum, P_obj *selected) noexcept;
	static void skipped_item(P_char, P_obj) noexcept;
	static size_t pending_mobiles(int rnum) noexcept;
	static size_t pending_items(int rnum) noexcept;
	static bool pending_shop(int rnum, int room, int shop) noexcept;
	static bool restore(const critical_command &) noexcept;
	friend bool quest_mobile_native_birth_restore_historical_bounded(const critical_command &,
									 bool (*)(size_t,
										  void *) noexcept,
									 void *, size_t) noexcept;
	friend bool quest_mobile_native_birth_restore_historical_bounded(
		const critical_native_recovery_envelope &, bool (*)(size_t, void *) noexcept,
		void *, size_t) noexcept;
	static bool restore_historical_command_bounded(const critical_command &,
						       const std::vector<uint8_t> *,
						       bool (*)(size_t, void *) noexcept, void *,
						       size_t) noexcept;
	static bool restore_historical_bounded(const critical_native_recovery_envelope &,
					       bool (*)(size_t, void *) noexcept, void *,
					       size_t) noexcept;

	static bool restore_command(const critical_command &,
				    const std::vector<uint8_t> *) noexcept;
	static void completions(const critical_completion *, size_t) noexcept;
	static void pulse(bool) noexcept;
	static void pulse_policy(bool prepare_original_resets, bool recovery_only) noexcept;
	static bool recovery_pulse() noexcept;
	static bool charge() noexcept;
	// Lifetime allowance in the same aggregate budget; call charge() after scratch dies.
	static bool charge(size_t prospective_scratch) noexcept;
	static bool discard(size_t) noexcept;
	static bool publish(size_t, bool allow_reconstruction = false) noexcept;
	static bool recover_cold(size_t, bool allow_reconstruction) noexcept;
	static bool clear_cold_projection(size_t) noexcept;
	static bool resume_cold_projection(size_t) noexcept;
	static bool settle_checkpoint(size_t) noexcept;
	static bool checkpoint(size_t, const native_mobile_birth_recovery_context &) noexcept;
	static int prepare_action(size_t, uint8_t, size_t, size_t) noexcept;
	static bool finish_action(size_t, const native_mobile_birth_recovery_effect &) noexcept;
	static bool restore(const critical_native_recovery_envelope &) noexcept;
	static bool cleanup_refusal(const critical_command &, const critical_completion &,
				    void *) noexcept;
};
// Complete original historical passive restore acceptance. Bare non-birth
// commands retain original true behavior; envelopes require original complete
// family validity. No configured-flat domain, progression or readiness grant.
// Caller owns inputs and fresh SAME registry CURRENT through every callback.
// Callee owns private workspace/body until actual transfer; ROOT then owns it.
bool quest_mobile_native_birth_restore_historical_bounded(const critical_command &,
							  bool (*)(size_t, void *) noexcept, void *,
							  size_t) noexcept;
bool quest_mobile_native_birth_restore_historical_bounded(const critical_native_recovery_envelope &,
							  bool (*)(size_t, void *) noexcept, void *,
							  size_t) noexcept;

// Genuine owning upper source includes the original keeper call SOURCE once;
// lower birth codecs own their child SOURCE. Actual ROOT callback targets remain
// caller-owned, with no foreign registry or keeper physical bytes substituted.
// Both query outputs remain unchanged on unsupported ordinary GNU13 source.
bool quest_mobile_native_birth_historical_restore_source_frame_bytes(size_t *) noexcept;
bool quest_mobile_native_birth_historical_restore_initial_inline_bytes(size_t *) noexcept;
constexpr size_t quest_mobile_native_birth_historical_restore_source_query_frame_bytes() noexcept
{
	// Own output and checked codec/keeper output formals; codec/keeper/bytes/
	// copy locals, copy return and five genuine keeper query size_t carriers;
	// own/support/codec/keeper bool results. Caller accessor return is extra N.
	return 3 * sizeof(void *) + 10 * sizeof(size_t) + 4 * sizeof(bool);
}
constexpr size_t quest_mobile_native_birth_historical_restore_initial_query_frame_bytes() noexcept
{
	return sizeof(size_t *) + sizeof(bool) + sizeof(bool);
}
#endif
