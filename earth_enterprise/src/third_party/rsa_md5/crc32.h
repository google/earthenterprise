// Copyright 2006 Google Inc. All Rights Reserved.
// Author: mikegoss@google.com (Mike Goss)

#ifndef CRC32_H__
#define CRC32_H__

#include <stddef.h>
#include <khTypes.h>

// CRC-32 hash function used by Google Earth Enterprise Fusion (uses
// CRC class).

const size_t kCRC32Size = sizeof(uint32);

uint32 Crc32(const void *buffer, size_t buffer_len);

#endif  // CRC32_H__
