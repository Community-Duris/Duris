#ifndef _ALLIANCES_H_
#define _ALLIANCES_H_

#include <vector>
using namespace std;

extern vector<struct Alliance> alliances;

typedef struct Alliance *P_Alliance;

class Alliance
{
	friend void load_alliances();
	friend void save_alliances();

    public:
	Alliance(P_Guild forgers, P_Guild joiners, int tribute_owed);
	Alliance()
		: forging_assoc(NULL)
		, joining_assoc(NULL)
		, tribute_owed(0)
	{
	}
	P_Guild get_forgers() { return forging_assoc; }
	P_Guild get_joiners() { return joining_assoc; }
	bool is_allied_with(P_Guild ally);

    protected:
	P_Guild forging_assoc;
	P_Guild joining_assoc;
	int tribute_owed;
};

#define IS_FORGING_ASSOC(alliance, assoc_id) (alliance && alliance->forging_assoc_id == assoc_id)
#define IS_JOINING_ASSOC(alliance, assoc_id) (alliance && alliance->joining_assoc_id == assoc_id)

void load_alliances();
void save_alliances();

void do_acc(P_char ch, char *argument, int cmd);
void do_alliance(P_char ch, char *arg, int cmd);

void alliance_forge(P_char ch, char *arg, int cmd);
void alliance_sever(P_char ch, char *arg, int cmd);

void sever_alliance(P_Guild guild);

void send_to_alliance(char *str, P_Alliance alliance);

#endif // _ALLIANCES_H_
