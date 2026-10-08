#include "economy/cardgames.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

// Literal single-card oracle, by legal Card ID 1..52. No score implementation,
// protected-field access, native participant or replacement provider.
static constexpr std::array<int, 52> single_scores = { 11, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10, 10,
						       11, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10, 10,
						       11, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10, 10,
						       11, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10, 10 };

struct scenario
{
	const char *name;
	std::vector<int> ids;
	std::vector<int> prefix_scores;
};

// Prefix expectations are written independently, not calculated from ranks or
// copied from another hand/capture. Three selected orders share final score 20.
static const std::array<scenario, 14> scenarios = {
	scenario{ "ace_nine_ten", { 1, 9, 10 }, { 11, 20, 20 } },
	scenario{ "ace_ten", { 1, 10 }, { 11, 21 } },
	scenario{ "two_aces", { 1, 14 }, { 11, 12 } },
	scenario{ "two_aces_nine", { 1, 14, 9 }, { 11, 12, 21 } },
	scenario{ "two_aces_ten", { 1, 14, 10 }, { 11, 12, 12 } },
	scenario{ "four_aces_nine", { 1, 14, 27, 40, 9 }, { 11, 12, 13, 14, 13 } },
	scenario{ "bust", { 10, 23, 5 }, { 10, 20, 25 } },
	scenario{ "five_cards", { 2, 3, 4, 5, 6 }, { 2, 5, 9, 14, 20 } },
	scenario{ "soft_seventeen", { 1, 6, 10 }, { 11, 17, 17 } },
	scenario{ "hard_ace", { 10, 9, 1 }, { 10, 19, 20 } },
	scenario{ "order_ten_ace_nine", { 10, 1, 9 }, { 10, 21, 20 } },
	scenario{ "order_nine_ten_ace", { 9, 10, 1 }, { 9, 19, 20 } },
	scenario{ "four_suits_two", { 2, 15, 28, 41 }, { 2, 4, 6, 8 } },
	scenario{ "three_faces", { 11, 25, 52 }, { 10, 20, 30 } }
};

static void observe(Hand &hand, int score, int count)
{
	for (int repeat = 0; repeat < 3; ++repeat)
	{
		assert(hand.getOwner() == nullptr);
		assert(hand.BlackjackValue() == score);
		assert(hand.numCards() == count);
	}
}

static void exercise(const std::vector<int> &ids, const std::vector<int> &prefix_scores)
{
	assert(ids.size() == prefix_scores.size());
	// A separate real hand, with its own allocations, stays live throughout.
	Hand independent;
	independent.ReceiveCard(new Card(3));
	independent.ReceiveCard(new Card(4));
	observe(independent, 7, 2);

	auto source = std::make_unique<Hand>();
	std::vector<P_Card> tracked;
	observe(*source, 0, 0);
	for (size_t i = 0; i < ids.size(); ++i)
	{
		assert(ids[i] >= 1 && ids[i] <= 52);
		assert(std::find(ids.begin(), ids.begin() + i, ids[i]) == ids.begin() + i);
		auto *card = new Card(ids[i]);
		assert(std::find(tracked.begin(), tracked.end(), card) == tracked.end());
		tracked.push_back(card);
		source->ReceiveCard(card);
		observe(*source, prefix_scores[i], static_cast<int>(i + 1));
		observe(independent, 7, 2);
	}

	const int final_score = prefix_scores.empty() ? 0 : prefix_scores.back();
	const int count = static_cast<int>(ids.size());
	observe(*source, final_score, count);
	P_Card original_head = tracked.empty() ? nullptr : tracked.back();
	assert(source->Fold() == original_head);
	observe(*source, 0, 0);
	assert(source->Fold() == nullptr);
	observe(independent, 7, 2);

	{
		Hand destination;
		observe(destination, 0, 0);
		// Fold relinquished the entire list. Reattach each known allocation only
		// through the genuine public operation, without reading/writing links.
		for (size_t i = 0; i < tracked.size(); ++i)
		{
			destination.ReceiveCard(tracked[i]);
			observe(destination, prefix_scores[i], static_cast<int>(i + 1));
			observe(*source, 0, 0);
			observe(independent, 7, 2);
		}
		// Destroy the emptied original hand before the new owner. An accidental
		// retained ownership would invalidate the destination under ASan.
		source.reset();
		observe(destination, final_score, count);
		independent.ReceiveCard(new Card(13));
		observe(independent, 17, 3);
		observe(destination, final_score, count);
	} // The genuine destination destructor releases the transferred cards once.
	tracked.clear(); // Nonowning addresses; never dereference after destruction.
	observe(independent, 17, 3);
} // Its genuine destructor releases the independent hand's own cards.

int main(int argc, char **argv)
{
	assert(argc == 2);
	const std::string name = argv[1];
	if (name == "empty")
		exercise({}, {});
	else if (name.starts_with("single_"))
	{
		const int id = std::stoi(name.substr(7));
		assert(id >= 1 && id <= 52);
		exercise({ id }, { single_scores[id - 1] });
	}
	else
	{
		const auto found = std::find_if(scenarios.begin(), scenarios.end(),
						[&](const auto &entry)
						{ return entry.name == name; });
		assert(found != scenarios.end());
		exercise(found->ids, found->prefix_scores);
	}
	std::printf("PASS %s genuine_hand_score_ownership=1 gambling_native_publication=0\n",
		    name.c_str());
}
