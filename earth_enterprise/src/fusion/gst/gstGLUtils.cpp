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


#include <gstGLUtils.h>
#include <notify.h>

void gstGLTexSpec::Init(GLint iformat, GLsizei width, GLsizei height,
                        GLenum format, GLenum type) {
  status_ = false;

  switch (iformat) {
    case GL_RGB:
    case GL_R3_G3_B2:
    case GL_RGB5_A1:
    case GL_RGBA4:
    case GL_LUMINANCE8:
    case GL_LUMINANCE16:
    case GL_LUMINANCE12_ALPHA12:
      internal_format_ = iformat;
      break;
    default:
      notify(NFY_WARN, "Unsupported internalformat type: %d", iformat);
      return;
  }

  if (width < 8 || width > 2048) {
    notify(NFY_WARN, "Unsupported texture width: %d", width);
    return;
  } else {
    width_ = width;
  }

  if (height < 8 || height > 2048) {
    notify(NFY_WARN, "Unsupported texture height: %d", height);
    return;
  } else {
    height_ = height;
  }

  switch (format) {
    case GL_RED:
    case GL_GREEN:
    case GL_BLUE:
    case GL_ALPHA:
    case GL_LUMINANCE:
      num_comps_ = 1;
      format_ = format;
      break;
    case GL_LUMINANCE_ALPHA:
      num_comps_ = 2;
      format_ = format;
      break;
    case GL_RGB:
      num_comps_ = 3;
      format_ = format;
      break;
    case GL_RGBA:
      num_comps_ = 4;
      format_ = format;
      break;
    default:
      notify(NFY_WARN, "Unsupported texture format: %d", format);
      return;
  }

  switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
      comp_size_ = 1;
      type_ = type;
      break;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
      comp_size_ = 2;
      type_ = type;
      break;
    case GL_INT:
    case GL_UNSIGNED_INT:
      comp_size_ = 3;
      type_ = type;
      break;
    case GL_FLOAT:
      comp_size_ = 4;
      type_ = type;
      break;
    default:
      notify(NFY_WARN, "Unsupported texture type: %d", type);
      return;
  }

  status_ = true;
}

