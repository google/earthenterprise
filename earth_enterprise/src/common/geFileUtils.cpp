// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.



#include <fcntl.h>
#include <string>
#include "common/geFileUtils.h"
#include "common/khSimpleException.h"
#include "common/khException.h"
#include "common/khFileUtils.h"
#include "common/notify.h"


// ****************************************************************************
// ***  geLocalBufferWriteFile
// ****************************************************************************
geLocalBufferWriteFile::geLocalBufferWriteFile() : fp_(NULL) { }


// ****************************************************************************
// ***  Open
// ****************************************************************************
int geLocalBufferWriteFile::Open(
    const std::string& filePath, const int flags, const mode_t permissions) {

  // Open should not be called, if an existing file is already open.
  if (fp_) {
    throw khSimpleErrnoException(
        "Tmp file already opened, but requested opening ") << filePath;
  }

  // Make sure that the final destination file is openable.
  filePath_ = filePath;
  if (!khEnsureParentDir(filePath)) {
    throw khSimpleErrnoException("Unable to create directory for ") << filePath;
  }
  dst_fd_ = ::open(filePath_.c_str(), flags, permissions);
  if (dst_fd_ == -1) {
    throw khSimpleErrnoException("Unable to open file: \'") << filePath_
                                                            << "\'";
  }

  // Open the temp file.
  fp_ = tmpfile();
  if (!fp_) {
    throw khSimpleErrnoException("Unable to open tmp file.");
  }

  // Return fd for the temp file.
  return this->FileId();
}


// ****************************************************************************
// ***  Close
// ***  Close local tmp file, copy it to the real file location, and delete
// ***  the local tmp file.
// ****************************************************************************
void geLocalBufferWriteFile::Close() {
  if (fp_) {
    fflush(fp_);
    rewind(fp_);

    notify(NFY_NOTICE, "Copying created file: %s", filePath_.c_str());

    // Files are closed by the copy method.
    try {
      // File names are just for error messages in case of failure.
      khCopyOpenFile(this->FileId(), "tmp file", dst_fd_, filePath_.c_str());
    } catch(const std::exception &e) {
      // If we got an exception, try to close both src and dst files
      ::close(dst_fd_);
      fclose(fp_);
      throw(e);
    }
  } else {
    notify(NFY_WARN, "Close requested, but file not open.");
  }
}


// ****************************************************************************
// ***  FileId
// ****************************************************************************
int geLocalBufferWriteFile::FileId() const {
  if (!fp_) {
    throw khSimpleErrnoException(
        "Requested file id before opening file.");
  }

  return fileno(fp_);
}


// ****************************************************************************
// ***  geLocalBufferWriteFile destructor
// ****************************************************************************
geLocalBufferWriteFile::~geLocalBufferWriteFile() {
  // Don't generate warning if it was closed explicitly
  if (fp_) {
    notify(NFY_NOTICE, "Closing file %s.", filePath_.c_str());
    this->Close();
  }
}
