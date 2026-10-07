#ifndef DURIS_ENHANCEMENT_ESSENCE_REWARD_H
#define DURIS_ENHANCEMENT_ESSENCE_REWARD_H

#include <cstdint>

struct enhancement_essence_reward
{
	bool primary_passed = false;
	int vnum = 0;
};

// Native observations run synchronously; retain the previously captured limits.
template <typename Observations>
enhancement_essence_reward enhancement_select_essence_reward(int64_t moblvl, int primary_roll_max,
							     int max_roll_max,
							     Observations &observed)
{
	enhancement_essence_reward result;
	if (observed.roll(1, primary_roll_max) < moblvl)
	{
		result.primary_passed = true;
		observed.primary_notice(moblvl);
		if (observed.roll(1, max_roll_max) < moblvl)
		{
			switch (observed.roll(1, 8))
			{
			case 1:
				result.vnum = 400239;
				break;
			case 2:
				result.vnum = 400241;
				break;
			case 3:
				result.vnum = 400243;
				break;
			case 4:
				result.vnum = 400245;
				break;
			case 5:
				result.vnum = 400247;
				break;
			case 6:
				result.vnum = 400249;
				break;
			case 7:
				result.vnum = 400251;
				break;
			case 8:
				result.vnum = 400253;
				break;
			}
		}
		else
		{
			result.vnum = observed.roll(1, 13);
			switch (result.vnum)
			{
			case 1:
				result.vnum = 400238;
				break;
			case 2:
				result.vnum = 400240;
				break;
			case 3:
				result.vnum = 400242;
				break;
			case 4:
				result.vnum = 400244;
				break;
			case 5:
				result.vnum = 400246;
				break;
			case 6:
				result.vnum = 400248;
				break;
			case 7:
				result.vnum = 400250;
				break;
			case 8:
				result.vnum = 400252;
				break;
			case 9:
				result.vnum = 400254;
				break;
			case 10:
				result.vnum = 400255;
				break;
			case 11:
				result.vnum = 400256;
				break;
			case 12:
				result.vnum = 400257;
				break;
			case 13:
				result.vnum = 400258;
				break;
			}
		}
	}
	return result;
}

#endif
