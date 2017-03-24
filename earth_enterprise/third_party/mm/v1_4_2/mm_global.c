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
**  mm_global.c -- Global Malloc-Replacement API
**
*/

#define MM_PRIVATE
#include "mm.h"

static MM *mm_global = NULL;

int MM_create(size_t size, const char *file)
{
    if (mm_global != NULL)
        return FALSE;
    if ((mm_global = mm_create(size, file)) == NULL)
        return FALSE;
    return TRUE;
}

int MM_permission(mode_t mode, uid_t owner, gid_t group)
{
    if (mm_global != NULL)
        return -1;
    return mm_permission(mm_global, mode, owner, group);
}

void MM_reset(void)
{
    if (mm_global == NULL)
        return;
    mm_reset(mm_global);
    return;
}

void MM_destroy(void)
{
    if (mm_global == NULL)
        return;
    mm_destroy(mm_global);
    mm_global = NULL;
    return;
}

int MM_lock(mm_lock_mode mode)
{
    if (mm_global == NULL)
        return FALSE;
    return mm_lock(mm_global, mode);
}

int MM_unlock(void)
{
    if (mm_global == NULL)
        return FALSE;
    return mm_unlock(mm_global);
}

void *MM_malloc(size_t size)
{
    if (mm_global == NULL)
        return NULL;
    return mm_malloc(mm_global, size);
}

void *MM_realloc(void *ptr, size_t size)
{
    if (mm_global == NULL)
        return NULL;
    return mm_realloc(mm_global, ptr, size);
}

void MM_free(void *ptr)
{
    if (mm_global == NULL)
        return;
    mm_free(mm_global, ptr);
    return;
}

void *MM_calloc(size_t number, size_t size)
{
    if (mm_global == NULL)
        return NULL;
    return mm_calloc(mm_global, number, size);
}

char *MM_strdup(const char *str)
{
    if (mm_global == NULL)
        return NULL;
    return mm_strdup(mm_global, str);
}

size_t MM_sizeof(const void *ptr)
{
    if (mm_global == NULL)
        return -1;
    return mm_sizeof(mm_global, ptr);
}

size_t MM_maxsize(void)
{
    return mm_maxsize();
}

size_t MM_available(void)
{
    if (mm_global == NULL)
        return -1;
    return mm_available(mm_global);
}

char *MM_error(void)
{
    return mm_lib_error_get();
}

/*EOF*/
