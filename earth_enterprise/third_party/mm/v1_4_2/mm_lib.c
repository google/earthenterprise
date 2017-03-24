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
** mm_lib.c -- Internal Library API
**
*/

#define MM_PRIVATE
#include "mm.h"

/*
 * Error Handling
 */

#define MM_LIB_ERROR_MAXLEN 1024
static char mm_lib_error[MM_LIB_ERROR_MAXLEN+1] = { NUL };

void mm_lib_error_set(unsigned int type, const char *str)
{
    int l, n;
    char *cp;

    if (str == NULL) {
        mm_lib_error[0] = NUL;
        return;
    }
    if (type & MM_ERR_ALLOC)
        strcpy(mm_lib_error, "mm:alloc: ");
    else if (type & MM_ERR_CORE)
        strcpy(mm_lib_error, "mm:core: ");
    l = strlen(mm_lib_error);
    n = strlen(str);
    if (n > MM_LIB_ERROR_MAXLEN-l)
        n = MM_LIB_ERROR_MAXLEN-l;
    memcpy(mm_lib_error+l, str, n+1);
    l += n;
    if (type & MM_ERR_SYSTEM && errno != 0) {
        if (MM_LIB_ERROR_MAXLEN-l > 2) {
            strcpy(mm_lib_error+l, " (");
            l += 2;
        }
        cp = strerror(errno);
        n = strlen(cp);
        if (n > MM_LIB_ERROR_MAXLEN-l)
            n = MM_LIB_ERROR_MAXLEN-l;
        memcpy(mm_lib_error+l, cp, n+1);
        l += n;
        if (MM_LIB_ERROR_MAXLEN-l > 1) {
            strcpy(mm_lib_error+l, ")");
            l += 1;
        }
    }
    *(mm_lib_error+l) = NUL;
    return;
}

char *mm_lib_error_get(void)
{
    if (mm_lib_error[0] == NUL)
        return NULL;
    return mm_lib_error;
}

/*
 * Version Information
 */

#define _MM_VERS_C_AS_HEADER_
#include "mm_vers.c"
#undef  _MM_VERS_C_AS_HEADER_

int mm_lib_version(void)
{
    return MM_VERSION;
}

