#include "world/githyanki_item_policy.h"
#include "core/structs.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include <cstdio>

githyanki_level_item_choice githyanki_level_item_select(P_char ch) noexcept
{
	if (GET_CLASS(ch, CLASS_NECROMANCER) || GET_CLASS(ch, CLASS_SORCERER) ||
	    GET_CLASS(ch, CLASS_PSIONICIST) || GET_CLASS(ch, CLASS_CONJURER) ||
	    GET_CLASS(ch, CLASS_ILLUSIONIST) || GET_CLASS(ch, CLASS_SUMMONER))
		return {
			19, "staff silverish silver %sz",
			"&+CWith a blinding flash of light, a staff suddenly appears in your hands!\r\n",
			"&+CIn a blinding flash of light, a staff suddenly appears in $n's hands!"
		};
	if (GET_CLASS(ch, CLASS_WARRIOR) || GET_CLASS(ch, CLASS_REAVER))
		return {
			418, "warhammer hammer silverish silver %sz",
			"&+CWith a blinding flash of light, a warhammer suddenly appears in your hands!\r\n",
			"&+CIn a blinding flash of light, a warhammer suddenly appears in $n's hands!"
		};
	if (GET_CLASS(ch, CLASS_ANTIPALADIN))
		return {
			18, "longsword sword long silverish silver %sz",
			"&+CWith a blinding flash of light, a sword suddenly appears in your hands!\r\n",
			"&+CIn a blinding flash of light, a sword suddenly appears in $n's hands!"
		};
	return { 0, nullptr, "&+CYou should have gotten a special item.  Let a god know.\r\n",
		 nullptr };
}

int githyanki_level_item_format_keywords(char *output, size_t capacity, int vnum,
					 const char *name) noexcept
{
	switch (vnum)
	{
	case 19:
		return std::snprintf(output, capacity, "staff silverish silver %sz", name);
	case 418:
		return std::snprintf(output, capacity, "warhammer hammer silverish silver %sz",
				     name);
	case 18:
		return std::snprintf(output, capacity, "longsword sword long silverish silver %sz",
				     name);
	default:
		return -1;
	}
}
