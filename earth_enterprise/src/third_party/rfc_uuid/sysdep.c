// Copyright 2008 Google Inc. All Rights Reserved.
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "copyrt.h"
#include "sysdep.h"
#include "../../third_party/rsa_md5/global.h"
#include "../../third_party/rsa_md5/md5.h"

const char* kNodeIDFile = "/tmp/gdl/u2";

/* system dependent call to get IEEE node ID.
   This sample implementation generates a random node ID. */
void get_ieee_node_identifier(uuid_node_t *node)
{
    static int inited = 0;
    int read_bytes;
    static uuid_node_t saved_node;
    unsigned char seed[16];
    FILE *fp;

    if (!inited) {
      fp = fopen(kNodeIDFile, "rb");
      if (fp) {
        read_bytes = fread(&saved_node, sizeof saved_node, 1, fp);
        fclose(fp);
        if (read_bytes == sizeof saved_node) {
          *node = saved_node;
          return;
        }
      }
      get_random_info(seed);
      seed[0] |= 0x01;
      memcpy(&saved_node, seed, sizeof saved_node);
      fp = fopen(kNodeIDFile, "wb");
      if (fp) {
        fwrite(&saved_node, sizeof saved_node, 1, fp);
        fclose(fp);
      }
      inited = 1;
    }

    *node = saved_node;
}

/* system dependent call to get the current system time. Returned as
   100ns ticks since UUID epoch, but resolution may be less than
   100ns. */
void get_system_time(uuid_time_t *uuid_time)
{
    struct timeval tp;

    gettimeofday(&tp, (struct timezone *)0);

    /* Offset between UUID formatted times and Unix formatted times.
       UUID UTC base time is October 15, 1582.
       Unix base time is January 1, 1970.*/
    *uuid_time = ((unsigned64)tp.tv_sec * 10000000)
        + ((unsigned64)tp.tv_usec * 10)
        + I64(0x01B21DD213814000);
}

/* Sample code, not for use in production; see RFC 1750 */
void get_random_info(unsigned char seed[16])
{
    MD5_CTX c;
    struct {
        struct sysinfo s;
        struct timeval t;
        char hostname[257];
    } r;
    memset(&r,0,sizeof r);
    MD5Init(&c);
    sysinfo(&r.s);
    gettimeofday(&r.t, (struct timezone *)0);
    gethostname(r.hostname, 256);
    MD5Update(&c, (unsigned char *)(&r), sizeof r);
    MD5Final(seed, &c);
}

