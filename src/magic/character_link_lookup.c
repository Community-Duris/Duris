#include "core/prototypes.h"

P_char get_linked_char(P_char ch, ush_int type)
{
	struct char_link_data *cld;

	for (cld = ch->linking; cld; cld = cld->next_linking)
		if (cld->type == type)
			return cld->linked;

	return NULL;
}
