#ifndef __JMALLOC_H__
#define __JMALLOC_H__

/*
** malloc/free routines
** written by Jeff Tupper (tupper@cs.ubc.ca , i3130898@rick.cs.ubc.ca) 1991, 1992
** 4446 Lazelle Avenue, Terrace B.C., Canada, V8G 1R8
**
*/
/****************************************************************************
**
** Copyright 1991, 1992 by Jeff Tupper
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation, and that no fee is charged for any service or
** product of which this software or documentation are included in.
**
** For commercial use information, contact Jeff Tupper.
**
** Jeff Tupper makes no representations about the
** suitability of this software for any purpose.  It is provided "as is"
** without express or implied warranty.
**
*****************************************************************************
*/
/*
** note this package is not 100% compatible with all malloc packages
**
** differences:
**
** after a Free() the memory freed will be garbled before Free() returns
** (ie. Free() garbles the memory you pass to it)
**
** the MallocSize() call is not a standard malloc-package call. using it
** will force dependance upon this paticular package
**
** free(NULL) will be allowed, and will do nothing (besides waste CPU time)
**
** note:
**
** it may run with another malloc package (as long as the other malloc package
** can handle non-contiguous memory blocks)
*/
/*
***********************************************************************************
**
** External installation dependant parameters
**
***********************************************************************************
*/

#define malloc_pointer char *
#define malloc_size    unsigned
#define free_return    int
/*
** initializes Malloc package
** returns 0 on failure (sbrk() failed)
*/
static int MallocInit(void);

/*
** allocates a block of given size (perhaps more)
** and return a pointer to it, or NULL, if out-of-memory
*/
malloc_pointer malloc(malloc_size size);

/*
** free a previously Malloc()ed block
*/
void free(malloc_pointer block);

/*
** given a previously malloc()ed block, resize it to
** the requested size, or move it if necesarry
**
** if there is not enough memory to satisfy the request
** NULL is returned, and the given block is still available for use
** (free must be called to free the block)
*/
malloc_pointer realloc(malloc_pointer block, malloc_size size);

/*
** as Malloc(), but clears the memory with 0's before
** returning
*/
malloc_pointer calloc(malloc_size elem, malloc_size elsize);

/* an exact count of how many free bytes currently available */
extern long malloc_memory_available;

/*
** returns the size of the allocated block
** WARNING: may be larger than asked for
** WARNING: this is not portable - many malloc packages have no similar routine
** NOTICE: as used blocks are not currently merged, this should not have an accumulating
**         effect. future versions will most likely (not necessarily) be the same
*/
malloc_size MallocSize(malloc_pointer block);

/*
** Single function interface to all of the malloc
** statistics
*/

void MallocStatistics(int type);

/*
** 0: MallocStats
** 1: MallocDetails
** 2: MallocTreeDetails
** 3: MallocDetailStats
*/
#define MallocStatisticTypes 4 /** the # of different malloc statistics **/

typedef void (*MallocCleaner)(long);

/*
** hires a cleaner
**
** cleaner gets called with 0 if memory exhausted, before sbrk()
** cleaner gets called with 1 if sbrk() fails, or HARDLIMIT reached
** cleaner gets called with 2+ if any memory is still not available (phase X)
**
** every time around this loop the last phase (phase X) the argument to cleaner
** advances by one
**
** suggested actions for cleaner:
**
** argument: 0 - free memory you don't really need
**           1 - free all memory possible
**           2 - may opt for program quit, before bugs are exposed...
** around  100 - start saving stuff to disk, and discarding from memory
** around 1000 - time to quit, as system is probably thrashing due to paging
**               unless HARDLIMIT is set quite low
**   100000000 - may as well call cleanerfire()
**
** the later advice is program dependant (ie the numbers may be too large or too small)
*/
void MallocCleanerHire(MallocCleaner new);

/*
** fires a cleaner, so that the cleaner procedure will not get called again
*/
void MallocCleanerFire(MallocCleaner old);

/*
** resets the counter for clean(2+)
*/
void MallocCleanerWarningLevel(long level);

#endif
