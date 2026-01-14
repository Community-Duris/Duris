#ifndef __AUCTION_HOUSES_H_
#define __AUCTION_HOUSES_H_

/*
 auction_houses.h
 -- torgal april 2006 (torgal@durismud.com)
*/

#include "files.h"
#include "db.h"
#include "structs.h"
#include <string>
#include <vector>
using namespace std;

void init_auction_houses();
void shutdown_auction_houses();
void auction_houses_activity();
int auction_house_room_proc(int room_num, P_char ch, int cmd, char *arg);
void new_ah_call(P_char ch, char *arg, int cmd);

string format_time(long);
bool finalize_auction(int auction_id, P_char to_ch);
bool insert_money_pickup(int pid, int money);

// builds item info text for web display. returns length written.
int build_obj_info_text(P_obj obj, char *buf, size_t bufsize);

bool auction_offer(P_char ch, char *arg);
bool auction_list(P_char ch, char *arg);
bool auction_info(P_char ch, char *arg);
bool auction_bid(P_char ch, char *arg);
bool auction_pickup(P_char ch, char *arg);
bool auction_help(P_char ch, char *arg);
bool auction_remove(P_char ch, char *arg);
bool auction_resort(P_char ch, char *arg);

class EqSortFlag
{

public:
  EqSortFlag(const char *_keyword, const char *_desc) : keyword(_keyword), desc(_desc) {}

  virtual bool match(P_obj obj) = 0;
  const char *gKey() { return keyword.c_str(); }
  const char *gDesc() { return desc.c_str(); }

  virtual ~EqSortFlag() = default;

protected:
  string keyword;
  string desc;
};

class EqSlotFlag : public EqSortFlag
{
public:
  EqSlotFlag(const char *_keyword, const char *_desc, unsigned int _eq_bit) : eq_bit(_eq_bit), EqSortFlag(_keyword, _desc)
  {
  }

  bool match(P_obj obj)
  {
    return (obj->wear_flags & eq_bit) ? true : false;
  }

  unsigned int eq_bit;
};

class EqClassFlag : public EqSortFlag
{
public:
  EqClassFlag(const char *_keyword, const char *_desc, unsigned _wear_class) : wear_class(_wear_class), EqSortFlag(_keyword, _desc)
  {
  }

  bool match(P_obj obj)
  {
    return IS_SET(obj->extra_flags, ITEM_ALLOWED_CLASSES) ? IS_SET(obj->anti_flags, wear_class) : !IS_SET(obj->anti_flags, wear_class);
  }

  unsigned wear_class;
};

class EqTypeFlag : public EqSortFlag
{
public:
  EqTypeFlag(const char *_keyword, const char *_desc, ::byte _type) : type(_type), EqSortFlag(_keyword, _desc)
  {
  }

  bool match(P_obj obj) { return (obj->type == type) ? true : false; }

  ::byte type;
};

class EqAffFlag : public EqSortFlag
{
public:
  EqAffFlag(const char *_keyword, const char *_desc, unsigned _bitv) : bitv(_bitv), EqSortFlag(_keyword, _desc) {}

  bool match(P_obj obj) { return IS_SET(obj->bitvector, bitv) ? TRUE : FALSE; }

protected:
  unsigned bitv;
};

class EqAff2Flag : public EqSortFlag
{
public:
  EqAff2Flag(const char *_keyword, const char *_desc, unsigned _bitv) : bitv(_bitv), EqSortFlag(_keyword, _desc) {}

  bool match(P_obj obj) { return IS_SET(obj->bitvector2, bitv) ? TRUE : FALSE; }

protected:
  unsigned bitv;
};

class EqAff3Flag : public EqSortFlag
{
public:
  EqAff3Flag(const char *_keyword, const char *_desc, unsigned _bitv) : bitv(_bitv), EqSortFlag(_keyword, _desc) {}

  bool match(P_obj obj) { return IS_SET(obj->bitvector3, bitv) ? TRUE : FALSE; }

protected:
  unsigned bitv;
};

class EqAff4Flag : public EqSortFlag
{
public:
  EqAff4Flag(const char *_keyword, const char *_desc, unsigned _bitv) : bitv(_bitv), EqSortFlag(_keyword, _desc) {}

  bool match(P_obj obj) { return IS_SET(obj->bitvector4, bitv) ? TRUE : FALSE; }

protected:
  unsigned bitv;
};

class EqAff5Flag : public EqSortFlag
{
public:
  EqAff5Flag(const char *_keyword, const char *_desc, unsigned _bitv) : bitv(_bitv), EqSortFlag(_keyword, _desc) {}

  bool match(P_obj obj) { return IS_SET(obj->bitvector5, bitv) ? TRUE : FALSE; }

protected:
  unsigned bitv;
};

class EqExtraFlag : public EqSortFlag
{
public:
  EqExtraFlag(const char *_keyword, const char *_desc, unsigned _extra) : extra(_extra), EqSortFlag(_keyword, _desc) {}

  bool match(P_obj obj) { return IS_SET(obj->extra_flags, extra) ? TRUE : FALSE; }

protected:
  unsigned extra;
};

class EqExtra2Flag : public EqSortFlag
{
public:
  EqExtra2Flag(const char *_keyword, const char *_desc, unsigned _extra2) : extra2(_extra2), EqSortFlag(_keyword, _desc) {}

  bool match(P_obj obj) { return IS_SET(obj->extra2_flags, extra2) ? TRUE : FALSE; }

protected:
  unsigned extra2;
};

class EqApplyFlag : public EqSortFlag
{
public:
  EqApplyFlag(const char *_keyword, const char *_desc, ::byte _apply) : apply(_apply), EqSortFlag(_keyword, _desc)
  {
  }

  bool match(P_obj obj)
  {
    for (int i = 0; i < MAX_OBJ_AFFECT; i++)
    {
      if ((obj->affected[i].location == apply) && (obj->affected[i].modifier != 0))
        return true;
    }
    return false;
  }

  ::byte apply;
};

class EqSort
{
public:
  EqSort();

  ~EqSort()
  {
    for (vector<EqSortFlag *>::iterator it = flags.begin(); it != flags.end(); it++)
    {
      if (*it)
      {
        delete *it;
        *it = NULL;
      }
    }
  }

  string getSortFlagsString(P_obj obj);

  string getDescString(const char *keyword);

  string getKeyword(const int num);
  int getSize() { return flags.size(); }

  bool isKeyword(const char *_keyword);

private:
  vector<EqSortFlag *> flags;
};

#endif // __AUCTION_HOUSES_H_
