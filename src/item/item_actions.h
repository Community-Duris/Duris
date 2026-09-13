#ifndef DURIS_ITEM_ACTIONS_H
#define DURIS_ITEM_ACTIONS_H

#include "core/structs.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

// Item actions have their own activity and timing. They never borrow a
// character's spellcast payload, spell slots, AFF2_CASTING or event_wait.
enum class item_action_mode
{
	passive,
	active
};
enum class item_action_source
{
	equipped,
	carried
};
enum class item_action_call
{
	weapon,
	wand,
	staff,
	scroll,
	spell
};

enum class item_action_effect_target
{
	original,
	actor
};
enum class item_action_consumption
{
	none,
	committed,
	reserved,
	rejected
};
enum class item_action_outcome
{
	completed,
	partially_resolved,
	interrupted
};
enum class item_action_start
{
	legacy,
	scheduled,
	suppressed
};

constexpr size_t ITEM_ACTION_MAX_EFFECTS = 3;

struct item_action_effect
{
	uint32_t id = 0; // Typed adapter's effect id, never a raw special-proc address.
	int power = 0;
	item_action_call call = item_action_call::weapon;
	item_action_effect_target target = item_action_effect_target::original;
	int auxiliary =
		0; // Typed adapter data, copied at selection (for example a drain's heal cap).
};

struct item_action_definition
{
	uint32_t id = 0;
	uint64_t revision = 0;
	item_action_mode mode = item_action_mode::passive;
	item_action_source source = item_action_source::equipped;
	int windup_pulses = 0;
	std::array<item_action_effect, ITEM_ACTION_MAX_EFFECTS> effects = {};
	size_t effect_count = 0;
	bool selected_effects = false;
	int progress_pulses = 0; // Optional single progress beat within the windup.
};

// Selected once by the original trigger/RNG path, before admission. Only an
// explicitly opted-in definition accepts invocation-specific effect parameters.
struct item_action_selection
{
	std::array<item_action_effect, ITEM_ACTION_MAX_EFFECTS> effects = {};
	size_t effect_count = 0;
};

struct item_action_identity
{
	uint64_t action_id = 0; // Single-use transition token; departure invalidates it.
	uint64_t source_uid = 0;
	uint64_t actor_id = 0;
	uint64_t target_id = 0;
	uint32_t ability_id = 0;
	uint64_t revision = 0;
	int origin_room = NOWHERE;
	int source_slot = -1;
	unsigned long long deadline_tick = 0;
};

// Borrowed live pointers are valid only within ONE adapter call. The runtime
// reacquires and revalidates participants before every effect. An adapter must
// not retain these pointers, retarget, or use them after its own effect extracts
// a participant. Definitions and selected effects are copied at registration.
struct item_action_context
{
	const item_action_identity &identity;
	const item_action_definition &definition;
	P_char actor;
	P_char target;
	P_obj source;
};

class item_action_adapter
{
    public:
	virtual ~item_action_adapter() = default;
	// Read-only permission/eligibility checks, both at admission and completion.
	virtual bool validate(const item_action_context &) const noexcept = 0;
	// Atomic, synchronous resource operation AFTER scheduler acceptance. Rejected
	// means no cost/cooldown was changed. No gameplay callbacks or transitions here.
	virtual item_action_consumption commit(const item_action_context &) const noexcept = 0;
	virtual void announce(const item_action_context &) const noexcept = 0;
	// Presentation only; no world mutations, resource changes or new actions.
	virtual void progress(const item_action_context &) const noexcept {}
	virtual void resolve(const item_action_context &,
			     const item_action_effect &) const noexcept = 0;
	// Exactly once after an accepted cost operation, including cancellation. Only
	// identities are provided: the source/participants may already be gone. Release
	// reservations according to the adapter policy (partially_resolved has already
	// invoked an effect and must not get a free refund); committed costs are not refilled
	// by the framework. Must not invoke gameplay callbacks or start new actions.
	virtual void finish(const item_action_identity &, item_action_consumption,
			    item_action_outcome) const noexcept = 0;
};

// Main/game-thread only. No definitions are installed at boot by this foundation.
// Publish copies the definition and takes exclusive ownership of its adapter;
// updates must strictly increase the revision and cancel the previous revision.
bool item_actions_publish(const item_action_definition &, std::unique_ptr<item_action_adapter>);
void item_actions_disable(uint32_t ability_id);
void item_actions_reload(); // Data reload barrier: cancel all and discard definitions.
void update_item_action_properties();
bool item_actions_enabled();
uint64_t item_actions_definition_revision(uint32_t ability_id);

// Only `legacy` permits a caller to use its original instant path. A selected
// new action that cannot start (including caps/rejection) is always suppressed.
item_action_start start_item_action(uint32_t ability_id, P_char actor, P_char target, P_obj source);
item_action_start start_selected_item_action(uint32_t ability_id, P_char actor, P_char target,
					     P_obj source, const item_action_selection &);
bool item_action_active(P_char actor);
bool abort_item_action(P_char actor);
size_t item_actions_pending();

// Call before a real room departure, extraction, source transfer or unequip.
// Both actor and original-target departures cancel; returning cannot revive it.
void item_actions_character_leaving(P_char);
void item_actions_source_leaving(P_obj);

// Distinct callbacks let the scheduler prioritize active player devices only.
void event_item_action_active(P_char, P_char, P_obj, void *);
void event_item_action_passive(P_char, P_char, P_obj, void *);

#endif
