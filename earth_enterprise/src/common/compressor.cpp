// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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


#include <string.h>
#include <math.h>
#include <utility>
#include "compressor.h"
#include <common/khConstants.h>
#include <common/khStringUtils.h>

#if defined USING_LIBPNG16
#include <libpng16/png.h>
#elif defined USING_LIBPNG15
#include <libpng15/png.h>
#else
#include <libpng12/png.h>
#endif

// After introducing date comments into jpeg headers, our
// decompressor gives us the following warning, which we want to detect and
// filter.
const std::string kJpegDateExtraneousBytes =
  "Corrupt JPEG data: 27 extraneous bytes before marker 0xdb";

//---------------------------------------------------------------------------
// Compressor adapter to minify 4:1 any incoming data before calling
// standard compressor.
//---------------------------------------------------------------------------

MinifyCompressor::MinifyCompressor(Compressor* c, std::uint32_t w, std::uint32_t h, unsigned int comp)
    : compressor_(c), width_(w), height_(h), comp_(comp) {
  small_buffer_ = new char[(w * h * comp) / 16];
}

MinifyCompressor::~MinifyCompressor() {
  delete compressor_;
  delete [] small_buffer_;
}

 std::uint32_t MinifyCompressor::compress(const char *inbuf) {
  char* dest = small_buffer_;
  for (std::uint32_t row = 0; row < height_; row += 4) {
    for (std::uint32_t col = 0; col < width_; col += 4) {
      memcpy(dest, inbuf + (row * width_ * comp_) + (col * comp_), comp_);
      dest += comp_;
    }
  }
  return compressor_->compress(small_buffer_);
}

 std::uint32_t MinifyCompressor::decompress(char* inbuf, int inbufSize, char* outbuf) {
  assert(!"Currently decompress is unsupported.");
  return 0;
}


//------------------------------------------------------------------------------
// JPEG
//------------------------------------------------------------------------------

static void errorExit(j_common_ptr cinfo) {
  ErrorMgr* err = static_cast<ErrorMgr*>(cinfo->err);

  // Format the warning message.
  char buffer[JMSG_LENGTH_MAX];
  err->format_message(cinfo, buffer);

  fprintf(stderr, "%s\n", buffer);

  // Return control to the setjmp point.
  longjmp(err->setjmpBuffer, 1);
}

static void emitMessage(j_common_ptr cinfo, int msgLevel) {
  jpeg_error_mgr* err = cinfo->err;

  if (msgLevel > 0) {
    // No trace messages.
    return;
  }

  if (msgLevel < 0) {
    // It's a warning message. Since corrupt files may generate many
    // warnings, the policy implemented here is to show only the first one.
    err->num_warnings++;
    if (err->num_warnings > 1) return;
  }

  // Format the warning message.
  char buffer[JMSG_LENGTH_MAX];
  err->format_message(cinfo, buffer);

  // We will often get this warning which is due to the addition
  // of jpeg date comments to the headers of our jpeg files. Detect
  // and ignore.
  if (strcmp(buffer, kJpegDateExtraneousBytes.c_str()) != 0) {
    fprintf(stderr, "%s\n", buffer);
  }
}

ErrorMgr::ErrorMgr() {
  // Set up the normal JPEG error routines, then override error_exit.
  jpeg_std_error(this);
  error_exit = errorExit;
  emit_message = emitMessage;
}

//---------------------------------------------------------------------------

JPEGCompressor::JPEGCompressor(std::uint32_t w, std::uint32_t h,
                               unsigned int c, int quality)
  : quality_(quality), writer_(0), width_(w),
    height_(h), comp_(c), valid_(true) {
  // Setup the output destination.
  // Verified that this buffer is not enough for a tile of 256 x 1.
  std::uint32_t cell_size = cellSize();
  if (cell_size < 1000000) {
    cell_size += 256 * 3;
  }
  writer_ =  new WriteBuffer(cell_size);

  memset(&cinfo_, 0, sizeof(cinfo_));

  cinfo_.err = &error_mgr_;

  jpeg_create_compress(&cinfo_);

  if (setjmp(error_mgr_.setjmpBuffer)) {
    notify(NFY_WARN, "Failed to initialize JPG compressor");
    valid_ = false;
  }


  // setup the iamge parameters
  cinfo_.image_width = width_;
  cinfo_.image_height = height_;
  cinfo_.input_components = comp_;

  switch (comp_) {
    case 1 :
      cinfo_.in_color_space = JCS_GRAYSCALE;
      break;
    case 3 :
      cinfo_.in_color_space = JCS_RGB;
      break;
    default :
      notify(NFY_WARN, "Invalid number of components.  Must be 1 or 3 (not %d)", comp_);
      valid_ = false;
  }

  cinfo_.dest = writer_;

  jpeg_set_defaults(&cinfo_);

  jpeg_set_quality(&cinfo_, quality_, TRUE);
}

JPEGCompressor::~JPEGCompressor() {
  jpeg_destroy_compress(&cinfo_);
  delete writer_;
}

 std::uint32_t JPEGCompressor::compress(const char *inbuf) {
  // Initialize internal state of jpeg compressor.
  jpeg_start_compress(&cinfo_, TRUE);

  // Add JPEG comment tag, if available.
  if (!comment_string_.empty()) {
    jpeg_write_marker(&cinfo_, JPEG_COM, reinterpret_cast<const JOCTET *>(
        comment_string_.c_str()), comment_string_.size());
  }

  std::uint32_t row_stride = width_ * comp_;

  unsigned char *cdata = (unsigned char*)inbuf;
  while (cinfo_.next_scanline < height_) {
    int wcount = jpeg_write_scanlines(&cinfo_, &cdata, 1);
    cdata += wcount * row_stride;
  }

  // Cleanup compression.
  jpeg_finish_compress(&cinfo_);

  // Calculate the buffer write length.
  return dataLen();
}

 std::uint32_t JPEGCompressor::decompress(char *inbuf, int inbufSize, char *outbuf) {
  jpeg_decompress_struct dinfo;

  dinfo.err = &error_mgr_;

  jpeg_create_decompress(&dinfo);

  if (setjmp(error_mgr_.setjmpBuffer)) {
    jpeg_destroy_decompress(&dinfo);
    return 0;
  }

  ReadBuffer reader(inbuf, inbufSize);

  dinfo.src = &reader;

  jpeg_read_header(&dinfo, TRUE);

  if (dinfo.image_width != width_
      || dinfo.image_height != height_
      || dinfo.num_components != (int)comp_) {
    jpeg_destroy_decompress(&dinfo);
    return 0;
  }

  jpeg_start_decompress(&dinfo);

  std::uint32_t row_stride = width_ * comp_;
  unsigned char* cdata;
  std::uint32_t line = 0;
  while (dinfo.output_scanline < height_) {
    cdata = (unsigned char*)outbuf + (row_stride * (line++));
    if (jpeg_read_scanlines(&dinfo, &cdata, 1) != 1) {
      fprintf(stderr, "Fatal jpeg error!  Bad data, decompression aborted.");
      jpeg_destroy_decompress(&dinfo);
      return 0;
    }
  }

  jpeg_finish_decompress(&dinfo);

  jpeg_destroy_decompress(&dinfo);

  return cellSize(); // JPEGCompressor only supports 8bit data.
}

void JPEGCompressor::SetAcquisitionDate(const std::string& date) {
  // Don't add a comment for empty or unknown date.
  if (!date.empty() && date != kUnknownDate)
    comment_string_ = JPEGCommentAcquisitionDate(date);
}

std::string JPEGCompressor::JPEGCommentAcquisitionDate(
  const std::string& date_string) {
  std::string date_string_modified(date_string);
  ReplaceCharsInString(date_string_modified, "-", ":");
  static const std::uint32_t kBufferSize = 200;  // Many more than the comment length.
  char buffer[kBufferSize];
  snprintf(buffer, kBufferSize, "*#G0#*0*AD*%s*#0G#*",
           date_string_modified.c_str());
  return std::string(buffer);
}

// ****************************************************************************
// ***  LZCompressor
// ****************************************************************************

class LZCompressor : public Compressor {
 public:
  LZCompressor(int l, std::uint32_t sz);
  virtual ~LZCompressor();
  LZCompressor(const LZCompressor&) = delete;
  LZCompressor(LZCompressor&&) = delete;
  LZCompressor& operator=(const LZCompressor&) = delete;
  LZCompressor& operator=(LZCompressor&&) = delete;

  virtual char *data() { return buf; }
  virtual std::uint32_t dataLen() { return bufLen; }
  virtual std::uint32_t bufSize() { return buff_size_ + reserve_size; }

  virtual std::uint32_t compress(const char *inbuf);
  virtual std::uint32_t decompress(char *inbuf, int inbufSize, char *outbuf);

 private:
  const int level_;
  const std::uint32_t uncompressed_buff_size_;
  const std::uint32_t buff_size_;
  static const size_t reserve_size = kCRC32Size;
  char*  buf;
  std::uint32_t bufLen;
};

// external interface
Compressor* NewLZCompressor(int l, std::uint32_t sz) {
  return new LZCompressor(l, sz);
}

LZCompressor::LZCompressor(int l, std::uint32_t sz)
  : level_(l),
    uncompressed_buff_size_(sz),
    // zlib specs say buffer must be at least 0.1% larger than inSize
    // plus an additional 12 bytes. We'll makes it 1% bigger.
    buff_size_(std::uint32_t(ceil(1.01 * uncompressed_buff_size_)) + 12),
    buf(new char[buff_size_ + reserve_size]),
    bufLen(0) {
}

LZCompressor::~LZCompressor() {
  delete[] buf;
}

 std::uint32_t LZCompressor::compress(const char *inbuf) {
  unsigned long tmpLen = buff_size_;
  if (compress2((unsigned char*)buf, &tmpLen,
                (const unsigned char*)inbuf, uncompressed_buff_size_,
                level_) != Z_OK) {
    tmpLen = 0;
  }
  return (bufLen = tmpLen);
}

 std::uint32_t LZCompressor::decompress(char *inbuf, int inbufSize, char *outbuf) {
  unsigned long outbufLen = uncompressed_buff_size_;
  if (uncompress((unsigned char*)outbuf, &outbufLen,
                 (unsigned char*)inbuf, inbufSize) != Z_OK) {
    outbufLen = 0;
  }
  return outbufLen;
}


// ****************************************************************************
// ***  DXT1 Compressor
// ****************************************************************************

// DXT1 compression details:
//
// Source pixels are grouped into 4x4 blocks called texels.
// For each texel, the minimum and maximum color value is found
// and converted to 16 bits (565).
// Then each pixel gets 2 bits to linearly interpolate between
// this range.
//
// So a source 4x4 RGB pixel block takes 4*4*3 bytes, or 48 bytes and
// will be compressed to 2 16-bit end points (4 bytes) plus 2 bits
// per pixel (4 bytes) for a total of 8 bytes.
//
// The effective compression ratio becomes 6:1
//
// To compute the raw size required, figure 1/2 byte per pixel output
//
 std::uint32_t DXT1Compressor::kMagic = 0x44585431;

DXT1Compressor::DXT1Compressor(GLenum f, GLenum t, int w, int h)
  : format(f), type(t),
    width(w), height(h),
    compress_buff(NULL) {
}

DXT1Compressor::~DXT1Compressor() {
  delete [] compress_buff;
}

void *DXT1Compressor::GetTexelAddress(int texelindex, void *ptex) {
  void *result = NULL;

  switch (type) {

    case GL_UNSIGNED_BYTE:
      // depends on the format
      switch (format) {
        case GL_RGBA:
          result = &(((GLubyte *)(ptex))[4*texelindex]);
          break;
        case GL_RGB:
          result = &(((GLubyte *)(ptex))[3*texelindex]);
          break;
        case GL_LUMINANCE_ALPHA:
          result = &(((GLubyte *)(ptex))[2*texelindex]);
          break;
        case GL_LUMINANCE:
          result = &(((GLubyte *)(ptex))[1*texelindex]);
          break;
        case GL_ALPHA:
          result = &(((GLubyte *)(ptex))[1*texelindex]);
          break;
        default:
          notify(NFY_FATAL, "TexelToColor8888() -- unsupported format/type pair: %d %d",
                 format, type);
          break;
      }
      break;

      // All of these are 1 unsigned short per texel.
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
      result = &(((GLushort *)(ptex))[texelindex]);
      break;

    default:
      notify(NFY_FATAL, "GetTexelAddress() -- unsupported type: %d", type);
      break;
  }

  return result;
}

DXT1Compressor::Color8888 DXT1Compressor::TexelToColor8888(void *texel) {
  Color8888 result = { 0, 0, 0, 0 };
  GLubyte  *ubptr = (GLubyte *) texel;
  GLushort *usptr = (GLushort *) texel;

  switch (type) {
    case GL_UNSIGNED_BYTE:
      switch (format) {
        case GL_RGBA:
          result.red   = ubptr[0];
          result.green = ubptr[1];
          result.blue  = ubptr[2];
          result.alpha = ubptr[3];
          break;
        case GL_RGB:
          result.red   = ubptr[0];
          result.green = ubptr[1];
          result.blue  = ubptr[2];
          result.alpha = 255;
          break;
        case GL_LUMINANCE:
          result.red   = ubptr[0];
          result.green = ubptr[0];
          result.blue  = ubptr[0];
          result.alpha = 255;
          break;
        case GL_LUMINANCE_ALPHA:
          result.red   = ubptr[0];
          result.green = ubptr[0];
          result.blue  = ubptr[0];
          result.alpha = ubptr[1];
          break;
        case GL_ALPHA:
          result.red   = 0;
          result.green = 0;
          result.blue  = 0;
          result.alpha = ubptr[0];
          break;
        default:
          notify(NFY_FATAL, "TexelToColor8888() -- unsupported format/type pair: %d %d",
                 format, type);
          break;
      }
      break;

    case GL_UNSIGNED_SHORT_5_6_5:
      if (format != GL_RGB) {
        notify(NFY_FATAL, "TexelToColor8888() -- unsupported format/type pair: %d %d",
               format, type);
      }
      result.red   = (GLubyte) ((((usptr[0] & 0xF800) >> 11) & 0x0000001F) << 3);
      result.green = (GLubyte) ((((usptr[0] & 0x07E0) >>  5) & 0x0000003F) << 2);
      result.blue  = (GLubyte) ((((usptr[0] & 0x001F) >>  0) & 0x0000001F) << 3);
      result.alpha = 255;
      break;
    case GL_UNSIGNED_SHORT_4_4_4_4:
      if (format != GL_RGBA) {
        notify(NFY_FATAL, "TexelToColor8888() -- unsupported format/type pair: %d %d",
               format, type);
      }
      result.red   = (GLubyte) ((((usptr[0] & 0xF000) >> 12) & 0x0000000F) << 4);
      result.green = (GLubyte) ((((usptr[0] & 0x0F00) >>  8) & 0x0000000F) << 4);
      result.blue  = (GLubyte) ((((usptr[0] & 0x00F0) >>  4) & 0x0000000F) << 4);
      result.alpha = (GLubyte) ((((usptr[0] & 0x000F) >>  0) & 0x0000000F) << 4);
      break;
    case GL_UNSIGNED_SHORT_5_5_5_1:
      if (format != GL_RGBA) {
        notify(NFY_FATAL, "TexelToColor8888() -- unsupported format/type pair: %d %d",
               format, type);
      }
      result.red   = (GLubyte) ((((usptr[0] & 0xF800) >> 11) & 0x0000001F) << 3);
      result.green = (GLubyte) ((((usptr[0] & 0x07C0) >>  6) & 0x0000001F) << 3);
      result.blue  = (GLubyte) ((((usptr[0] & 0x003E) >>  1) & 0x0000001F) << 3);
      result.alpha = (GLubyte) (255 * (usptr[0] & 0x00000001));
      break;
    default:
      notify(NFY_FATAL, "TexelToColor8888() -- unsupported type: %d", type);
      break;
  }

  return result;
}

 std::uint32_t DXT1Compressor::compress(const char *src_buff) {
  int w_mask = width - 1;
  int h_mask = height - 1;

  // Ensure buffer has been allocated.
  if (compress_buff == NULL)
    compress_buff = new char[dataLen()+reserve_size];

  // Populate header.
  Header *hdr = (Header*)compress_buff;
  hdr->magic = kMagic;
  hdr->checksum = 0x00000000;         // TODO: Assemble a valid checksum.

  S3TC_CELL *ps3tc = (S3TC_CELL*)(compress_buff + sizeof( Header ));

  std::vector<char> flipbuff( width * height * 3 );

  // Remove this when source orientation is lower-left
  // flip image orientation
  // incoming orientation is upperleft, and opengl wants lowerleft.
  const int line_sz = width * 3;
  for ( int y = 0; y < height; ++y )
    memcpy( &flipbuff[0] + ( (height - 1 - y) * line_sz ),
            src_buff + ( y * line_sz ), line_sz );

  for (int y = 0; y < height; y += 4) {
    GLvoid *p[4];

    for ( int i = 0; i < 4; ++i ) {
      // Get handle to the first entry of the first row of the block.
      // Revert this when source orientation is lower-left.
      p[i] = GetTexelAddress( width * ((y + i) & h_mask), &flipbuff[0] );
      //p[i] = GetTexelAddress( width * ((y + i) & h_mask), src_buff );
    }

    for ( int x = 0; x < width; x += 4 ) {

      GLubyte xmin = 0, ymin = 0, xmax = 0, ymax = 0;
      GLushort l[4][4];
      GLushort lmin = 0, lmax = 0;
      GLushort lcut[3];
      Color8888 cmin, cmax;
      GLushort c[2];
      std::uint32_t idx = 0, flip = 0;
      GLboolean bHasAlpha = false;
      GLboolean bFirst = true;

      // find the min/max luma locations for the 4x4 cell
      for ( int cy = 0; cy < 4; ++cy ) {
        for ( int cx = 0; cx < 4; ++cx ) {
          GLvoid *texel = GetTexelAddress( (x + cx) & w_mask, p[cy] );
          Color8888 rgba = TexelToColor8888( texel );
          if ( rgba.alpha < 0x7f ) {
            bHasAlpha = true;
          } else {
            // Opaque
            // Calculate approximate luma value.
            l[cy][cx] =
              ((GLushort)rgba.red << 1) +
              ((GLushort)rgba.green << 2) +
              ((GLushort)rgba.green) +
              ((GLushort)rgba.blue);

            if ( bFirst ) {
              bFirst = FALSE;
              xmin = xmax = cx;
              ymin = ymax = cy;
              lmin = lmax = l[cy][cx];
            } else {
              if (lmin > l[cy][cx]) {
                lmin = l[cy][cx];
                xmin = cx;
                ymin = cy;
              }
              if (lmax < l[cy][cx]) {
                lmax = l[cy][cx];
                xmax = cx;
                ymax = cy;
              }
            }
          }
        }
      }

      // Pack RGB565 values for endpoints.
      GLvoid *texel = GetTexelAddress( (x + xmin) & w_mask, p[ymin] );
      cmin = TexelToColor8888( texel );
      texel = GetTexelAddress( (x + xmax) & w_mask, p[ymax] );
      cmax = TexelToColor8888( texel );
      c[0] = ((cmin.red >> 3) << 11) | ((cmin.green >> 2) << 5) | (cmin.blue >> 3);
      c[1] = ((cmax.red >> 3) << 11) | ((cmax.green >> 2) << 5) | (cmax.blue >> 3);

      if ( bHasAlpha || ( c[0] == c[1] ) ) {
        // Encode numerically smaller endpoint first (c0 <= c1 implies alpha).
        if ( c[1] < c[0] ) {
          GLushort t = c[0]; c[0] = c[1]; c[1] = t;
          flip = 1;
        }

        // Decision points at 1/3 and 2/3.
        lcut[0] = (2 * lmin + lmax + 1) / 3;
        lcut[1] = (lmin + 2 * lmax + 1) / 3;

        // Generate the index bits.
        for ( int cy = 0; cy < 4; ++cy ) {
          for ( int cx = 0; cx < 4; ++cx ) {
            std::uint32_t code;
            GLvoid *texel = GetTexelAddress( (x + cx) & w_mask, p[cy] );
            if ( TexelToColor8888( texel ).alpha < 0x7f)    code = 3;
            else if ( l[cy][cx] <= lcut[0])                 code = 0 ^ flip;
            else if ( l[cy][cx] <= lcut[1])                 code = 2;
            else                                            code = 1 ^ flip;

            // Build from the end of the codeword down; 0,0 should be in lsbs.
            idx = (idx >> 2) | (code << 30);
          }
        }
      } else {
        // Opaque
        // Encode numerically larger endpoint first (c0 > c1 implies no alpha).
        if ( c[0] < c[1] ) {
          GLushort t = c[0]; c[0] = c[1]; c[1] = t;
          flip = 1;
        }

        // Decision points at 1/4, 1/2 and 3/4.
        lcut[0] = (3 * lmin + lmax + 2) / 4;
        lcut[1] = (lmin + lmax) / 2;
        lcut[2] = (lmin + 3 * lmax + 2) / 4;

        // Generate the index bits.
        for ( int cy = 0; cy < 4; ++cy ) {
          for ( int cx = 0; cx < 4; ++cx ) {
            std::uint32_t code;
            if      (l[cy][cx] <= lcut[0])  code = 0 ^ flip;
            else if (l[cy][cx] <= lcut[1])  code = 2 ^ flip;
            else if (l[cy][cx] <= lcut[2])  code = 3 ^ flip;
            else                            code = 1 ^ flip;

            // Build from the end of the codeword down; 0,0 should be in lsbs.
            idx = (idx >> 2) | (code << 30);
          }
        }
      }

      // Pack the output cell.
      ps3tc->c[0] = c[0];
      ps3tc->c[1] = c[1];
      ps3tc->idx = idx;
      // Advance to the next cell.
      ++ps3tc;
    }
  }

  return dataLen();
}

 std::uint32_t DXT1Compressor::decompress(char *inbuf, int inbufSize, char *outbuf) {
  std::uint32_t sz = width * height * 3;
  memset( outbuf, 0, sz );

  // skip past header
  // TODO: validate magic in header and checksum
  S3TC_CELL *s3tc_base = (S3TC_CELL*)(inbuf + sizeof( Header ));

  for ( int y = 0; y < height; y += 4 ) {

    // find addressess of output pixels
    GLvoid *p[4];
    for ( int i = 0; i < 4; ++i )
      p[i] = GetTexelAddress( ((width > 4) ? width : 4) * (y + i), outbuf );

    for ( int x = 0; x < width; x += 4 ) {
      S3TC_CELL *ps3tc = &s3tc_base[((y * width) >> 4) + (x >> 2)];
      Color8888 lut[4];
      GLuint idx = ps3tc->idx;

      // assemble 2-entry lut
      for ( int i = 0; i < 2; ++i ) {
        lut[i].red   = ((ps3tc->c[i] >> 11) & 0x1f) << 3; lut[i].red   |= lut[i].red   >> 5;
        lut[i].green = ((ps3tc->c[i] >>  5) & 0x3f) << 2; lut[i].green |= lut[i].green >> 6;
        lut[i].blue  = ((ps3tc->c[i] >>  0) & 0x1f) << 3; lut[i].blue  |= lut[i].blue  >> 5;
        lut[i].alpha = 0xff;
      }

      // check for alpha
      // c0 <= c1 implies alpha

      if ( ps3tc->c[0] > ps3tc->c[1] ) {
        // no alpha
        for ( int i = 2; i < 4; ++i ) {
          GLushort i0 = (4 - i) * 85;
          GLushort i1 = i0 ^ 255;
          lut[i].red    = (GLubyte)(((GLushort)lut[0].red   * i0 + (GLushort)lut[1].red   * i1) >> 8);
          lut[i].green  = (GLubyte)(((GLushort)lut[0].green * i0 + (GLushort)lut[1].green * i1) >> 8);
          lut[i].blue   = (GLubyte)(((GLushort)lut[0].blue  * i0 + (GLushort)lut[1].blue  * i1) >> 8);
          lut[i].alpha  = 0xff;
        }
      } else {
        // alpha
        lut[2].red    = (GLubyte)(((GLushort)lut[0].red   + (GLushort)lut[1].red)   >> 1);
        lut[2].green  = (GLubyte)(((GLushort)lut[0].green + (GLushort)lut[1].green) >> 1);
        lut[2].blue   = (GLubyte)(((GLushort)lut[0].blue  + (GLushort)lut[1].blue)  >> 1);
        lut[2].alpha  = 0xff;

        lut[3].red    = 0;
        lut[3].green  = 0;
        lut[3].blue   = 0;
        lut[3].alpha  = 0;
      }

      // unpack the index bits
      for ( GLubyte cy = 0; cy < 4; ++cy ) {
        for ( GLubyte cx = 0; cx < 4; ++cx ) {
          GLubyte *utexel = (GLubyte *)GetTexelAddress( x + cx, p[cy] );
          utexel[0] = lut[idx & 0x3].red;
          utexel[1] = lut[idx & 0x3].green;
          utexel[2] = lut[idx & 0x3].blue;
          // don't support output alpha
          idx >>= 2;
        }
      }
    }
  }

  return sz;
}

//------------------------------------------------------------------------------
// PNG
//------------------------------------------------------------------------------

namespace {

struct PngInputStream {
  PngInputStream(const char* const _data, size_t _length)
      : data(_data), length(_length), pos(0) {
  }
  const char* const data;
  size_t length;
  size_t pos;
};

void PngReadFunc(png_structp png_ptr, png_bytep data, png_size_t size) {
  PngInputStream* png_input_stream =
      static_cast<PngInputStream*>(png_get_io_ptr(png_ptr));
  if (png_input_stream->pos + size > png_input_stream->length) {
    // Not enough bytes, let's just clear the buffer.
    // This is not going to work, but at least we get defined behavior,
    // and don't expose any uninitialized bytes.
    memset(data, 0, size);
    // Make all consequent reads fail, too.
    png_input_stream->pos = png_input_stream->length;
  } else {
    memcpy(data, &png_input_stream->data[png_input_stream->pos], size);
    png_input_stream->pos += size;
  }
}

class PngReader {
 public:
  PngReader() : png_ptr_(NULL), info_ptr_(NULL), row_pointers_(NULL) {}

  ~PngReader() {
    Clean();
  }

  // Reads PNG from input buffer to PNG internal structures.
  // Returns 0 - success, -1 - Error.
  std::int32_t Read(const char* const inbuf, int inbufSize);

  // Cleans up internal data structures.
  inline void Clean() {
    png_destroy_read_struct(&png_ptr_, &info_ptr_, NULL);
  }

  inline std::uint32_t width() const {
    return png_get_image_width(png_ptr_, info_ptr_);
  }
  inline std::uint32_t height() const  {
    return png_get_image_height(png_ptr_, info_ptr_);
  }
  inline std::uint32_t components() const  {
    return static_cast<std::uint32_t>(png_get_channels(png_ptr_, info_ptr_));
  }

  inline unsigned char color_type() const  {
    return png_get_color_type(png_ptr_, info_ptr_);
  }

  // Gets raster data from PNG internal structures.
  // Returns data length.
  std::uint32_t GetRasterData(char* outbuf) const;

 private:
  png_structp png_ptr_;
  png_infop info_ptr_;
  png_bytep *row_pointers_;
};

int PngReader::Read(const char* const inbuf, int inbufSize) {
  // Create a PNG read structure.
  // Note: The error handling functions are set to NULL - the default error
  // handlers to be used.
  png_ptr_ = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr_) {
    notify(NFY_WARN, "Unable to allocate PNG read struct.");
    return -1;  // Error.
  }

  // Create a png_info structure.
  info_ptr_ = png_create_info_struct(png_ptr_);
  if (!info_ptr_) {
    notify(NFY_WARN, "Unable to allocate PNG info struct for reading.");
    return -1;  // Error.
  }

  // When libpng encounters an error it expects to longjump back to our routine.
  // Therefore we call setjmp and pass our png_jmpbuf(png_ptr).
  if (setjmp(png_jmpbuf(png_ptr_))) {
    notify(NFY_WARN, "Unable to initialize PNG library for reading.");
    return -1;  // Error.
  }

  // Set read function.
  PngInputStream png_input_stream(inbuf, inbufSize);
  png_set_read_fn(png_ptr_, &png_input_stream, PngReadFunc);

  // Set png_transforms flags as follows:
  // PACKING - Expand 1, 2 and 4-bit samples to bytes.
  // STRIP_16 - Strip 16-bit samples to 8 bits.
  // EXPAND - expands palette images to rgb, grayscale to 8 bit images,
  //          tRNS chunks to alpha channel.
  // NOTE: We don't want to handle endian-ness here since we do not need to
  // unpack RGBA tuples.
  const unsigned int png_transforms =
      PNG_TRANSFORM_PACKING |
      PNG_TRANSFORM_EXPAND |
      PNG_TRANSFORM_STRIP_16;

  png_read_png(png_ptr_, info_ptr_, png_transforms, NULL);

  // Note: memory for row pointers is managed by PNG library.
  row_pointers_ = png_get_rows(png_ptr_, info_ptr_);
  return 0;  // Success.
}

 std::uint32_t PngReader::GetRasterData(char* outbuf) const {
  switch (components()) {
    case 1:  // Indexcolor or grayscale format.
      assert(color_type() == PNG_COLOR_TYPE_GRAY);
      if (color_type() != PNG_COLOR_TYPE_GRAY) {
        notify(NFY_WARN, "PNG reader: unsupported palette modes.");
        return 0;
      }
    case 2:  // Grayscale with alpha format.
    case 3:  // RGB format.
    case 4:  // RGBA format.
      {
        // Copy pixel's data to output buffer.
        // NOTE: We don't want to handle endian-ness here since we do not need
        // to unpack RGBA tuples.
        std::uint32_t row_width_in_bytes = width() * components();
        for (std::uint32_t i = 0; i < height(); ++i, outbuf += row_width_in_bytes) {
          memcpy(outbuf, row_pointers_[i], row_width_in_bytes);
        }
        return height()*row_width_in_bytes;
      }
    default:
      // Should not be reached.
      notify(NFY_WARN, "PNG reader: png buffer error.");
      return 0;
  }
}

}  // namespace

class PNGCompressor : public Compressor {
 public:
  PNGCompressor(std::uint32_t width, std::uint32_t height, std::uint32_t comp, unsigned int options,
                std::int32_t input_buf_width,
                int compression_level);
  virtual ~PNGCompressor();

  virtual char* data() { return (char*) &output_buff_.first[0]; }
  virtual std::uint32_t dataLen() { return output_buff_.second; }
  virtual std::uint32_t bufSize() { return dataLen() + reserve_size; }

  virtual std::uint32_t compress(const char* inbuf);
  virtual std::uint32_t decompress(char* inbuf, int inbufSize, char* outbuf);

  virtual bool IsValid() { return valid_; }

  static const size_t reserve_size = kCRC32Size;
 private:
  std::uint32_t width_;
  std::uint32_t height_;
  std::uint32_t comp_;
  const std::int32_t input_buf_width_;

  // don't use string::resize as that may give back memory i.e
  // 1. use string just as buffer, rather than new, as heap management is better
  // 2. reserve enough memory so that we don't need to reallocate ever
  // 3. write using memcpy and
  // 4. keep a separate std::uint32_t for size.
  std::pair<std::string, std::uint32_t> output_buff_;
  std::vector<const char*> row_pointers_;
  bool valid_;
  unsigned int options_;
  int compression_level_;
};

namespace {

// PNG writing support.
class PngWriteStruct {
 public:
  PngWriteStruct() : png_ptr(NULL), info_ptr(NULL) {}
  ~PngWriteStruct() {
    png_destroy_write_struct(&png_ptr, &info_ptr);
  }
  png_structp png_ptr;
  png_infop info_ptr;
};

void png_write_func(png_structp png_ptr,
                    png_byte* data,
                    png_size_t length) {
  std::pair<std::string, std::uint32_t>* buffer =
     static_cast<std::pair<std::string, std::uint32_t>*>(png_get_io_ptr(png_ptr));
  const std::string::size_type size = buffer->second;
  const std::string::size_type required =
      size + length + PNGCompressor::reserve_size;
  // Increment in chunks of original capacity if required, always allocate more
  // than required, considering future need.
  if (buffer->first.capacity() < required) {
    const std::string::size_type new_capacity =
        ((required/(buffer->first.capacity())) + 1) * buffer->first.capacity();
    const std::string backup(&(buffer->first[0]), size);
    buffer->first.reserve(new_capacity);
    memcpy(&(const_cast<char*>(buffer->first.c_str())[0]), &(backup[0]), size);
  }
  memcpy(&(const_cast<char*>(buffer->first.c_str())[size]), data, length);
  buffer->second += length;
}

void png_flush_func(png_structp png_ptr) {
  // do nothing.
}

}  // namespace

// external interface
Compressor* NewPNGCompressor(std::uint32_t w, std::uint32_t h, std::uint32_t c, unsigned int options,
                             std::int32_t input_buf_width,
                             int compression_level) {
  Compressor* png = new PNGCompressor(w, h, c, options, input_buf_width,
                                      compression_level);
  if (png->IsValid()) {
    return png;
  } else {
    delete png;
    return NULL;
  }
}

PNGCompressor::PNGCompressor(std::uint32_t w, std::uint32_t h, std::uint32_t c, unsigned int options,
                             std::int32_t input_buf_width, int compression_level)
  : width_(w), height_(h), comp_(c),
    input_buf_width_((input_buf_width == 0) ? width_ : input_buf_width),
    valid_(true), options_(options), compression_level_(compression_level) {
  if (comp_ != 1 && comp_ != 3 && comp_ != 4) {
    valid_ = false;
    return;
  }
  row_pointers_.resize(height_);
  // if w * h * c is not greater than 64 KB assume single pixel case and
  // reserve atleast 64 KB
  output_buff_.first.reserve(std::max(w * h * c, 64U * 1024));
  output_buff_.second = 0;
}

PNGCompressor::~PNGCompressor() {
}

 std::uint32_t PNGCompressor::compress(const char* inbuf) {
  PngWriteStruct png_write_struct;

  png_write_struct.png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                     NULL, NULL, NULL);
  if (!png_write_struct.png_ptr) {
    notify(NFY_WARN, "Unable to allocate PNG write struct.");
    return 0;
  }

  png_write_struct.info_ptr = png_create_info_struct(png_write_struct.png_ptr);
  if (!png_write_struct.info_ptr) {
    notify(NFY_WARN, "Unable to allocate PNG info struct for writing.");
    return 0;
  }

  if (setjmp(png_jmpbuf(png_write_struct.png_ptr))) {
    notify(NFY_WARN, "Unable to initialize PNG library for writing.");
    return 0;
  }

  // set the filter to be none to improve performance
  png_set_filter(png_write_struct.png_ptr,
                 PNG_FILTER_TYPE_DEFAULT,
                 PNG_FILTER_NONE);

  // set zlib compression level
  png_set_compression_level(png_write_struct.png_ptr, compression_level_);

  // set compression strategy
  png_set_compression_strategy(png_write_struct.png_ptr, Z_DEFAULT_STRATEGY);

  int color_type;
  if (comp_ == 1) {
    color_type = PNG_COLOR_TYPE_GRAY;
  } else if (comp_ == 3) {
    color_type = PNG_COLOR_TYPE_RGB;
  } else {
    color_type = PNG_COLOR_TYPE_RGB_ALPHA;
  }
  png_set_IHDR(png_write_struct.png_ptr,
               png_write_struct.info_ptr,
               width_, height_,
               8, color_type, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  if (options_ & PNGOPT_BGR) {
    png_set_bgr(png_write_struct.png_ptr);
  }
  if (options_ & PNGOPT_SWAPALPHA) {
    png_set_swap_alpha(png_write_struct.png_ptr);
  }

  output_buff_.second = 0;

  for (unsigned int row = 0; row < height_; ++row) {
    std::int32_t row_offset = row * input_buf_width_ * comp_;
    row_pointers_[row] = inbuf + row_offset;
  }

  png_set_write_fn(png_write_struct.png_ptr,
                   (void*)&output_buff_,
                   png_write_func,
                   png_flush_func);

  // write header
  png_write_info(png_write_struct.png_ptr, png_write_struct.info_ptr);

  // write bytes
  png_write_image(png_write_struct.png_ptr, (png_byte**)&row_pointers_[0]);

  // end write
  png_write_end(png_write_struct.png_ptr, NULL);

  return output_buff_.second;
}

 std::uint32_t PNGCompressor::decompress(char* inbuf, int inbufSize, char* outbuf) {
  PngReader png_reader;
  if (png_reader.Read(inbuf, inbufSize) != 0) {
    // Couldn't read PNG buffer - return.
    return 0;
  }

  if (png_reader.width() != width_ ||
      png_reader.height() != height_ ||
      png_reader.components() != comp_) {
    // Our assumption about image format is wrong - return.
    return 0;
  }

  return png_reader.GetRasterData(outbuf);
}
