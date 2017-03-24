/*
**  mm_vers.c -- Version Information for OSSP mm (syntax: C/C++)
**  [automatically generated and maintained by GNU shtool]
*/

#ifdef _MM_VERS_C_AS_HEADER_

#ifndef _MM_VERS_C_
#define _MM_VERS_C_

#define MM_VERSION 0x104202

typedef struct {
    const int   v_hex;
    const char *v_short;
    const char *v_long;
    const char *v_tex;
    const char *v_gnu;
    const char *v_web;
    const char *v_sccs;
    const char *v_rcs;
} mm_version_t;

extern mm_version_t mm_version;

#endif /* _MM_VERS_C_ */

#else /* _MM_VERS_C_AS_HEADER_ */

#define _MM_VERS_C_AS_HEADER_
#include "mm_vers.c"
#undef  _MM_VERS_C_AS_HEADER_

mm_version_t mm_version = {
    0x104202,
    "1.4.2",
    "1.4.2 (15-Aug-2006)",
    "This is OSSP mm, Version 1.4.2 (15-Aug-2006)",
    "OSSP mm 1.4.2 (15-Aug-2006)",
    "OSSP mm/1.4.2",
    "@(#)OSSP mm 1.4.2 (15-Aug-2006)",
    "$Id: OSSP mm 1.4.2 (15-Aug-2006) $"
};

#endif /* _MM_VERS_C_AS_HEADER_ */

