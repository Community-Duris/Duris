#ifndef _RANDOBJ_H_
#define _RANDOBJ_H_

typedef enum _randObjType
{
	mundaneObj = 0,
	magicObj,
	rareObj,
	uniqueObj,
	setObj
} randObjType;

typedef enum _randObjAffType
{
	roatAffBit1 = 0,
	roatAffBit2,
	roatAffBit3,
	roatAffBit4,
	roatAff,
	roatExtra1,
	roatExtra2
} randObjAffType;

typedef struct _randObjAff
{
	randObjAffType affType;

	unsigned int where;

	unsigned int cost;
	unsigned int incCost;
	int          negGood; // boolean

	unsigned int restrictedLoc; // uses item_wear flags

	int rareOnly; // boolean

	const char *name, *name2;
} randObjAff;

#endif
