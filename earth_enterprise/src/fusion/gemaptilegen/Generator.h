/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GEMAPTILEGEN_GENERATOR_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GEMAPTILEGEN_GENERATOR_H_


#include <gemaptilegen/.idl/Config.h>
#include <mttypes/DrainableQueue.h>
#include <mttypes/Semaphore.h>
#include <mttypes/WaitBaseManager.h>
#include <merge/merge.h>
#include <gst/vectorprep/WorkflowProcessor.h>
#include <packetfile/packetfilewriter.h>
#include <gst/maprender/Renderer.h>
#include <gst/maprender/Combiner.h>
#include <gst/vectorquery/FuseSelector.h>
#include <gst/vectorprep/FusePreparer.h>
#include <map>
#include <string>

class geFilePool;
class Compressor;
class geColor;

namespace gemaptilegen {

class CompressorOutput;

class TaskConfig {
 public:
  const khTilespace& fusion_tilespace_;

  std::uint32_t selector_input_seed_size_;

  // must be smaller than input_seed_size
  std::uint32_t selector_output_batch_size_;

  std::uint32_t preparer_semaphore_count_;

  // must be less than preparer_semaphore_count_
  std::uint32_t merge_source_input_batch_size_;

  std::uint32_t merger_output_batch_size_;

  std::uint32_t num_map_tiles_from_each_fusion_tile_;

  // must be 1 for now!
  // Need re-syncronization logic before we can make multiple render threads
  std::uint32_t num_render_threads_;

  // sent directly to png compressor.  must be 0-9
  int png_compression_level_;

  TaskConfig(const khTilespace &fusion_tilespace, int pnglevel,
             std::uint32_t num_cpus) :
      fusion_tilespace_(fusion_tilespace),
      png_compression_level_(pnglevel) {
    selector_input_seed_size_   = 10;

    // must be smaller than input_seed_size
    selector_output_batch_size_ = 1;

    preparer_semaphore_count_ = 6;

    // must be less than preparer_semaphore_count_
    merge_source_input_batch_size_ = 1;

    merger_output_batch_size_ = 1;

    // number of 256*256 pixel map subtiles from one Fusion supertile (which is
    // 2048*2048)
    const std::uint32_t larger_by =
        fusion_tilespace_.tileSize / ClientMapFlatTilespace.tileSize;
    num_map_tiles_from_each_fusion_tile_ = larger_by * larger_by;

    switch (num_cpus) {
      case 0:
      case 1:
        // atleast one renderer thread
        num_render_threads_ = 1;
        break;
      case 2:
      case 3:
      case 4:
        num_render_threads_ = num_cpus;
        break;
      default:
        // If we have more than 4 cpus save one for reads/writes.
        num_render_threads_ = num_cpus - 1;
    }
  }
};


// Various task TODO items.
// They hold all the information needed to perform the next step in the task.
class RenderItem;

class RenderContext {
 public:
  maprender::Combiner           combiner_;
  maprender::CombinerOutputTile combine_tile_;
  maprender::Renderer           renderer_;
  maprender::RendererOutputTile render_tile_;

  RenderContext(const khTilespace &fusion_tilespace, char* canvas_buff);
};


class Generator : public mttypes::WaitBaseManager {
 public:
  typedef mttypes::Queue<CompressorOutput*>          WriteQueue;
  typedef mttypes::Queue<RenderItem*>                RenderQueue;
  typedef vectorprep::FusePreparer<MapDisplayRuleConfig> Preparer;
  typedef std::pair<vectorprep::FusePreparer<MapDisplayRuleConfig>::OutTile,
                    const int> MergeEntry;

  Generator(geFilePool &file_pool, const std::string &outdir,
            const Config &config, const TaskConfig &task_config,
            bool is_mercator);
  ~Generator(void);

  void Run(void);

 private:

  // various thread functions
  void MergeLoop(mttypes::Semaphore* write_buffer_control);
  void RenderLoop(char* pixel_buffer);

  // Need to suspend both renderer and compressors if write queue is too
  // big (> max_bytes_sent_to_write) and resume when write queue comes back to
  // normal (i.e <= max_bytes_sent_to_write).
  const std::uint32_t write_buffer_size_;  // size of write_buffer for bundle writer
  const std::uint32_t index_buffer_size_;  // size of index_buffer for bundle writer
  // max size of buffer at which other jobs are suspended to avoid write
  // swamping, e.g memory overflow of buffer
  // Note that this buffer size is bytes_sent_to_write_ + write_buffer_size_ +
  // index_buffer_size
  const std::uint32_t max_buff_size_to_suspend_others_;
  const std::uint32_t max_bytes_sent_to_write_;  // maximum that can be sent to write

  PacketFileWriter writer_;
  const TaskConfig task_config_;
  khLevelCoverage target_coverage_;
  khLevelCoverage fusion_coverage_;

  khDeletingVector<vectorquery::FuseSelector> sublayer_selectors_;
  khDeletingVector<Preparer> sublayer_preparers_;
  khDeletingVector<RenderItem>    render_items_deleter_;

  WriteQueue      write_packet_queue_;
  RenderQueue     render_queue_;
  RenderQueue     free_render_item_queue_;
  const bool      is_mercator_;
};

}  // namespace gemaptilegen


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GEMAPTILEGEN_GENERATOR_H_
