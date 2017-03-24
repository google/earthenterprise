// Copyright 2008 Google Inc. All Rights Reserved.
#include <sys/types.h>
#include <sys/time.h>
#include <sys/sysinfo.h>

#include "copyrt.h"
#ifdef __cplusplus
extern "C" {
#endif
/* set the following to the number of 100ns ticks of the actual
   resolution of your system's clock */
#define UUIDS_PER_TICK 1024

/* Set the following to a calls to get and release a global lock */
#define LOCK
#define UNLOCK

typedef unsigned long long   unsigned64;
typedef unsigned long        unsigned32;
typedef unsigned short       unsigned16;
typedef unsigned char        unsigned8;
typedef unsigned char        byte;

/* Set this to what your compiler uses for 64-bit data type */
#define unsigned64_t unsigned long long
#define I64(C) C##LL

typedef unsigned64_t uuid_time_t;
typedef struct {
    char nodeID[6];
} uuid_node_t;

void get_ieee_node_identifier(uuid_node_t *node);
void get_system_time(uuid_time_t *uuid_time);
void get_random_info(unsigned char seed[16]);

#ifdef __cplusplus
}
#endif
