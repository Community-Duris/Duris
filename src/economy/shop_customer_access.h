#ifndef SHOP_CUSTOMER_ACCESS_H
#define SHOP_CUSTOMER_ACCESS_H

#include "core/utils.h"

namespace shop_customer_access
{
enum class refusal
{
	none,
	before_open,
	between_sessions,
	after_close,
	wrong_race,
	unseen
};
struct decision
{
	int allowed;
	refusal reason;
};
template <class Observations> decision classify(Observations &observations)
{
	if (observations.first_open() > observations.hour())
	{
		return { FALSE, refusal::before_open };
	}
	else if (observations.first_close() < observations.hour())
	{
		if (observations.second_open() > observations.hour())
		{
			return { FALSE, refusal::between_sessions };
		}
		else if (observations.second_close() < observations.hour())
		{
			return { FALSE, refusal::after_close };
		}
	}
	/*
	 * If shopkeeper is racist, turn customer away. MIAX
	 */
	if ((observations.racist() == 1) && !observations.trusted())
	{
		if (observations.keeper_race() != observations.customer_race())
		{
			return { FALSE, refusal::wrong_race };
		}
	}
	if (!(observations.visible()) && !observations.trusted())
	{
		return { FALSE, refusal::unseen };
	};

	switch (observations.customer_mode())
	{
	case 0:
		return { TRUE, refusal::none };
	case 1:
		return { TRUE, refusal::none };
	default:
		return { TRUE, refusal::none };
	};
}
}

#endif
