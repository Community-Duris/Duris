#ifndef ENCUMBRANCE_POLICY_H
#define ENCUMBRANCE_POLICY_H

inline int encumbrance_weight(int object_weight)
{
	return object_weight > 0 ? object_weight : 0;
}

#endif
