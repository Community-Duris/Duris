#ifndef DURIS_SKILL_SELECTION_PREPARATION_H
#define DURIS_SKILL_SELECTION_PREPARATION_H

#include "core/structs.h"
#include <span>

struct player_skill_selection_input
{
	unsigned int primary_classes = 0, secondary_classes = 0;
	int specialization = 0;
	bool npc = false, multiclass_npc = false;
};

struct player_skill_selection_modifiers
{
	float primary = 100.f, secondary = 95.f;
};

// One complete, genuinely initialized CLASS_COUNT row set, including every
// specialization. The owner retains its immutable table/configuration cut and
// supplies the prepared class/spec/NPC flags, rather than BEFORE selection.
// These values establish no source, actor lifetime, transaction or ACK authority.
// Invalid indices/spec or an unsafe original float-to-int conversion refuse
// without changing output. NPC multiclass retains the original strict-greater
// scan and class-zero default when no flagged class has a positive base cap.
bool player_skill_select_prepared_values(const player_skill_selection_input &,
					 std::span<const ClassSkillInfo>,
					 const player_skill_selection_modifiers &,
					 ClassSkillInfo *) noexcept;
bool player_skill_required_level_prepared_values(const player_skill_selection_input &,
						 std::span<const ClassSkillInfo>,
						 const player_skill_selection_modifiers &,
						 int *) noexcept;

// Original live arithmetic/selection, shared by the checked value route above.
// The native wrapper owns its property reads and invalid-primary diagnostic/
// warrior mutation. Original multiclass indices/spec and conversions must be
// defined; preparation callers use the checked entry point. False means the
// original single-class invalid-primary fallback is required, or a bad view/
// output was supplied. No fallback or actor mutation occurs in this calculus.
bool player_skill_select_original_values(const player_skill_selection_input &,
					 std::span<const ClassSkillInfo>,
					 const player_skill_selection_modifiers &,
					 ClassSkillInfo *) noexcept;
int player_skill_required_level_original_values(const ClassSkillInfo &,
						int specialization) noexcept;

#endif
