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
**  mm_test.c -- Test Suite for MM Library
**
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MM_PRIVATE
#include "mm.h"

#define FAILED_IF(expr) \
     if (expr) { \
         char *e; \
         e = mm_error(); \
         fprintf(stderr, "%s\n", e != NULL ? e : "Unknown Error"); \
         exit(1); \
     }

int main(int argc, char *argv[])
{
    unsigned char *core;
    int i;
    size_t s, s2;
    pid_t pid;
    int size;
    MM *mm;
    int n;
    char *cp[1025];
    int version;
    struct count_test { int count; int prev; } *ct;

    setbuf(stderr, NULL);

    /*
    **
    **  Test Global Library API
    **
    */

    fprintf(stderr, "\n*** TESTING GLOBAL LIBRARY API ***\n\n");

    fprintf(stderr, "Fetching library version\n");
    version = mm_lib_version();
    FAILED_IF(version == 0x0);
    fprintf(stderr, "version = 0x%X\n", version);

    /*
    **
    **  Test Low-Level Shared Memory API
    **
    */

    fprintf(stderr, "\n*** TESTING LOW-LEVEL SHARED MEMORY API ***\n");

    fprintf(stderr, "\n=== Testing Memory Segment Access ===\n");

    fprintf(stderr, "Creating 16KB shared memory core area\n");
    core = mm_core_create(1024*16, NULL);
    FAILED_IF(core == NULL);
    s = mm_core_size(core);
    FAILED_IF(s == 0);
    fprintf(stderr, "actually allocated core size = %d\n", s);

    fprintf(stderr, "Writing 0xF5 bytes to shared memory core area\n");
    for (i = 0; i < s; i++) {
        fprintf(stderr, "write to core[%06d]\r", i);
        core[i] = 0xF5;
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "Reading back 0xF5 bytes from shared memory core area\n");
    for (i = 0; i < s; i++) {
        fprintf(stderr, "read from core[%06d]\r", i);
        if (core[i] != 0xF5) {
            fprintf(stderr, "Offset %d: 0xF5 not found (found 0x%X\n", i, core[i]);
            exit(0);
        }
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "Destroying shared memory core area\n");
    mm_core_delete(core);

    fprintf(stderr, "\n=== Testing Memory Locking ===\n");

    fprintf(stderr, "Creating small shared memory core area\n");
    ct = mm_core_create(sizeof(struct count_test), NULL);
    FAILED_IF(ct == NULL);
    s = mm_core_size(ct);
    FAILED_IF(s == 0);
    fprintf(stderr, "actually allocated core size = %d\n", s);

    ct->prev  = 0;
    ct->count = 1;
    if ((pid = fork()) == 0) {
        /* child */
        while (ct->count < 32768) {
            if (!mm_core_lock(ct, MM_LOCK_RW)) {
                fprintf(stderr, "locking failed (child)\n");
                FAILED_IF(1);
            }
            if (ct->prev != (ct->count-1)) {
                fprintf(stderr, "Failed, prev=%d != count=%d\n", ct->prev, ct->count);
                exit(1);
            }
            ct->count += 1;
            fprintf(stderr, "count=%06d (child )\r", ct->count);
            ct->prev  += 1;
            if (!mm_core_unlock(ct)) {
                fprintf(stderr, "locking failed (child)\n");
                FAILED_IF(1);
            }
        }
        exit(0);
    }
    /* parent ... */
    while (ct->count < 32768) {
        if (!mm_core_lock(ct, MM_LOCK_RW)) {
            fprintf(stderr, "locking failed (parent)\n");
            FAILED_IF(1);
        }
        if (ct->prev != (ct->count-1)) {
            fprintf(stderr, "Failed, prev=%d != count=%d\n", ct->prev, ct->count);
            exit(1);
        }
        ct->count += 1;
        fprintf(stderr, "count=%06d (parent)\r", ct->count);
        ct->prev  += 1;
        if (!mm_core_unlock(ct)) {
            fprintf(stderr, "locking failed (parent)\n");
            kill(pid, SIGTERM);
            FAILED_IF(1);
        }
    }
    waitpid(pid, NULL, 0);
    fprintf(stderr, "\n");

    fprintf(stderr, "Destroying shared memory core area\n");
    mm_core_delete(ct);

    /*
    **
    **  Test Standard Malloc-style API
    **
    */

    fprintf(stderr, "\n*** TESTING STANDARD MALLOC-STYLE API ***\n");

    fprintf(stderr, "\n=== Testing Allocation ===\n");

    fprintf(stderr, "Creating MM object\n");
    size = mm_maxsize();
    if (size > 1024*1024*1)
        size = 1024*1024*1;
    mm = mm_create(size, NULL);
    FAILED_IF(mm == NULL)
    mm_display_info(mm);
    s = mm_available(mm);
    FAILED_IF(s == 0);
    fprintf(stderr, "actually available bytes = %d\n", s);

    fprintf(stderr, "Allocating areas inside MM\n");
    n = 0;
    for (i = 0; i < 1024; i++)
        cp[i] = NULL;
    for (i = 0; i < 1024; i++) {
        fprintf(stderr, "total=%09d allocated=%09d add=%06d\r", s, n, (i+1)*(i+1));
        s2 = mm_available(mm);
        if ((i+1)*(i+1) > s2) {
            cp[i] = mm_malloc(mm, (i+1)*(i+1));
            if (cp[i] != NULL) {
                fprintf(stderr, "\nExpected an out of memory situation. Hmmmmm\n");
                FAILED_IF(1);
            }
            break;
        }
        cp[i] = mm_malloc(mm, (i+1)*(i+1));
        n += (i+1)*(i+1);
        FAILED_IF(cp[i] == NULL)
        memset(cp[i], 0xF5, (i+1)*(i+1));
    }
    mm_display_info(mm);

    fprintf(stderr, "\n=== Testing Defragmentation ===\n");

    fprintf(stderr, "Fragmenting memory area by freeing some selected areas\n");
    for (i = 0; i < 1024; i++) {
        if (i % 2 == 0)
            continue;
        if (cp[i] != NULL)
            mm_free(mm, cp[i]);
        cp[i] = NULL;
    }
    mm_display_info(mm);

    fprintf(stderr, "Freeing all areas\n");
    for (i = 0; i < 1024; i++) {
        mm_free(mm, cp[i]);
    }
    mm_display_info(mm);

    fprintf(stderr, "Checking for memory leaks\n");
    s2 = mm_available(mm);
    if (s != s2) {
        fprintf(stderr, "Something is leaking, we've lost %d bytes\n", s - s2);
        FAILED_IF(1);
    }
    else {
        fprintf(stderr, "Fine, we have again %d bytes available\n", s2);
    }

    fprintf(stderr, "Destroying MM object\n");
    mm_destroy(mm);

    /******/

    fprintf(stderr, "\nOK - ALL TESTS SUCCESSFULLY PASSED.\n\n");
    exit(0);
}

