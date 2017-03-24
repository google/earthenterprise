/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */



#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_GEFILEUTILS_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_GEFILEUTILS_H_

#include <stdio.h>
#include <string>
#include "common/khFileUtils.h"


// ****************************************************************************
// ***  geLocalBufferWriteFile
// ***
// ***  This class supports creating a temp file on the local disk system and
// ***  then copying it to the intended path when it is destroyed. This class
// ***  can be used to circumvent problems with NFS, such as the inability to
// ***  memory map the file immediately after it is defined.  Use it only when
// ***  your open mode uses some file write operation. Otherwise simple open
// ***  may be better.
// ****************************************************************************
class geLocalBufferWriteFile {
  public:
    // Init file id
    geLocalBufferWriteFile();
    // If file is open, close it.
    ~geLocalBufferWriteFile(void);
    // Open the indicated file or a proxy for it with the given flags (RD/WR)
    // and mod permissions.
    // Throw exception if file is already open.
    int Open(const std::string& filePath, int flags, mode_t permissions);
    // Close file. If it is local tmp file, copy it to actual file location.
    void Close();
    // Returns file id (fd).
    // Throw exception if file is not open.
    int FileId() const;

  private:
    std::string filePath_;     // Path to actual file
    FILE* fp_;                 // Temporary file pointer
    int dst_fd_;               // File id of the actual destination file
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_GEFILEUTILS_H_
