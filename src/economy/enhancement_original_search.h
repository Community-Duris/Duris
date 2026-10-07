#ifndef DURIS_ENHANCEMENT_ORIGINAL_SEARCH_H
#define DURIS_ENHANCEMENT_ORIGINAL_SEARCH_H

#include <climits>
#include <cstdint>

// new_value retains the producer's native-int value/gain sum. Observe changing
// search configuration synchronously; native lookup and result ownership stay
// with the provider. Even an all-invalid unsuccessful step spends the budget.
template <typename Observations> inline bool
enhancement_search_original(int64_t new_value, int64_t max_search, Observations &observed)
{
	int64_t search_count = 0;
	for (int64_t step = 0; step <= observed.max_roll(); step++)
	{
		for (int direction = 0; direction < 2; direction++)
		{
			int64_t candidate;
			if (step == 0)
			{
				if (direction > 0)
					continue;
				candidate = new_value;
			}
			else if (observed.down_first())
				candidate = direction == 0 ? new_value - step : new_value + step;
			else
				candidate = direction == 0 ? new_value + step : new_value - step;
			if (candidate < 1 || candidate > INT_MAX ||
			    candidate > observed.value_limit())
				continue;
			if (observed.try_value(static_cast<int>(candidate)))
				return true;
		}
		search_count++;
		if (search_count > max_search)
			break;
	}
	return false;
}

#endif
