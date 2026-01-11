#ifndef _MCCP_H_
#define _MCCP_H_

#define MCCP_NONE 0
#define MCCP_VER1 1
#define MCCP_VER2 2

/* size of output compression buffer - affects compression ratio */
#define COMPRESS_BUF_SIZE 1024

/* gzip like values 0,1..9, 0 - none, 1 - fastest, 9 - best */
#define COMPRESS_EFFICIENCY 5

/* Memory Footprint
 * 
 * from zlib manual: (note, you need to recompile LIBRARY with the parameters
 * explained below, not only duris itselt):
 * 
 * zlib's memory footprint can be specified fairly precisely. It is
 * larger for compression than for decompression, and the exact requirements
 * depend on how the library was compiled. 
 * 
 * The memory requirements for compression depend on two parameters,
 * windowBits and memLevel: 
 * 
 * deflate memory usage (bytes) = (1 << (windowBits+2)) + (1 << (memLevel+9))
 *
 * For the default values of 15 and 8, respectively, this is 256 KB.
 * Both windowBits and memLevel can be set to lower values at compile time
 * via the MAX_WBITS and MAX_MEM_LEVEL macros, but only at a cost in
 * compression efficiency. 
 */

/* count calloc and free made by zlib, there should be 5 buffers per stream */
extern int mccp_alloc;
extern int mccp_free;

void sga_negotiate(P_desc desc);
void send_ga(P_desc desc);

#endif /* _MCCP_H_ */
