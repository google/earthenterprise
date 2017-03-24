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

#ifndef KHSRC_FUSION_GST_GSTGLUTILS_H__
#define KHSRC_FUSION_GST_GSTGLUTILS_H__

#include <GL/gl.h>

class gstGLTexSpec {
 public:
  gstGLTexSpec(GLint i = GL_RGB, GLsizei w = 256, GLsizei h = 256,
               GLenum f = GL_RGB, GLenum t = GL_UNSIGNED_BYTE) {
    Init(i, w, h, f, t);
  }

  gstGLTexSpec(const gstGLTexSpec& that) {
    Init(that.InternalFormat(), that.Width(), that.Height(),
         that.Format(), that.Type());
  }

  void operator=(const gstGLTexSpec& that) {
    Init(that.InternalFormat(), that.Width(), that.Height(),
         that.Format(), that.Type());
  }

  GLint InternalFormat() const { return internal_format_; }
  GLsizei Width() const { return width_; }
  GLsizei Height() const { return height_; }
  GLenum Format() const { return format_; }
  GLenum Type() const { return type_; }

  int NumComps() const { return num_comps_; }
  int CompSize() const { return comp_size_; }

  int CellSize() const { return width_ * height_ * num_comps_ * comp_size_; }

  int Status() const { return status_; }

 private:
  void Init(GLint internalformat, GLsizei width, GLsizei height,
            GLenum format, GLenum type);

  GLint internal_format_;
  GLsizei width_;
  GLsizei height_;
  GLenum format_;
  GLenum type_;
  int num_comps_;
  int comp_size_;
  bool status_;
};

#endif  // !KHSRC_FUSION_GST_GSTGLUTILS_H__
