#include "core/prototypes.h"
#include "core/structs.h"

typedef struct Deck *P_Deck;
typedef struct Card *P_Card;
typedef struct Hand *P_Hand;

class Card
{
	friend class Hand;
	friend class Deck;

    private:
	int value;

    protected:
	P_Card nextCard;

	const char *getSuit();
	const char *getValue();
	int getNumber() { return value; };

    public:
	Card(int val, P_Card next = NULL)
	{
		value = (val < 1) ? 1 : (val > 52) ? 52 : val;
		nextCard = next;
	};
	char *Display();
	char *Display2();
};

class Deck
{
    protected:
	P_Card cards;
	int numCards;

    public:
	Deck();
	void Shuffle(int numShuffles);
	P_Card DealACard();
	char *Display();
	char *Display2();
	~Deck()
	{
		while (cards != NULL)
		{
			P_Card a = cards;
			cards = cards->nextCard;
			delete a;
		}
	};
};

class Hand
{
    protected:
	P_Card cards;
	P_char owner;

    public:
	Hand()
	{
		cards = NULL;
		owner = NULL;
	};
	Hand(P_char pl)
	{
		cards = NULL;
		owner = pl;
	};
	int BlackjackValue();
	int numCards()
	{
		int i = 0;
		P_Card c = cards;
		while (c)
		{
			i++;
			c = c->nextCard;
		};
		return i;
	};
	P_char getOwner() { return owner; };
	void ReceiveCard(P_Card newCard)
	{
		newCard->nextCard = cards;
		cards = newCard;
	};
	char *Display();
	P_Card Fold()
	{
		P_Card a = cards;
		cards = NULL;
		return a;
	};
	~Hand()
	{
		while (cards != NULL)
		{
			P_Card a = cards;
			cards = cards->nextCard;
			delete a;
		}
	};
};

void event_object_proc(P_char, P_char, P_obj, void *);

#define BJ_PREBID 0
#define BJ_POSTBID 1
#define BJ_POSTDEAL 2
#define BJ_POSTHIT 3
#define BJ_DEALERSTURN 4

#define STR_PLAT "&+WPlatinum&n"
#define STR_GOLD "&+YGold&n"
#define STR_SILV "&+wSilver&n"
#define STR_COPP "&+yCopper&n"

#define STR_CARDS_ARG_FAIL \
	"&+yYou must offer a positive amount of a valid coin type (ie 1 p, or 2 gold)."
#define STR_CARDS_TYPE_FAIL \
	"&+ySorry, we only accept &+Yvalid cash&+y amounts here. Please ensure you offer a valid amount and selection of coins!!&n"
#define STR_CARDS_CASH_FAIL "&+yYou &+Ydont have enough &+ycoins of that type available!&n"
#define STR_CARDS_BIGBID_FAIL "&+ySorry, we don't accept bids that large!&n"
#define STR_CARDS_CASH_OK \
	"&+ySeeing that you &+Yhave enough &+ycoins available to cover your &+Wbet&+y it looks like its time to &+Wdeal&+y!!&n"
#define STR_CARDS_FAILED "&+yYou mouth the word and are &+Mmagically&+y understood."
#define STR_CARDS_BID_FAIL "&+yYou must make a bid first, using the offer command."
#define STR_CARDS_GAME_ON \
	"&+yThere is already a game in progress.  You can say 'showgame' to see the current status."
#define STR_CARDS_SHUFFLE \
	"&+yThe cards are quickly &+Rs&+Ch&+Bu&+Gf&+Yf&+Ml&+Ce&+Rd&+y and stacked, ready for a new &+Wgame!!&n\n"
#define STR_CARDS_DEAL                                                                                                                                                                                    \
	"&+yA brief sizzle of &+bm&+Ba&+Cg&+Wi&+Cc&+Ba&+bl &+bf&+Bl&+Cam&+Be&+bs&+y flickers over the deck, and some of the cards begin to &+Rr&+Ca&+Bn&+Gd&+Yo&+Mm&+Cl&+Ry&+y separate into two piles. " \
	"One pile is before you, and the other is in front of the &+wdeck&+y itself. The cards reveal themselves to you quickly.&n\n"
#define STR_CARDS_CHOOSE                                                                                                                                                                                   \
	"&+yThe &+bm&+Ba&+Cg&+Wi&+Cc&+Ba&+bl &+bf&+Bl&+Cam&+Be&+bs&+y flicker over the &+wdeck&+y once more, and you realise that you can choose to either &+Whit&+y, &+Wstay&+y or &+Wfold&+y simply by " \
	"telling the cards what you wish to do.&n\n"
#define STR_CARDS_GAME_0 \
	"&+yYou dont appear to be in a game yet, or there are no cards dealt. Please tell me when its time to &+Wdeal&+y cards and we can begin playing!"
#define STR_CARDS_GAME_DEAL \
	"&+yThere were already some cards out for you, you should either &+Whit&+y, &+Wstay&+y or &+Wfold&+y your current game."
#define STR_CARDS_FOLD                                                                                                                                                                                     \
	"&+yYou decide to &+Rfold&+y and retire this game session. The &+bm&+Ba&+Cg&+Wi&+Cc&+Ba&+bl &+bf&+Bl&+Cam&+Be&+bs&+y crawl over the cards and they all return to the &+wdeck&+y, taking whatever " \
	"&+Wcash&+y you bet as well!!&n\n"
#define STR_CARDS_BUST \
	"&+yYou &+RBUSTED!!&+y. The &+bm&+Ba&+Cg&+Wi&+Cc&+Ba&+bl &+bf&+Bl&+Cam&+Be&+bs&+y crawl over the cards and they all return to the &+wdeck&+y, taking whatever &+Wcash&+y you bet as well!!&n\n"
