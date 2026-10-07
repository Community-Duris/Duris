#ifndef DURIS_SHOP_PURCHASE_QUOTE_H
#define DURIS_SHOP_PURCHASE_QUOTE_H

// Prepare one purchase price with the native float arithmetic and observation order.
// The caller retains this price for any produced-stock continuation.
template <typename Observations> int shop_prepare_purchase_quote(Observations &observations)
{
	float cost_factor = observations.charisma_modifier();
	if (!observations.same_race())
		cost_factor = cost_factor * 2.;

	cost_factor = observations.sell_percent() * (1.0 - (cost_factor / 100.));
	if (cost_factor < observations.sell_percent())
	{
		cost_factor = observations.sell_percent() + .01;
	}

	if (observations.barter_enabled())
	{
		if (observations.barter_succeeds())
		{
			cost_factor -= .25;
		}
		else
		{
			cost_factor += .10;
		}
	}

	int sale = (int)(observations.item_cost() * cost_factor);
	sale -= (int)(sale * observations.epic_bonus());
	if (sale < 1)
	{
		sale = 1;
	}
	return sale;
}

#endif
