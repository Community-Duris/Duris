#include "core/prototypes.h"

#include <cassert>

int main()
{
	char_data actor{}, first{}, second{};
	assert(get_linked_char(&actor, LNK_PET) == nullptr);
	char_link_data tail{};
	tail.type = LNK_PET;
	tail.linking = &actor;
	tail.linked = &second;
	char_link_data head{};
	head.type = LNK_RIDING;
	head.linking = &actor;
	head.linked = &first;
	head.next_linking = &tail;
	actor.linking = &head;
	assert(get_linked_char(&actor, LNK_RIDING) == &first);
	assert(get_linked_char(&actor, LNK_PET) == &second);
	assert(get_linked_char(&actor, LNK_BLOOD_ALLIANCE) == nullptr);
	head.type = LNK_PET;
	assert(get_linked_char(&actor, LNK_PET) == &first);
	assert(actor.linking == &head && head.next_linking == &tail && head.linked == &first &&
	       tail.linked == &second);
	// Reverse links and type registry remain separate from the original forward
	// read; neither creates a missing link or changes the first-match order.
	actor.linking = nullptr;
	actor.linked = &head;
	assert(get_linked_char(&actor, LNK_PET) == nullptr);
}
