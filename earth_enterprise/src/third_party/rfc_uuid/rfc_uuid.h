// Copyright 2008 Google Inc. All Rights Reserved.
#ifndef COMMON_THIRD_PARTY_RFC_UUID_H__
#define COMMON_THIRD_PARTY_RFC_UUID_H__

#include "copyrt.h"
#include "sysdep.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
  unsigned32  time_low;
  unsigned16  time_mid;
  unsigned16  time_hi_and_version;
  unsigned8   clock_seq_hi_and_reserved;
  unsigned8   clock_seq_low;
  byte        node[6];
} uuid_t;



/* uuid_create -- generate a UUID */
int uuid_create(uuid_t * uuid);


/* uuid_compare --  Compare two UUID's "lexically" and return
    -1   u1 is lexically before u2
     0   u1 is equal to u2
     1   u1 is lexically after u2
     Note that lexical ordering is not temporal ordering!
     */
int uuid_compare(uuid_t *u1, uuid_t *u2);

#ifdef __cplusplus
}
#endif
#endif  // COMMON_THIRD_PARTY_RFC_UUID_H__
