#ifndef DURIS_TRANSPORT_H
#define DURIS_TRANSPORT_H
#include "core/structs.h"
#include <string>

#ifdef DURIS_PERSISTENT_TRANSPORT
bool transport_frontend_active();
bool transport_world_active();
bool transport_frontend_ready();
int transport_frontend_main(int argc, char **argv, int port, int tls_port);
bool transport_world_configure();
bool transport_world_boot(bool restoring);
void transport_world_ready();
void transport_world_pump();
void transport_world_finish_pulse();
bool transport_world_quiesce();
bool transport_world_commit(const char *path);
void transport_world_abort();
bool transport_world_verify_file(const char *path);
bool transport_restore_descriptor(P_desc d, const char *name);
bool transport_restore_identity(P_desc d);
bool transport_restore_gameplay(P_desc d);
bool transport_descriptor_eligible(P_desc d);
bool transport_descriptor_tls(P_desc d);
bool transport_frontend_input(P_desc d, unsigned kind, const char *data, size_t size);
void transport_frontend_metadata(P_desc d);
void transport_frontend_accepted(P_desc d);
void transport_descriptor_closed(P_desc d);
int transport_world_output(P_desc d, unsigned kind, const void *data, size_t size);
#else
inline bool transport_frontend_active()
{
	return false;
}
inline bool transport_world_active()
{
	return false;
}
inline bool transport_frontend_ready()
{
	return false;
}
inline bool transport_world_configure()
{
	return true;
}
inline bool transport_world_boot(bool)
{
	return true;
}
inline void transport_world_ready() {}
inline void transport_world_pump() {}
inline void transport_world_finish_pulse() {}
inline bool transport_world_quiesce()
{
	return true;
}
inline bool transport_world_commit(const char *)
{
	return true;
}
inline void transport_world_abort() {}
inline bool transport_world_verify_file(const char *)
{
	return true;
}
inline bool transport_restore_descriptor(P_desc, const char *)
{
	return false;
}
inline bool transport_restore_identity(P_desc)
{
	return false;
}
inline bool transport_restore_gameplay(P_desc)
{
	return false;
}
inline bool transport_descriptor_eligible(P_desc)
{
	return false;
}
inline bool transport_descriptor_tls(P_desc)
{
	return false;
}
inline int transport_frontend_main(int, char **, int, int)
{
	return 1;
}
inline bool transport_frontend_input(P_desc, unsigned, const char *, size_t)
{
	return false;
}
inline void transport_frontend_metadata(P_desc) {}
inline void transport_frontend_accepted(P_desc) {}
inline void transport_descriptor_closed(P_desc) {}
inline int transport_world_output(P_desc, unsigned, const void *, size_t)
{
	return -1;
}
#endif

// The common line filter and greeting still run in the owning world thread.
void comm_transport_line(P_desc d, char *line);
void comm_transport_greet(P_desc d);
void websocket_dispatch_message(P_desc d, char *payload, size_t len);
#endif
