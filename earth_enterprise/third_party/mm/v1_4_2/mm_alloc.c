/* ====================================================================
 * Copyright (c) 1999-2006 Ralf S. Engelschall <rse@engelschall.com>
 * Copyright (c) 1999-2006 The OSSP Project <http://www.ossp.org/>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by
 *     Ralf S. Engelschall <rse@engelschall.com>."
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by
 *     Ralf S. Engelschall <rse@engelschall.com>."
 *
 * THIS SOFTWARE IS PROVIDED BY RALF S. ENGELSCHALL ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL RALF S. ENGELSCHALL OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 */

/*
**
**  mm_alloc.c -- Standard Malloc-Style API
**
*/

#define MM_PRIVATE
#include "mm.h"

/*
 * Create a memory pool
 */
MM *mm_create(size_t usize, const char *file)
{
    MM *mm = NULL;
    void *core;
    size_t size;
    size_t maxsize;

    /* defaults */
    maxsize = mm_maxsize();
    if (usize == 0)
        usize = maxsize;
    if (usize > maxsize)
        usize = maxsize;
    if (usize < MM_ALLOC_MINSIZE)
        usize = MM_ALLOC_MINSIZE;

    /* determine size */
    size = usize+SIZEOF_mem_pool;

    /* get a shared memory area */
    if ((core = mm_core_create(size, file)) == NULL)
        return NULL;

    /* fill in the memory pool structure */
    mm = (MM *)core;
    mm->mp_size    = size;
    mm->mp_offset  = SIZEOF_mem_pool;

    /* first element of list of free chunks counts existing chunks */
    mm->mp_freechunks.mc_size      = 0; /* has to be 0 forever */
    mm->mp_freechunks.mc_usize     = 0; /* counts chunks */
    mm->mp_freechunks.mc_u.mc_next = NULL;

    return mm;
}

/*
 * Set permissions on memory pool's underlaying disk files
 */
int mm_permission(MM *mm, mode_t mode, uid_t owner, gid_t group)
{
    if (mm == NULL)
        return -1;
    return mm_core_permission((void *)mm, mode, owner, group);
}

/*
 * Reset a memory pool.
 */
void mm_reset(MM *mm)
{
    if (mm == NULL)
        return;
    mm->mp_offset = SIZEOF_mem_pool;
    mm->mp_freechunks.mc_usize = 0;
    mm->mp_freechunks.mc_u.mc_next = NULL;
    return;
}

/*
 * Destroy a memory pool
 */
void mm_destroy(MM *mm)
{
    if (mm == NULL)
        return;
    /* wipe out the whole area to be safe */
    memset(mm, 0, mm->mp_size);
    /* and delete the core area */
    (void)mm_core_delete((void *)mm);
    return;
}

/*
 * Lock a memory pool
 */
int mm_lock(MM *mm, mm_lock_mode mode)
{
    if (mm == NULL)
        return FALSE;
    return mm_core_lock((void *)mm, mode);
}

/*
 * Unlock a memory pool
 */
int mm_unlock(MM *mm)
{
    if (mm == NULL)
        return FALSE;
    return mm_core_unlock((void *)mm);
}

/*
 * Display debugging information
 */
void mm_display_info(MM *mm)
{
    mem_chunk *mc;
    int nFree;
    int nAlloc;
    int i;

    if (!mm_core_lock((void *)mm, MM_LOCK_RD))
        return;
    mc = &(mm->mp_freechunks);
    nFree   = 0;
    while (mc->mc_u.mc_next != NULL) {
        mc = mc->mc_u.mc_next;
        nFree += mc->mc_size;
    }
    nAlloc = mm->mp_offset-SIZEOF_mem_pool-nFree;

    fprintf(stderr, "Information for MM\n");
    fprintf(stderr, "    memory area     = 0x%lx - 0x%lx\n", (unsigned long)mm, (unsigned long)(mm+mm->mp_size));
    fprintf(stderr, "    memory size     = %d\n", mm->mp_size);
    fprintf(stderr, "    memory offset   = %d\n", mm->mp_offset);
    fprintf(stderr, "    bytes spare     = %d\n", mm->mp_size-mm->mp_offset);
    fprintf(stderr, "    bytes free      = %d (%d chunk%s)\n",
            nFree, mm->mp_freechunks.mc_usize,
            mm->mp_freechunks.mc_usize == 1 ? "" : "s");
    fprintf(stderr, "    bytes allocated = %d\n", nAlloc);

    fprintf(stderr, "    List of free chunks:\n");
    if (mm->mp_freechunks.mc_usize > 0) {
        mc = &(mm->mp_freechunks);
        i = 1;
        while (mc->mc_u.mc_next != NULL) {
            mc = mc->mc_u.mc_next;
            fprintf(stderr, "        chunk #%03d: 0x%lx-0x%lx (%d bytes)\n",
                    i++, (unsigned long)mc, (unsigned long)(mc+mc->mc_size), mc->mc_size);
        }
    }
    else {
        fprintf(stderr, "        <empty-list>\n");
    }
    mm_core_unlock((void *)mm);
    return;
}

/*
 * Insert a chunk to the list of free chunks. Algorithm used is:
 * Insert in sorted manner to the list and merge with previous
 * and/or next chunk when possible to form larger chunks out of
 * smaller ones.
 */
static void mm_insert_chunk(MM *mm, mem_chunk *mcInsert)
{
    mem_chunk *mc;
    mem_chunk *mcPrev;
    mem_chunk *mcPrevPrev;
    mem_chunk *mcNext;

    if (!mm_core_lock((void *)mm, MM_LOCK_RW))
        return;
    mc = &(mm->mp_freechunks);
    mcPrevPrev = mc;
    while (mc->mc_u.mc_next != NULL && (char *)(mc->mc_u.mc_next) < (char *)mcInsert) {
        mcPrevPrev = mc;
        mc = mc->mc_u.mc_next;
    }
    mcPrev = mc;
    mcNext = mc->mc_u.mc_next;
    if (mcPrev == mcInsert || mcNext == mcInsert) {
        mm_core_unlock((void *)mm);
        ERR(MM_ERR_ALLOC, "chunk of memory already in free list");
        return;
    }
    if ((char *)mcPrev+(mcPrev->mc_size) == (char *)mcInsert &&
        (mcNext != NULL && (char *)mcInsert+(mcInsert->mc_size) == (char *)mcNext)) {
        /* merge with previous and next chunk */
        mcPrev->mc_size += mcInsert->mc_size + mcNext->mc_size;
        mcPrev->mc_u.mc_next = mcNext->mc_u.mc_next;
        mm->mp_freechunks.mc_usize -= 1;
    }
    else if ((char *)mcPrev+(mcPrev->mc_size) == (char *)mcInsert &&
             (char *)mcInsert+(mcInsert->mc_size) == ((char *)mm + mm->mp_offset)) {
        /* merge with previous and spare block (to increase spare area) */
        mcPrevPrev->mc_u.mc_next = mcPrev->mc_u.mc_next;
        mm->mp_offset -= (mcInsert->mc_size + mcPrev->mc_size);
        mm->mp_freechunks.mc_usize -= 1;
    }
    else if ((char *)mcPrev+(mcPrev->mc_size) == (char *)mcInsert) {
        /* merge with previous chunk */
        mcPrev->mc_size += mcInsert->mc_size;
    }
    else if (mcNext != NULL && (char *)mcInsert+(mcInsert->mc_size) == (char *)mcNext) {
        /* merge with next chunk */
        mcInsert->mc_size += mcNext->mc_size;
        mcInsert->mc_u.mc_next = mcNext->mc_u.mc_next;
        mcPrev->mc_u.mc_next = mcInsert;
    }
    else if ((char *)mcInsert+(mcInsert->mc_size) == ((char *)mm + mm->mp_offset)) {
        /* merge with spare block (to increase spare area) */
        mm->mp_offset -= mcInsert->mc_size;
    }
    else {
        /* no merging possible, so insert as new chunk */
        mcInsert->mc_u.mc_next = mcNext;
        mcPrev->mc_u.mc_next = mcInsert;
        mm->mp_freechunks.mc_usize += 1;
    }
    mm_core_unlock((void *)mm);
    return;
}

/*
 * Retrieve a chunk from the list of free chunks.  Algorithm used
 * is: Search for minimal-sized chunk which is larger or equal
 * than the request size. But when the retrieved chunk is still a
 * lot larger than the requested size, split out the requested
 * size to not waste memory.
 */
static mem_chunk *mm_retrieve_chunk(MM *mm, size_t size)
{
    mem_chunk *mc;
    mem_chunk **pmcMin;
    mem_chunk *mcRes;
    size_t sMin;
    size_t s;

    if (size == 0)
        return NULL;
    if (mm->mp_freechunks.mc_usize == 0)
        return NULL;
    if (!mm_core_lock((void *)mm, MM_LOCK_RW))
        return NULL;

    /* find best-fitting chunk */
    pmcMin = NULL;
    sMin = mm->mp_size; /* initialize with maximum possible */
    mc = &(mm->mp_freechunks);
    while (mc->mc_u.mc_next != NULL) {
        s = mc->mc_u.mc_next->mc_size;
        if (s >= size && s < sMin) {
            pmcMin = &(mc->mc_u.mc_next);
            sMin = s;
            if (s == size)
                break;
        }
        mc = mc->mc_u.mc_next;
    }

    /* create result chunk */
    if (pmcMin == NULL)
        mcRes = NULL;
    else {
        mcRes = *pmcMin;
        if (mcRes->mc_size >= (size + min_of(2*size,128))) {
            /* split out in part */
            s = mcRes->mc_size - size;
            mcRes->mc_size = size;
            /* add back remaining chunk part as new chunk */
            mc = (mem_chunk *)((char *)mcRes + size);
            mc->mc_size = s;
            mc->mc_u.mc_next = mcRes->mc_u.mc_next;
            *pmcMin = mc;
        }
        else {
            /* split out as a whole */
            *pmcMin = mcRes->mc_u.mc_next;
            mm->mp_freechunks.mc_usize--;
        }
    }

    mm_core_unlock((void *)mm);
    return mcRes;
}

/*
 * Allocate a chunk of memory
 */
void *mm_malloc(MM *mm, size_t usize)
{
    mem_chunk *mc;
    size_t size;
    void *vp;

    if (mm == NULL || usize == 0)
        return NULL;
    size = mm_core_align2word(SIZEOF_mem_chunk+usize);
    if ((mc = mm_retrieve_chunk(mm, size)) != NULL) {
        mc->mc_usize = usize;
        return &(mc->mc_u.mc_base.mw_cp);
    }
    if (!mm_core_lock((void *)mm, MM_LOCK_RW))
        return NULL;
    if ((mm->mp_size - mm->mp_offset) < size) {
        mm_core_unlock((void *)mm);
        ERR(MM_ERR_ALLOC, "out of memory");
        errno = ENOMEM;
        return NULL;
    }
    mc = (mem_chunk *)((char *)mm + mm->mp_offset);
    mc->mc_size  = size;
    mc->mc_usize = usize;
    vp = (void *)&(mc->mc_u.mc_base.mw_cp);
    mm->mp_offset += size;
    mm_core_unlock((void *)mm);
    return vp;
}

/*
 * Reallocate a chunk of memory
 */
void *mm_realloc(MM *mm, void *ptr, size_t usize)
{
    size_t size;
    mem_chunk *mc;
    void *vp;

    if (mm == NULL || usize == 0)
        return NULL;
    if (ptr == NULL)
        return mm_malloc(mm, usize); /* POSIX.1 semantics */
    mc = (mem_chunk *)((char *)ptr - SIZEOF_mem_chunk);
    if (usize <= mc->mc_usize) {
        mc->mc_usize = usize;
        return ptr;
    }
    size = mm_core_align2word(SIZEOF_mem_chunk+usize);
    if (size <= mc->mc_size) {
        mc->mc_usize = usize;
        return ptr;
    }
    if ((vp = mm_malloc(mm, usize)) == NULL)
        return NULL;
    memcpy(vp, ptr, mc->mc_usize);
    mm_free(mm, ptr);
    return vp;
}

/*
 * Free a chunk of memory
 */
void mm_free(MM *mm, void *ptr)
{
    mem_chunk *mc;

    if (mm == NULL || ptr == NULL)
        return;
    mc = (mem_chunk *)((char *)ptr - SIZEOF_mem_chunk);
    mm_insert_chunk(mm, mc);
    return;
}

/*
 * Allocate and initialize a chunk of memory
 */
void *mm_calloc(MM *mm, size_t number, size_t usize)
{
    void *vp;

    if (mm == NULL || number*usize == 0)
        return NULL;
    if ((vp = mm_malloc(mm, number*usize)) == NULL)
        return NULL;
    memset(vp, 0, number*usize);
    return vp;
}

/*
 * Duplicate a string
 */
char *mm_strdup(MM *mm, const char *str)
{
    int n;
    void *vp;

    if (mm == NULL || str == NULL)
        return NULL;
    n = strlen(str);
    if ((vp = mm_malloc(mm, n+1)) == NULL)
        return NULL;
    memcpy(vp, str, n+1);
    return vp;
}

/*
 * Determine user size of a memory chunk
 */
size_t mm_sizeof(MM *mm, const void *ptr)
{
    mem_chunk *mc;

    if (mm == NULL || ptr == NULL)
        return -1;
    mc = (mem_chunk *)((char *)ptr - SIZEOF_mem_chunk);
    return mc->mc_usize;
}

/*
 * Determine maximum size of an allocateable memory pool
 */
size_t mm_maxsize(void)
{
    return (mm_core_maxsegsize()-SIZEOF_mem_pool);
}

/*
 * Determine available memory
 */
size_t mm_available(MM *mm)
{
    mem_chunk *mc;
    int nFree;

    if (!mm_core_lock((void *)mm, MM_LOCK_RD))
        return 0;
    nFree = mm->mp_size-mm->mp_offset;
    mc = &(mm->mp_freechunks);
    while (mc->mc_u.mc_next != NULL) {
        mc = mc->mc_u.mc_next;
        nFree += mc->mc_size;
    }
    mm_core_unlock((void *)mm);
    return nFree;
}

/*
 * Return last error string
 */
char *mm_error(void)
{
    return mm_lib_error_get();
}

/*EOF*/
