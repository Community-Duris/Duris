
#ifndef _SOJ_PROC_LIB_
#define _SOJ_PROC_LIB_

#ifdef SHLIB

typedef struct dynamic_procs
{
	const char *name;
	void       *handle;
} dynamic_procs;

extern dynamic_procs dynam_proc_list[];

int  load_proc_lib(char *);
int  unload_proc_lib(char *);
void load_all_proc_libs(void);
int  mob_proc_stub(P_char, P_char, int, char *);
int  obj_proc_stub(P_obj, P_char, int, char *);
int  room_proc_stub(int, P_char, int, char *);

/* prototypes for functions within a lib called automagically when the
   lib is loaded or unloaded */

void _init(void);
void _fini(void);

#endif /* SHLIB */
#endif /* _SOJ_PROC_LIB_ */
