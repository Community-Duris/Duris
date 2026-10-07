#ifndef DURIS_ENHANCEMENT_AFFECT_POLICY_H
#define DURIS_ENHANCEMENT_AFFECT_POLICY_H

#include <array>
#include <cstddef>

using enhancement_affect_words = std::array<unsigned long, 5>;

inline bool enhancement_affects_banned(const enhancement_affect_words &affects,
				       const enhancement_affect_words &allowed)
{
	for (std::size_t word = 0; word < affects.size(); ++word)
		if (affects[word] & ~allowed[word])
			return true;
	return false;
}

#endif
