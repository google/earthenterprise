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


#include "fusion/gemaptilegen/Generator.h"

#include <map>
#include <queue>
#include <string>
#include <vector>

#include "common/compressor.h"
#include "common/khAbortedException.h"
#include "common/khFileUtils.h"
#include "common/khStringUtils.h"
#include "common/khTimer.h"
#include "common/geColor.h"


namespace {
//#define USE_DEBUG // Uncomment this line to use the following debug function
#ifdef USE_DEBUG 
// To print a png file for debugging.
// Usage: WritePngFileDebug(&context.render_tile_.pixelBuf[0],
//          task_config_.fusion_tilespace_.tileSize, item->qt_path_.AsString())
void WritePngFileDebug(char* pixel_buffer, unsigned int tile_size,
                       const std::string& file_prefix) {
  unsigned int options = 0;
#if __BYTE_ORDER == __BIG_ENDIAN
  options = PNGOPT_SWAPALPHA;
#else
  options = PNGOPT_BGR;
#endif
  khDeleteGuard<Compressor> png_compressor(
      TransferOwnership(NewPNGCompressor(tile_size, tile_size, 4, options)));
  png_compressor->compress(pixel_buffer);
  khWriteSimpleFile(file_prefix + ".png",
                    png_compressor->data(), png_compressor->dataLen());
}
#endif
}  // namespace


namespace gemaptilegen {

// The rgb values has been pre-multiplied by alpha value and this function
// reverts that, so that rgb values are same as before pre-multiplication.
// Ideally for ARGB the process is:
//   r_new = r * 255 / alpha
//   g_new = g * 255 / alpha
//   b_new = b * 255 / alpha
// But the consideration for 'divide by 0', 'rounding error', as well as
// 'efficiency of calculation' gives the following lookup table based solution.
// Note :
//   Consumes 64 KB in lookup table.
//
//   We don't want to handle endian-ness here. The caller can use
//   /usr/include/endian.h and then compare #if __BYTE_ORDER == __LITTLE_ENDIAN
//   to feed the input
// Assumes that the chunk of 4 byte inputs will be in the following BGRA order
// for each 4 bytes: byte0 -> Blue, byte1 -> Green, byte2 -> Red, byte3-> Alpha
class AlphaMultiplicationUndoer {
 public:
  static void UndoAlphaPremultiplication(std::uint8_t* chunk);
  static inline void UndoAlphaPremultiplicationForPixel(std::uint8_t* bgra_color);

 private:
  AlphaMultiplicationUndoer() : lookup_table_(256, std::vector<std::uint8_t>(256, 0)) {
    // For alpha == 0, need to store 0, look up table is initialized to 0.
    // So nothing needs to be done in look up table, for alpha = 0
    const int MaxAlpha = 255;
    const int MaxRGB = 255;
    for (int alpha = 1; alpha < MaxAlpha; ++alpha) {
      for (int value = 0; value <= MaxRGB; ++value) {
        // alpha / 2 added as integer divisions are floor rather than round
        int un_premultiplied = (MaxRGB * value + alpha / 2) / alpha;
        // clamp the value to 255 as it cannot be more than that.
        if (un_premultiplied > MaxRGB) {
          un_premultiplied = MaxRGB;
        }
        lookup_table_[alpha][value] = (std::uint8_t)(un_premultiplied);
      }
    }
    // For alpha == 255, return value as it is
    for (int value = 0; value <= MaxRGB; ++value) {
      lookup_table_[MaxAlpha][value] = value;
    }
  }


  // In this 2 dimensional vector first dimension is indexed on alpha and 2nd
  // dimension is indexed on pot multiplied value. Content is original value
  // (before alpha multiplication).
  std::vector<std::vector<std::uint8_t> > lookup_table_;
  static const AlphaMultiplicationUndoer single_instance_for_lut;
};

const AlphaMultiplicationUndoer
    AlphaMultiplicationUndoer::single_instance_for_lut;

class CompressorJob {
 public:
  // Check whether all pixels (each pixel represented by 4 byte color for BGRA)
  // in a map tile are same. Since the maptile is in Fusion super tile,
  // need to increment SuperTileWidthInPxl() to move from one row to next row.
  static bool IsAllPixelsSame(char* tile_start_in_super_tile_buf) {
    // Since we know that tile keeps a set of 4 bytes (BGRA) it is faster to
    // compare in 4 byte chunks.
    const std::uint32_t* tile_buf32 = reinterpret_cast<const std::uint32_t*>(
        tile_start_in_super_tile_buf);
    const std::uint32_t* const tile_buf32_end = tile_buf32 +
                                         HeightInPxl() * SuperTileWidthInPxl();
    for (const std::uint32_t first_point_color = *tile_buf32;
         IsArrayOfIdenticalElements(tile_buf32, WidthInPxl()) &&
         (tile_buf32 += SuperTileWidthInPxl()) < tile_buf32_end;) {
      if (first_point_color != *tile_buf32) {
        break;
      }
    }

    if (!(tile_buf32 < tile_buf32_end)) {
      return true;
    }

    return false;
  }

  static void InitializeOnce(size_t width, size_t height,
                             size_t super_tile_width,
                             size_t super_tile_height) {
    static bool is_initialized = false;
    if (!is_initialized) {
      is_initialized = true;
      width_  = width;
      byte_width_  = width * 4;
      height_ = height;
      area_   = width * height;
      byte_size_ = area_ * 4;
      super_tile_width_  = super_tile_width;
      super_tile_byte_width_ = super_tile_width * 4;
      super_tile_height_ = super_tile_height;
      super_tile_area_   = super_tile_width * super_tile_height;
      super_tile_byte_size_ = super_tile_area_ * 4;
      num_tiles_per_super_tile_ = super_tile_area_ / area_;
    }
  }

  static size_t WidthInPxl()   { return width_; }
  static size_t WidthInBytes() { return byte_width_; }
  static size_t HeightInPxl()  { return height_;}
  static size_t AreaInPxl()    { return area_; }
  static size_t AreaInBytes()  { return byte_size_; }

  static size_t SuperTileWidthInPxl()   { return super_tile_width_;}
  static size_t SuperTileWidthInBytes() { return super_tile_byte_width_;}
  static size_t SuperTileHeightInPxl()  { return super_tile_height_;}
  static size_t SuperTileAreaInPxl()    { return super_tile_area_;}
  static size_t SuperTileAreaInBytes()  { return super_tile_byte_size_;}

  static size_t NumTilesPerSuperTile()  { return num_tiles_per_super_tile_;}

  static const unsigned int png_byte_order_option_;

 private:
  static size_t width_;
  static size_t byte_width_;
  static size_t height_;
  static size_t area_;
  static size_t byte_size_;
  static size_t super_tile_width_;
  static size_t super_tile_byte_width_;
  static size_t super_tile_height_;
  static size_t super_tile_area_;
  static size_t super_tile_byte_size_;
  static size_t num_tiles_per_super_tile_;

  CompressorJob(const CompressorJob& other);
  const CompressorJob& operator = (const CompressorJob& other);
};

const unsigned int CompressorJob::png_byte_order_option_ =
#if __BYTE_ORDER == __BIG_ENDIAN
    PNGOPT_SWAPALPHA;
#else
    PNGOPT_BGR;
#endif

size_t CompressorJob::width_;
size_t CompressorJob::byte_width_;
size_t CompressorJob::height_;
size_t CompressorJob::area_;
size_t CompressorJob::byte_size_;
size_t CompressorJob::super_tile_width_;
size_t CompressorJob::super_tile_byte_width_;
size_t CompressorJob::super_tile_height_;
size_t CompressorJob::super_tile_area_;
size_t CompressorJob::super_tile_byte_size_;
size_t CompressorJob::num_tiles_per_super_tile_;

void AlphaMultiplicationUndoer::UndoAlphaPremultiplication(std::uint8_t* chunk) {
  std::uint8_t* const end = chunk +
      CompressorJob::HeightInPxl() * CompressorJob::SuperTileWidthInBytes();
  for ( ; chunk < end; chunk += CompressorJob::SuperTileWidthInBytes() ) {
    for (std::uint8_t *four_byte_color = chunk,
         *chunk_end = chunk + CompressorJob::WidthInBytes();
         four_byte_color < chunk_end; four_byte_color += 4) {
      UndoAlphaPremultiplicationForPixel(four_byte_color);
    }
  }
}

void AlphaMultiplicationUndoer::UndoAlphaPremultiplicationForPixel(
    std::uint8_t *bgra_color) {
  const std::uint32_t alpha = bgra_color[3];
  // Optimize since most points are black or white
  if (alpha == 0 || alpha == 0xff) {
    // leave the color values as it is
    return;
  } else {
    const std::vector<std::uint8_t>& lookup_table_for_this_alpha =
        single_instance_for_lut.lookup_table_[alpha];
    *bgra_color = lookup_table_for_this_alpha[*bgra_color];  // B
    ++bgra_color;
    *bgra_color = lookup_table_for_this_alpha[*bgra_color];  // G
    ++bgra_color;
    *bgra_color = lookup_table_for_this_alpha[*bgra_color];  // R
  }
}


class CompressorOutput {
 public:
  // Constructor for a non-single-pixel tile.
  CompressorOutput(size_t serial, QuadtreePath qt_path, Compressor* compressor)
    : serial_(serial), qt_path_(qt_path), color_(0),
      data_len_(compressor->dataLen()),
      buf_size_(compressor->bufSize()) {
    buff_.reserve(buf_size_);
    memcpy(&(buff_[0]), compressor->data(), compressor->dataLen());
  }

  // Constructor for a single-pixel tile.
  CompressorOutput(size_t serial, QuadtreePath qt_path, const char color[4])
    : serial_(serial), qt_path_(qt_path),
      color_(*(reinterpret_cast<const std::uint32_t*>(color))),
      data_len_(0), buf_size_(0) {}

  // Constructor for a job to update serial_expected.
  CompressorOutput(size_t serial, size_t next_expected) : serial_(serial),
      qt_path_(), color_(0), data_len_(UINT_MAX), buf_size_(next_expected) {}

  // In case the current CompressorOutput is just to update serial returns the
  // new serial value to update to; Else returns 0
  size_t SerialChange() const {
    return data_len_ == UINT_MAX ? buf_size_ : 0;
  }

  const char* data() const { return &buff_[0]; }

  const size_t serial_;         // use this to serialize write on qt_path order
  const QuadtreePath qt_path_;  // For write bundle indexing
  // single color
  const std::uint32_t color_;
  const std::uint32_t data_len_;       // size of the data in the compressor,
                                // 0 for single pixel tile.
                                // UINT_MAX for serial updating job.
  const size_t buf_size_;       // size of the buffer including CRC bytes
                                // 0 for single pixel tile.
                                // The update serial for serial updating job.
 private:
  std::string buff_;            // buffer to copy PNG compressor output

  // disable copy operator
  void operator=(const CompressorOutput&);
  CompressorOutput(const CompressorOutput& that);
};

// This class wraps a CompressorOutput pointer. It implements the comparison
// operator which will be used in a priority queue, to serialize the
// Compressor Outputs in increasing order of the serial numbers.
class CompressorOutputPtr {
 public:
  explicit CompressorOutputPtr(const CompressorOutput* ptr) : ptr_(ptr) {}

  bool operator < (const CompressorOutputPtr& other) const {
    return other.ptr_->serial_ < ptr_->serial_;
  }

  operator const CompressorOutput*() const { return ptr_; }

  const CompressorOutput* ptr_;
};


// ****************************************************************************
// ***  RenderContext
// ****************************************************************************
RenderContext::RenderContext(const khTilespace &fusion_tilespace,
                             char* canvas_buff) :
    combiner_(fusion_tilespace),
    combine_tile_(),
    renderer_(false /* debug */),
    render_tile_(fusion_tilespace.tileSize, canvas_buff)
{ }


class RenderItem {
 public:
  explicit RenderItem(unsigned int max_in_tiles)
      : serial_(0), bytes_sent_to_write_(0) {
    in_tiles_.reserve(max_in_tiles);
  }

  QuadtreePath qt_path_;

  // NOTE: ownership of vector contents is explicitly not controlled by a smart
  // pointer. If an execption is thrown that would leak the vector,
  // we're going to exit anyway. And we don't want the MT overhead of managing
  // the shared pointers safely.
  // Additionally we cannot manager the lifetime in the destructor as this
  // object is stored in STL containers.
  std::vector<const maprender::Combiner::InTile*> in_tiles_;

  size_t serial_;  // Represents the serial number of this fusion super tile.
  size_t bytes_sent_to_write_;

  void Release(void) {
    for (unsigned int i = 0; i < in_tiles_.size(); ++i) {
      delete in_tiles_[i];
    }
    in_tiles_.clear();
  }
};


// ****************************************************************************
// ***  MergeSource to pull from FusePreparer::OutputQueue
// ****************************************************************************
class PreparerMergeSource : public MergeSource<Generator::MergeEntry> {
  typedef Generator::MergeEntry MergeEntry;
  typedef mttypes::BatchingQueuePuller<
      vectorprep::FusePreparer<MapDisplayRuleConfig>::OutTile> Puller;
  typedef Puller::PullHandle PullHandle;

 public:
  PreparerMergeSource(const std::string      &debug_desc,
                      Generator::Preparer    &preparer,
                      std::uint32_t                  input_batch_size,
                      int                     layer_no) :
      MergeSource<Generator::MergeEntry>(debug_desc),
      in_puller_(input_batch_size, preparer.OutputQueue()),
      in_return_semaphore_(preparer.OutputSemaphore()),
      curr_(),
      curr_entry_(vectorprep::FusePreparer<MapDisplayRuleConfig>::OutTile(),
                  layer_no) {
    // DO NOT pull a full batch, just pull one. We only need to seed the
    // merge source and don't want to wait for an entire batch
    in_puller_.ReleaseOldAndPull(curr_, 1);
    if (!curr_ || !curr_->Data()) {
      QString warn(kh::tr("No element for merge source for layer %1").arg(
          layer_no + 1));
      notify(NFY_WARN, "%s", warn.toUtf8().constData());
    } else {
      curr_entry_.first = curr_->Data();
    }
  }
  virtual const MergeEntry &Current(void) const {
    return curr_entry_;
  }
  virtual bool Advance(void) {
    if (curr_) {
      in_return_semaphore_.Release(1);
      in_puller_.ReleaseOldAndPull(curr_);
      if (curr_) {
        curr_entry_.first = curr_->Data();
        return true;
      }
    }
    return false;
  }
  virtual void Close(void) {
    // nothing to do
  }

 private:
  Puller                             in_puller_;
  mttypes::Semaphore                &in_return_semaphore_;
  PullHandle                         curr_;
  MergeEntry                         curr_entry_;

  DISALLOW_COPY_AND_ASSIGN(PreparerMergeSource);
};


// Just to aid CommandLoopForWriter as ptr_fun doesn't accept more than one
// argument
struct WriteArguments {
  Generator::WriteQueue* const write_packet_queue;
  PacketFileWriter* const writer;
  const std::uint32_t write_buffer_size;
  std::uint64_t* const write_count_ptr;
  mttypes::Semaphore* const write_buffer_control;

  WriteArguments(Generator::WriteQueue* write_packet_queue,
                 PacketFileWriter* writer,
                 std::uint32_t write_buffer_size,
                 std::uint64_t* write_count_ptr,
                 mttypes::Semaphore* write_buffer_control)
      : write_packet_queue(write_packet_queue),
        writer(writer),
        write_buffer_size(write_buffer_size),
        write_count_ptr(write_count_ptr),
        write_buffer_control(write_buffer_control) {}
};


// ****************************************************************************
// ***  Command thread loop for writer
// ****************************************************************************
void CommandLoopForWriter(const WriteArguments arg) {
  typedef std::map<std::uint32_t, PacketFileWriter::AllocatedBlock> LonleyPixelMap;
  typedef std::map<std::uint32_t, PacketFileWriter::AllocatedBlock>::iterator
      LonleyPixelMapIterator;
  LonleyPixelMap lonley_pixels;

  // Compressor for tiles having same non-transparent color for all pixels. We
  // create a single pixel png tile for this and browser renders it correctly.
  khDeleteGuard<Compressor> compressor(TransferOwnership((NewPNGCompressor(
                                    1,
                                    1,
                                    4, CompressorJob::png_byte_order_option_,
                                    // we are extracting the PNG tile directly
                                    // from our larger buffer.
                                    CompressorJob::SuperTileWidthInPxl(),
                                    0))));
  std::uint64_t write_count = 0;
  std::uint32_t bytes_written = 0;
  // This queue is to serialize writes based on quad tree path. This avoids the
  // need for costly resorting by file bundle writer.
  std::priority_queue<CompressorOutputPtr> backup_allocate_jobs;
  size_t serial_expected = 0;

  for (Generator::WriteQueue::PullHandle next;
       arg.write_packet_queue->ReleaseOldAndPull(next), next;) {
    const CompressorOutput* item = (const CompressorOutput*)(next->Data());
    assert(item);
    if (item->serial_ != serial_expected) {
      backup_allocate_jobs.push(CompressorOutputPtr(item));
      continue;
    }
    while (true) {
      size_t serial_change = item->SerialChange();
      if (serial_change != 0) {
        serial_expected = serial_change;
        delete item;
      } else {
        if (item->color_ != 0) {
          LonleyPixelMapIterator iter = lonley_pixels.find(item->color_);
          if (iter != lonley_pixels.end()) {
            arg.writer->WriteDuplicate(item->qt_path_, iter->second);
          } else {
            compressor->compress((const char *)&item->color_);
            LonleyPixelMapIterator iter = lonley_pixels.insert(
                LonleyPixelMap::value_type(
                    item->color_, arg.writer->AllocateAppend(
                       item->qt_path_, compressor->bufSize()))).first;
            arg.writer->WriteAtCRC(item->qt_path_,
                reinterpret_cast<void *>(compressor->data()),
                compressor->bufSize(),  /* with CRC space */
                iter->second);
          }
        } else {
          // Note that bufSize includes CRC space
          PacketFileWriter::AllocatedBlock allocation =
              arg.writer->AllocateAppend(item->qt_path_, item->buf_size_);
          arg.writer->WriteAtCRC(item->qt_path_,
              reinterpret_cast<void *>(const_cast<char*>(item->data())),
              item->buf_size_ /* includes CRC space */,
              allocation);
          bytes_written += item->buf_size_;
          if (bytes_written >= arg.write_buffer_size) {
            bytes_written -= arg.write_buffer_size;
            arg.write_buffer_control->Release(1);
          }
        }
        delete item;
        ++write_count;
        ++serial_expected;
      }
      if (backup_allocate_jobs.empty() ||
          (item = backup_allocate_jobs.top())->serial_ != serial_expected) {
        break;
      }
      backup_allocate_jobs.pop();
    }
  }
  const std::string bundle_file_name(arg.writer->abs_path() +
                                     FileBundleWriter::SegmentFileName(0));
  arg.writer->Close();
  // Make sure that in case no bundle file is generated, due to no feature at
  // this zoom level, atleast make an empty bundle file for publisher.
  if (!khExists(bundle_file_name)) {
#if defined(NDEBUG)
    khMakeEmptyFile(bundle_file_name);  // function issues NFY_WARNING
#else
    assert(khMakeEmptyFile(bundle_file_name));
#endif
  }
  *arg.write_count_ptr = write_count;
}

// Write a single transparent tile for the case when there's an empty layer.
// We still want a filebundle for further stages in the pipeline so that
// indexing and publishing assumptions are not violated (i.e., that there's a
// filebundle with something to index).
// We fill the filebundle with a single transparent tile.
// This is not multi-thread safe and is designed to be called only once
// by the process. The writer is closed on completion.
void WriteDummyTile(const WriteArguments arg) {
  // Here, we create the PNG compressor, compress a transparent tile image,
  // write it to a filebundle, close the writer and return.

  khDeleteGuard<Compressor> compressor(TransferOwnership((NewPNGCompressor(
                                    1,
                                    1,
                                    4, CompressorJob::png_byte_order_option_,
                                    // we are extracting the PNG tile directly
                                    // from our larger buffer.
                                    CompressorJob::SuperTileWidthInPxl(),
                                    0))));
  char transparent_color[] = { 0, 0, 0, 0 /* alpha=0 => transparent */};
  compressor->compress(transparent_color);
  // Place the transparent tile arbitrarily in the middle of the Pacific,
  // so it is rarely loaded.
  QuadtreePath qt_path(6, 31, 7);
  PacketFileWriter::AllocatedBlock allocated_block =
    arg.writer->AllocateAppend(qt_path, compressor->bufSize());
  arg.writer->WriteAtCRC(qt_path,
      reinterpret_cast<void *>(compressor->data()),
      compressor->bufSize(),  /* with CRC space */
      allocated_block);

  (*arg.write_count_ptr)++;
  arg.writer->Close();
}


void Generator::MergeLoop(mttypes::Semaphore* write_buffer_control) {
  // create and populate a Merger
  Merge<MergeEntry> merger("Prepare Merger");
  bool need_merging = false;
  for (unsigned int i = 0; i < sublayer_preparers_.size(); ++i) {
    PreparerMergeSource* ptr = new PreparerMergeSource(
                "Prepare Source",
                *sublayer_preparers_[i],
                task_config_.merge_source_input_batch_size_, i);
    if (ptr->Current().first) {
      merger.AddSource(TransferOwnership(ptr));
      need_merging = true;
    } else {
      delete ptr;
    }
  }


  if (need_merging) {
    merger.Start();
    std::uint32_t bytes_sent_to_write(0);
    size_t serial = 0;
    RenderQueue::PullHandle next;

    // bootstrap the first one
    free_render_item_queue_.ReleaseOldAndPull(next);
    assert(next);
    RenderItem* render_item = next->Data();
    render_item->serial_ = serial;
    ++serial;
    render_item->qt_path_ = merger.Current().first->GetPath();
    render_item->in_tiles_.push_back(&*merger.Current().first);

    while (merger.Advance()) {
      MergeEntry current = merger.Current();

      if (current.first->GetPath() != render_item->qt_path_) {
        // push the finised RenderItem off to the render Queue
        render_queue_.Push(render_item);

        // bootstrap the next one
        free_render_item_queue_.ReleaseOldAndPull(next);
      // Wait till bytes_sent_to_write decreases from max_bytes_sent_to_write_
      // to (max_bytes_sent_to_write - write_buffer_size_) i.e from 20M to 10M
        for (bytes_sent_to_write += render_item->bytes_sent_to_write_;
             bytes_sent_to_write > max_bytes_sent_to_write_;
             bytes_sent_to_write -= write_buffer_size_) {
          write_buffer_control->Acquire(1);
        }
        render_item->bytes_sent_to_write_ = 0;
        assert(next);
        render_item = next->Data();
        render_item->serial_ = serial;
        ++serial;
        render_item->qt_path_ = current.first->GetPath();
      }

      render_item->in_tiles_.push_back(&*current.first);
    }

    // push the finised RenderItem off to the render Queue
    render_queue_.Push(next->Data());
    merger.Close();
  }

  for (std::uint32_t i = 1; i < task_config_.num_render_threads_; ++i) {
    render_queue_.Push(NULL);  // A way to drain n-1 threads
  }
  render_queue_.PushDone();
}


// ****************************************************************************
// ***  RenderLoop
// ****************************************************************************
void Generator::RenderLoop(char* pixel_buffer) {
  khDeleteGuard<Compressor> compressor(TransferOwnership((NewPNGCompressor(
                                    CompressorJob::WidthInPxl(),
                                    CompressorJob::HeightInPxl(),
                                    4, CompressorJob::png_byte_order_option_,
                                    // we are extracting the PNG tile directly
                                    // from our larger buffer.
                                    CompressorJob::SuperTileWidthInPxl(),
                                    task_config_.png_compression_level_))));

  RenderQueue::PullHandle next;
  // this will block until something else is available
  for (; render_queue_.ReleaseOldAndPull(next), next;) {
    RenderItem* item = next->Data();
    if (item == NULL) {
      break;
    }
    bool not_blank = false;
    { // These created structures will be deleted as soon as we go out of scope.
      // All that will remain will be rendered output in pixel_buffer.
      RenderContext context(task_config_.fusion_tilespace_, pixel_buffer);
      // call maprender::Combiner
      ResetTile(&context.combine_tile_);
      context.combine_tile_.path = item->qt_path_;
      context.combiner_.Process(&context.combine_tile_, item->in_tiles_);

      // call maprender::Renderer to populate render_tile_
      context.render_tile_.path = item->qt_path_;
      not_blank = context.renderer_.Process(&context.render_tile_,
                                            context.combine_tile_);
    }

    size_t serial = item->serial_ * CompressorJob::NumTilesPerSuperTile();
    const size_t serial_at_beginning_of_tiling = serial;
    // If the tile has some feature to draw, i.e not totally blank
    if (not_blank) {
      // WritePngFileDebug(&context.render_tile_.pixelBuf[0],
      //                   super_tile_size_,
      //                   item->qt_path_.AsString());

      // figure out which target tiles we will make from this fusion tile
      std::uint32_t level, src_row, src_col;
      item->qt_path_.GetLevelRowCol(&level, &src_row, &src_col);

      khLevelCoverage todo_cov =
        TranslateLevelCoverage(task_config_.fusion_tilespace_,
                               khLevelCoverage(
                                   khTileAddr(level, src_row, src_col)),
                               is_mercator_ ? ClientMapMercatorTilespace
                                            : ClientMapFlatTilespace);

      // Get the origin and size before cropping. We'll need them to extract the
      // sub-tiles from our fusion tile
      khExtents<std::uint32_t> tileExtents = todo_cov.extents;

      // intersect the todo coverage with the target coverage
      todo_cov.cropTo(target_coverage_.extents);

      for (std::uint32_t tile_row = todo_cov.extents.beginRow();
           tile_row < todo_cov.extents.endRow(); ++tile_row) {
        std::uint32_t extract_tile_row = tile_row - tileExtents.beginRow();
        // our output rows are numbered bottom to top, but the tile is rendered
        // top to bottom
        extract_tile_row = tileExtents.height() - 1 - extract_tile_row;

        for (std::uint32_t tile_col = todo_cov.extents.beginCol();
             tile_col < todo_cov.extents.endCol(); ++tile_col) {
          std::uint32_t extract_tile_col = tile_col - tileExtents.beginCol();

          // determine the pixel offset of the extract tile in our bigger tile
          std::uint32_t offset = (extract_tile_row * CompressorJob::HeightInPxl()
                           * CompressorJob::SuperTileWidthInBytes()) +
                          extract_tile_col * CompressorJob::WidthInBytes();
          char* tile_buf = pixel_buffer + offset;
          const QuadtreePath out_path(todo_cov.level, tile_row, tile_col);
          bool is_single_color = CompressorJob::IsAllPixelsSame(tile_buf);
          CompressorOutput* compress_pack = NULL;
          if (is_single_color) {
            const unsigned char* ptr = reinterpret_cast<const unsigned char*>(tile_buf);
            const geColor color(geColor::BGRA, ptr[0], ptr[1], ptr[2], ptr[3]);
            if (color.IsTransparent()) {
              continue;  // transparent tile ignore
            }
            // Un-multiplies the alpha value because PNG is stored with alpha.
            // Process only first BGRA tuple that is then used to set color for
            // the whole tile.
            AlphaMultiplicationUndoer::UndoAlphaPremultiplicationForPixel(
                reinterpret_cast<std::uint8_t*>(tile_buf));
            compress_pack = new CompressorOutput(serial, out_path, tile_buf);
          } else {
            // Un-multiplies the alpha value because PNG is stored with alpha.
            AlphaMultiplicationUndoer::UndoAlphaPremultiplication(
                reinterpret_cast<std::uint8_t*>(tile_buf));
            compressor->compress(tile_buf);
            compress_pack = new CompressorOutput(serial, out_path, compressor);
            item->bytes_sent_to_write_ += compress_pack->buf_size_;
          }
          write_packet_queue_.Push(compress_pack);
          ++serial;
        }
      }
      if (serial_at_beginning_of_tiling != serial) {
        // some non transparent tile, so need cleaning for next run.
        memset(pixel_buffer, 0, CompressorJob::SuperTileAreaInBytes());
      }
    }
    size_t next_serial = serial_at_beginning_of_tiling +
                         CompressorJob::NumTilesPerSuperTile();
    // Put a serial increment job, symboling that we only gave a subset of
    // write jobs.
    if (serial != next_serial) {
      CompressorOutput* compress_pack = new CompressorOutput(serial,
                                                             next_serial);
      write_packet_queue_.Push(compress_pack);
    }
    // delete the memory for the in_tiles_ this has to be explicit due to
    // lifetime issues in the STL containers used to pass this along. But we
    // really don't have to worry about memory leaks if we throw an
    // exception. All applications that use this object treat exception as
    // fatal.
    item->Release();
    free_render_item_queue_.Push(item);
  }
  if (!next) {
    // we're all done, forward that knowledge to the compressor and return
    write_packet_queue_.PushDone();
  }
}

const size_t OneMegaByte = 1024 * 1024;
const size_t TenMegaByte = 10 * OneMegaByte;

Generator::Generator(geFilePool &file_pool, const std::string &outdir,
                     const Config &config, const TaskConfig &task_config,
                     bool is_mercator) :
    write_buffer_size_(TenMegaByte),
    index_buffer_size_(TenMegaByte),
    max_buff_size_to_suspend_others_(TenMegaByte * 4),
    max_bytes_sent_to_write_(max_buff_size_to_suspend_others_
                             - write_buffer_size_ - index_buffer_size_),
    writer_(file_pool, outdir),
    task_config_(task_config),
    target_coverage_(config.level_, khExtents<std::uint32_t>()),
    write_packet_queue_(),
    render_queue_(),
    free_render_item_queue_(),
    is_mercator_(is_mercator) {
  const khTilespace& client_map_tile_space =
      is_mercator_ ? ClientMapMercatorTilespace : ClientMapFlatTilespace;
  CompressorJob::InitializeOnce(
      client_map_tile_space.tileSize,           // width
      client_map_tile_space.tileSize,           // height
      task_config.fusion_tilespace_.tileSize,   // super tile w
      task_config.fusion_tilespace_.tileSize);  // super tile h

  AddWaitBase(&write_packet_queue_);
  AddWaitBase(&render_queue_);
  AddWaitBase(&free_render_item_queue_);

  writer_.BufferWrites(write_buffer_size_, index_buffer_size_);

  // Seed the free_render_item_queue_
  render_items_deleter_.reserve(task_config_.num_render_threads_ * 2);
  for (unsigned int i = 0; i < task_config_.num_render_threads_ * 2; ++i) {
    // Maximum in_tiles is same as number of sublayers (one in_tile from each)
    RenderItem* ptr = new RenderItem(config.sub_layers_.size());
    free_render_item_queue_.Push(ptr);
    render_items_deleter_.push_back(TransferOwnership(ptr));
  }

  // build the per-sublayer data structures
  // while we're at it, populate the target_coverage_ and fusion_coverage_
  sublayer_preparers_.reserve(config.sub_layers_.size());
  for (unsigned int sl = 0; sl < config.sub_layers_.size(); ++sl) {
    const gemaptilegen::Config::SubLayer& sublayer = config.sub_layers_[sl];

    std::vector<std::string> select_files;
    select_files.reserve(sublayer.display_rules_.size());
    std::vector<MapDisplayRuleConfig> display_rules;
    display_rules.reserve(sublayer.display_rules_.size());
    std::string messages;
    for (unsigned int dr = 0; dr < sublayer.display_rules_.size(); ++dr) {
      const gemaptilegen::Config::DisplayRule& disprule =
          sublayer.display_rules_[dr];
      {
        std::string message;
        if (!khExists(disprule.query_selectfile_)) {
          message = "doesn't exist.";
        } else if (khDiskUsage(disprule.query_selectfile_) == 0) {
          message = "is empty.";
        }
        if (!message.empty()) {
          QString warn(kh::tr("The file listing vector features to display "
                              "\"%1\" %2 ").arg(disprule.query_selectfile_.c_str(),
                                               message.c_str()));
          messages = messages + warn.toUtf8().constData();
          continue;
        }
      }
      select_files.push_back(disprule.query_selectfile_);
      display_rules.push_back(MapDisplayRuleConfig(
                                  disprule.name_,
                                  disprule.feature_,
                                  disprule.site_));
    }
    if (select_files.size() == 0) {
      if (!messages.empty()) {
        QString warn(kh::tr(
            "Please check the vector filter rules for layer %1.").arg(sl + 1));
        messages = messages + warn.toUtf8().constData();
        notify(NFY_WARN, "%s", messages.c_str());
      }
      continue;
    }

    // We need to oversize so that the icons/labels which span more than one
    // tile are rendered in both neighboring tiles. We don't expect these ones
    // to span more than a tile width in x (and not more than tile height in y).
    // Note that tile here means tile for browser which is 256 * 256 pixel.
    const double original_size = 8;   // The 2048 * 2048 tiles is 8 256 sub-tile
    const double extended_size = 10;  // We want 1 sub-tile oversize in each dir
    const double oversize_factor = (extended_size - original_size) /
                                   original_size;
    char prefix[128];
    snprintf(prefix, sizeof(prefix), "Layer %d:", sl + 1);
    sublayer_selectors_.push_back(
        TransferOwnership(
            new vectorquery::FuseSelector(
                config.level_,
                sublayer.vector_product_,
                select_files,
                task_config.fusion_tilespace_,
                oversize_factor,
                task_config_.selector_input_seed_size_,
                task_config_.selector_output_batch_size_,
                prefix)));
    AddWaitBase(&sublayer_selectors_.back()->InputQueue());
    AddWaitBase(&sublayer_selectors_.back()->OutputQueue());

    khLevelCoverage fusion_cov = sublayer_selectors_.back()->LevelCoverage();
    fusion_coverage_.extents.grow(fusion_cov.extents);

    khLevelCoverage target_cov = TranslateLevelCoverage(
        task_config.fusion_tilespace_,
        fusion_cov, client_map_tile_space);
    target_coverage_.extents.grow(target_cov.extents);

    // JS context scripts to use when doing field expansion
    QStringList js_context_scripts;
    if (!sublayer.context_script_.isEmpty()) {
      js_context_scripts.push_back(sublayer.context_script_);
    }

    sublayer_preparers_.push_back(
        TransferOwnership(new Preparer(
                              *sublayer_selectors_.back(),
                              display_rules,
                              js_context_scripts,
                              task_config_.preparer_semaphore_count_)));
    AddWaitBase(&sublayer_preparers_.back()->OutputSemaphore());
    AddWaitBase(&sublayer_preparers_.back()->OutputQueue());
  }
  if (sublayer_preparers_.size() == 0) {
    notify(NFY_NOTICE, "There are no features to render for this layer.\n"
           "This is likely due to a display rule filter that filtered all\n"
           "features combined with setting \"Allow Empty Layer\" to ON.\n"
           "This may also be due to lack of data in the vector resource.\n"
           "If this is an error, uncheck \"Allow Empty Layer\" in the \n"
           "Map Layer edit dialog.");
  }
  target_coverage_.cropToWorld(client_map_tile_space);
  fusion_coverage_.cropToWorld(task_config.fusion_tilespace_);
}

Generator::~Generator(void) {
  // in .cpp so we have complete definitions for some classes that are only
  // forward declared in the header
}

void Generator::Run(void) {
  std::uint64_t write_count = 0;
  khTimer_t start_time = khTimer::tick();
  khDeleteGuard<char, ArrayDeleter> buffer(TransferOwnership(
                     new char[CompressorJob::SuperTileAreaInBytes() *
                              task_config_.num_render_threads_]));
  memset(&buffer[0], 0, CompressorJob::SuperTileAreaInBytes() *
         task_config_.num_render_threads_);

  {
    if (sublayer_preparers_.size() == 0) {
      // Empty layer. Nothing to render.
      // Create an empty tile as a placeholder for the layer (so other indexing
      // pipeline does not break.
      // TODO: need nicer way to indicate empty layer.
      gemaptilegen::WriteDummyTile(WriteArguments(&write_packet_queue_,
                                                        &writer_,
                                                        write_buffer_size_,
                                                        &write_count,
                                                        NULL));
      notify(NFY_NOTICE, "Generated 1 map tile in 0 sec. (0 tps)");  // NOLINT
      return;
    }
    mttypes::Semaphore write_buffer_control(0);
    // Only one writer_thread and it waits for its input from
    // compressor_threads.
    mttypes::ManagedThread writer_thread(
        *this,
        khFunctor<void>(
            std::ptr_fun(gemaptilegen::CommandLoopForWriter),
            WriteArguments(&write_packet_queue_, &writer_, write_buffer_size_,
                           &write_count, &write_buffer_control)));
    writer_thread.run();  // 2 (one for main thread)

    // Renderer thread waits for data from Preparer threads.
    // It in turn writes to allocator queue.
    // create the render threads
    khDeletingVector<mttypes::ManagedThread> render_threads;
    for (unsigned int i = 0; i < task_config_.num_render_threads_; ++i) {
      char* pixel_buffer = &buffer[0] +
                           CompressorJob::SuperTileAreaInBytes() * i;
      render_threads.push_back(
          TransferOwnership(
              new mttypes::ManagedThread(
                  *this,
                  khFunctor<void>(
                      std::mem_fun(&Generator::RenderLoop),
                      this, pixel_buffer))));
      render_threads.back()->run();
    }  // 2 + num_render_threads_


    // selector threads write to Preparer threads. Preparer supplies data to
    // renderer. Renderer writes to compressor and then supplies that to
    // allocator.
    // create selector threads
    khDeletingVector<mttypes::ManagedThread> selector_threads;
    for (unsigned int i = 0; i < sublayer_selectors_.size(); ++i) {
      selector_threads.push_back(
          TransferOwnership(
              new mttypes::ManagedThread(
                  *this,
                  khFunctor<void>(
                      std::mem_fun(&vectorquery::FuseSelector::Run),
                      sublayer_selectors_[i]))));
      selector_threads.back()->run();
    }  // 2 + num_render_threads_ + num_sublayers +

    // create preparer threads
    khDeletingVector<mttypes::ManagedThread> preparer_threads;
    for (unsigned int i = 0; i < sublayer_preparers_.size(); ++i) {
      preparer_threads.push_back(
          TransferOwnership(
              new mttypes::ManagedThread(
                  *this,
                  khFunctor<void>(
                      std::mem_fun(&Preparer::Run),
                      sublayer_preparers_[i]))));
      preparer_threads.back()->run();
    }  // 2 + num_render_threads_ + 2 * num_sublayers


    MergeLoop(&write_buffer_control);

    // leaving this scope will join with all threads. All of them are
    // guaranteed to exit, either sucessfully or with an abort
  }

  if (IsAborted()) {
    // One of my threads threw an exception. We should throw too.
    throw khAbortedException();
  }

  khTimer_t stop_time = khTimer::tick();
  double elapsed = khTimer::delta_s(start_time, stop_time);
  double tps = (elapsed != 0.0 ? (write_count / elapsed) : 0.0);
  notify(NFY_NOTICE, "Generated %llu map tiles in %g sec. (%g tps)",
         static_cast<unsigned long long>(write_count), elapsed, tps);  // NOLINT
}

}  // namespace gemaptilegen
