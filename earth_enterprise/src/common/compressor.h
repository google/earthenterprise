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


#ifndef KHSRC_COMMON_COMPRESSOR_H__
#define KHSRC_COMMON_COMPRESSOR_H__

#include <string>
#include <setjmp.h>
#include <stdio.h>
#include <assert.h>
#include <khTypes.h>
#include <GL/gl.h>
#include <khGuard.h>
#include <third_party/rsa_md5/crc32.h>
#include <zlib.h>

extern "C" {
#include <jpeglib.h>
#undef HAVE_STDLIB_H
}


// ****************************************************************************
// ***  NOTE:
// ***
// ***  All compressors must leave at least kCRC32Size bytes on the end of the
// ***  buffer to be used for appending CRCs onto the packet
// ****************************************************************************


// Don't change the values in this enum, they are stored in pyramids indexes
// and raster ff indexes. The numbers match the old PYR_ #define's
enum CompressMode {
  CompressNone = 1,           // #define PYR_FORMAT_RAW  1
  CompressLZ   = 2,           // #define PYR_FORMAT_LZ   2
  CompressJPEG = 3,           // #define PYR_FORMAT_JPEG 3
  CompressDXT1 = 4,
  CompressPNG  = 5
};

inline std::string CompressModeName(CompressMode t) {
  switch (t) {
    case CompressNone:
      return "None";
    case CompressLZ:
      return "LZ";
    case CompressJPEG:
      return "JPEG";
    case CompressDXT1:
      return "DXT1";
    case CompressPNG:
      return "PNG";
  }
  return std::string(); // unreached but silences warnings
}

class CompressDef {
 public:
  CompressMode mode;
  int quality;

  CompressDef(CompressMode m, int q) : mode(m), quality(q) { }
  CompressDef(void) : mode(CompressNone), quality(0) { }
};


class Compressor {
 public:
  virtual ~Compressor() { }

  // inbuf = incoming uncompressed data
  // compressed data can be accessed by calling
  // the data() and dataLen() member functions
  // returns num bytes in result buffer, or 0 on error
  virtual uint32 compress(const char *inbuf) = 0;

  // inbuf = incoming compressed data
  // inbufSize = space used by incoming compressed data in bytes
  // outbuf = output buffer sufficiently sized to hold uncompressed data
  // returns num bytes in result buffer, or zero on error
  virtual uint32 decompress(char *inbuf, int inbufSize, char *outbuf) = 0;

  // pointer to compressed data, and it's size in bytes
  // used only by the compressor
  virtual char *data() = 0;
  virtual uint32 dataLen() = 0;
  virtual uint32 bufSize() = 0;

  virtual bool IsValid() { return true; }

  // Implement if the compressor can encode acquisition dates.
  // This needs to be called before pulling data() and dataLen().
  virtual void SetAcquisitionDate(const std::string& date) { }
};


//---------------------------------------------------------------------------
// JPEG
//---------------------------------------------------------------------------

struct ErrorMgr : public jpeg_error_mgr {
  ErrorMgr();

  jmp_buf setjmpBuffer;
};

//---------------------------------------------------------------------------

struct WriteBuffer : public jpeg_destination_mgr {
  char* buf;
  size_t bufSize;
  static const size_t reserve_size = kCRC32Size;

  WriteBuffer(size_t bsz, char *pakbuf = NULL) : bufSize(bsz) {
    init_destination = initDestination;
    empty_output_buffer = emptyOutputBuffer;
    term_destination = termDestination;

    if (pakbuf == NULL) {
      buf = new char[bufSize + reserve_size];
      track_buf_mem_ = true;
    } else {
      buf = pakbuf;
      track_buf_mem_ = false;
    }
  }
  ~WriteBuffer() {
    if (track_buf_mem_)
      delete [] buf;
  }

  static void initDestination(j_compress_ptr cinfo) {
    WriteBuffer* dest = (WriteBuffer*)cinfo->dest;
    dest->next_output_byte = (JOCTET*)dest->buf;
    dest->free_in_buffer = dest->bufSize;
  }

  static boolean emptyOutputBuffer(j_compress_ptr cinfo) {
    assert(0);
    return false;
  }

  static void termDestination(j_compress_ptr cinfo) {
    // do nothing, data should be in the buffer
  }

 private:
  bool track_buf_mem_;
};

//---------------------------------------------------------------------------

struct ReadBuffer : public jpeg_source_mgr {
  size_t bufSize;
  JOCTET *buf;

  ReadBuffer(char *data, size_t bsz) : bufSize(bsz) {
    init_source = initSource;
    fill_input_buffer = fillInputBuffer;
    skip_input_data = skipInputData;
    resync_to_restart = jpeg_resync_to_restart;
    term_source = termSource;
    bytes_in_buffer = 0;
    next_input_byte = NULL;
    buf = (JOCTET*)data;
  }

  ~ReadBuffer() { ; }

  static void initSource(j_decompress_ptr cinfo) { ; }

  static boolean fillInputBuffer(j_decompress_ptr cinfo) {
    ReadBuffer* src = (ReadBuffer*)cinfo->src;
    src->next_input_byte = &src->buf[0];
    src->bytes_in_buffer = src->bufSize;
    return TRUE;
  }

  static void skipInputData(j_decompress_ptr cinfo, long nBytes) { ; }

  static void termSource(j_decompress_ptr cinfo) { ; }
};

//---------------------------------------------------------------------------

class JPEGCompressor : public Compressor {
 public:
  JPEGCompressor(uint32 w, uint32 h, uint c, int quality);
  virtual ~JPEGCompressor();

  virtual char* data() { return writer_->buf; }

  virtual uint32 dataLen() { return writer_->bufSize - writer_->free_in_buffer; }
  virtual uint32 bufSize() { return writer_->bufSize + writer_->reserve_size; }

  virtual uint32 compress(const char *inbuf);
  virtual uint32 decompress(char *inbuf, int inbufSize, char *outbuf);

  virtual bool IsValid() { return valid_; }

  // after construction, returns size of a single image tile
  uint32 cellSize() { return width_ * height_ * comp_; }

  // JPEGCompressor can encode jpeg comments.
  // This needs to be called before pulling data() and dataLen().
  virtual void SetAcquisitionDate(const std::string& date);

  // Static utility to create a JPEG comment string containing the
  // acquisition date. This comment is specific to Google Earth.
  static std::string JPEGCommentAcquisitionDate(const std::string& date_string);

 private:
  bool init(void);
  int quality_;

  // construct the compression control structure
  jpeg_compress_struct cinfo_;
  WriteBuffer* writer_;
  std::string comment_string_;  // Optional comment to be prepended on the
                                // resulting compressed data.

  // construct the decompression control structure
  //jpeg_decompress_struct _dinfo;

  // init the error handler, used by both compress & decompress
  ErrorMgr error_mgr_;

  uint32 width_;
  uint32 height_;
  uint   comp_;

  bool valid_;
};


// custom compressor adapter to support mobile work
// this will minify the incoming buffer 4:1 before
// calling the supplied compressor
class MinifyCompressor : public Compressor {
 public:
  MinifyCompressor(Compressor* c, uint32 width, uint32 height, uint comp);
  virtual ~MinifyCompressor();

  virtual char* data() { return compressor_->data(); }

  virtual uint32 dataLen() { return compressor_->dataLen(); }
  virtual uint32 bufSize() { return compressor_->bufSize(); }

  virtual uint32 compress(const char *inbuf);
  virtual uint32 decompress(char *inbuf, int inbufSize, char *outbuf);

 private:
  Compressor* compressor_;
  uint32 width_;        // incoming width
  uint32 height_;       // incoming height
  uint comp_;           // number of components
  char* small_buffer_;  // space to minify
};

class DXT1Compressor : public Compressor {
 public:
  struct S3TC_CELL {
    GLushort c[2];    // interpolant endpoints (565)
    uint32    idx;    // 4x4x2-bits/pixel index array
  };

  struct Color8888 {
    GLubyte red;
    GLubyte green;
    GLubyte blue;
    GLubyte alpha;
  };

  struct Header {
    uint32 magic;
    uint32 checksum;
  };

  DXT1Compressor(GLenum f, GLenum t, int width, int height);
  virtual ~DXT1Compressor();

  virtual char* data() { return compress_buff; }

  // DXT1 has a fixed compression ratio, so these will always be the same
  virtual uint32 dataLen() { return sizeof(Header) + ((width * height) >> 1); }
  virtual uint32 bufSize() { return dataLen() + reserve_size; }

  virtual uint32 compress(const char* inbuf);
  virtual uint32 decompress(char* inbuf, int inbufSize, char* outbuf);

  static uint32 kMagic;

 private:
  GLenum format;
  GLenum type;
  int width;
  int height;

  void* GetTexelAddress(int texelindex, void* ptex);
  Color8888 TexelToColor8888(void* texel);

  static const size_t reserve_size = kCRC32Size;
  char* compress_buff;
};

//---------------------------------------------------------------------------

Compressor* NewLZCompressor(int level, uint32 uncompressed_buff_size);


// input_buf_width: 0 -> use width
//                : >0  -> input buffer is larger than target width
//                : <0  -> input buffer is larger and we're flipping Y axis
static const uint PNGOPT_BGR = 1;
static const uint PNGOPT_SWAPALPHA = 2;
Compressor* NewPNGCompressor(uint32 width, uint32 height, uint32 comp,
                             uint options = 0,
                             int32 input_buf_width = 0,
                             int compression_level = Z_DEFAULT_COMPRESSION);

#endif  // !KHSRC_COMMON_COMPRESSOR_H__
