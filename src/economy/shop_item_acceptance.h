#ifndef SHOP_ITEM_ACCEPTANCE_H
#define SHOP_ITEM_ACCEPTANCE_H

#include "core/utils.h"
#include "economy/shop.h"

namespace shop_item_acceptance
{
template <class Observations> int classify(Observations &observations, char repairing)
{
	int counter;

	if (observations.object_cost() < 1)
		return (OBJECT_NOTOK);

	if ((observations.extra_flag(ITEM_NOSELL) && !repairing) ||
	    observations.extra_flag(ITEM_TRANSIENT))
		return (OBJECT_NOTOK);

	for (counter = 0; observations.configured_type(counter) != 0; counter++)
	{
		if (observations.configured_type(counter) == observations.object_type())
		{
			if (((observations.charges() == 0) &&
			     ((observations.object_type() == ITEM_WAND) ||
			      (observations.object_type() == ITEM_STAFF))))
				return (OBJECT_DEAD);
			else if (observations.evaluate_keywords(counter))
				return (OBJECT_OK);
			return (OBJECT_OK);
		}
		else if (observations.configured_type(counter) == ITEM_ARMOR &&
			 observations.object_type() == ITEM_WORN)

			return (OBJECT_OK);
	}
	return (OBJECT_NOTOK);
}
}

#endif
