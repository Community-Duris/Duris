#ifndef _TRANSPORT_H_
#define _TRANSPORT_H_

void initialize_transport();
int flying_transport(P_char ch, P_char victim, int cmd, char *arg);
void event_flying_transport_move(P_char ch, P_char victim, P_obj obj, void *data);
void event_flying_transport_return(P_char ch, P_char victim, P_obj obj, void *data);

#endif // _TRANSPORT_H_
