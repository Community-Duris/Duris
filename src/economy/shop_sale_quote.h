#ifndef SHOP_SALE_QUOTE_H
#define SHOP_SALE_QUOTE_H

#include "core/utils.h"

namespace shop_sale_quote
{
template <class Observations> int sale_base(Observations &observations)
{
	float cost_factor;
	int sale;

	cost_factor = observations.charisma_modifier();
	if (observations.different_races())
		cost_factor = cost_factor / 2.;

	cost_factor = observations.buy_percent() * (1.0 + (cost_factor / 100.));
	if (cost_factor > observations.buy_percent())
		cost_factor = observations.buy_percent() - .01;

	/* condition affects value too */
	sale = (int)(observations.object_cost() * cost_factor * MIN(100, observations.condition()) /
		     100);

	if (sale < 1)
		sale = 1;
	return sale;
}

template <class Observations> int valuation_base(Observations &observations)
{
	float cost_factor;
	int sale;

	cost_factor = observations.charisma_modifier();

	if (observations.different_races())
		cost_factor = cost_factor / 2;

	cost_factor = observations.buy_percent() * (1.0 + (cost_factor / 100.));

	if (cost_factor > observations.buy_percent())
		cost_factor = observations.buy_percent() - .01;

	if (observations.barter_enabled())
	{
		if (observations.barter_successful())
		{
			cost_factor -= .25;
		}
		else
		{
			cost_factor += .10;
		}
	}
	/* condition affects value too */
	sale = (int)(observations.object_cost() * cost_factor * MIN(100, observations.condition()) /
		     100);

	if (sale < 1)
		sale = 1;
	return sale;
}

struct adjusted_quote
{
	int price;
	int trophy_count;
};

template <class Observations> adjusted_quote adjust(int sale, Observations &observations)
{
	int temp = 0;
	if ((temp = observations.trophy_count()) > 1)
	{
		int orig_sale = sale;

		sale -= (int)(observations.trophy_modifier() * temp * sale);
		sale = MAX((int)(observations.minimum_percent() * orig_sale), sale);

		if (sale < 1)
			sale = 1;
	}
	return { sale, temp };
}
}

#endif
