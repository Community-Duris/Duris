#ifndef _SPECIALIZATIONS_H_
#define _SPECIALIZATIONS_H_

#define GET_SPEC_NAME(cls, spc) (specdata[flag2idx(cls)][spc])

void do_specialize(P_char ch, char *argument, int cmd);
string single_spec_list(int race, int cls);
void unspecialize(P_char ch, P_obj obj);
bool is_allowed_race_spec(int race, uint m_class, int spec);
bool append_valid_specs(char *buf, P_char ch);

#endif
