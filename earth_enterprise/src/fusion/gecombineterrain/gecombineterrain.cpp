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

//
// gecombineterrain
//
// Perform 5-way terrain packet combination and compression.  Combined
// terrain packets are stored only at the even quadtree path level.
// Each even-level combined packet includes terrain (if any) from the
// four odd-level children of the packet.  This program takes as input
// any number of indexes of terrain packets, and merges the packets,
// writing the new packets in packetfile format.
//
// In order to gather the packets, QuadsetGather is used to group all
// terrain packets from the same quadset.  This uses the same code as
// the quadtree packet builder.  Although it is not really necessary
// to group all the packets of the quadset, this uses existing code
// with little additional overhead, making the process much simpler.

#include "fusion/gecombineterrain/combineterrain.h"
#include "fusion/gst/gstJobStats.h"
#include "fusion/config/gefConfigUtil.h"
#include "common/geindex/Traverser.h"
#include "common/config/geConfigUtil.h"
#include "common/notify.h"
#include "common/khGetopt.h"
#include "common/khAbortedException.h"
#include "common/khSimpleException.h"
#include "common/khFileUtils.h"
#include "common/performancelogger.h"

#ifdef JOBSTATS_ENABLED
enum {MERGER_CREATED, GATHERER_CREATED, COMBINE};
static gstJobStats::JobName JobNames[] = {
  {MERGER_CREATED,   "Merger_Created      "},
  {GATHERER_CREATED, "Gatherer_Created    "},
  {COMBINE,          "Combine_Terrain     "},
};
gstJobStats* job_stats = new gstJobStats("GECOMBINETERRAIN", JobNames, 3);
#endif

namespace {

const std::uint32_t kDefaultNumCPUs = 1;

// Number of cache blocks per filebundle.
const std::uint32_t kDefaultReadCacheBlocks = 5;

// Optimal Read cache block size.
const std::uint32_t kDefaultReadCacheBlockKilobyteSize = 64;

const std::uint32_t kDefaultSortBufferMegabytes = 512;

// Assume 4GB is the min recommended.
const std::uint64_t kDefaultMinMemoryAssumed = 4000000000U;

const static std::string taskName = "gecombinterrain";

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(
      stderr,
      "\nusage: %s --indexversion <ver> [--numcpus/--numCompressThreads <num>] "
      "[--read_cache_max_blocks <num>\n"
      "--output <outdir> <index1> [<index2> ...]\n"
      "   Terrain packets are merged from the specified index(es)\n"
      "   If the output packet file <outdir> already exists,\n"
      "   its contents are overwritten.\n"
      "   Supported options are:\n"
      "      --help | -?:  Display this usage message\n"
      "      --sortbuf <buffer_size>: size of packet index sort buf in MB\n"
      "        (default %u)\n"
      "      --numCompressThreads:    Number of CPUs/threads to use \n("
      "        defaults to systemrc, then to number of CPUs available)\n"
      "      --numcpus: deprecated in favor of --numCompressThreads\n"
      "      --read_cache_max_blocks: Number of read cache blocks for each\n"
      "                              terrain resource (between 0 and 1024)\n"
      "                              Read caching is DISABLED if num < 2.\n"
      "                              (default %u)\n"
      "      --read_cache_block_size: Size in Kilobytes of read cache blocks\n"
      "                               between 1 and 1024 (recommend set "
      "                               num to a power of 2) (default %u)\n"
      "\n",
      progn.c_str(), kDefaultSortBufferMegabytes, 
      kDefaultReadCacheBlocks, kDefaultReadCacheBlockKilobyteSize);
  exit(1);
}

typedef geindex::Traverser<geindex::BlendBucket> TmeshTraverser;
typedef geterrain::TerrainQuadsetItem TerrainQuadsetItem;
typedef Merge<TerrainQuadsetItem> TerrainMergeType;

// TranslatingTerrainTraverser - translate terrain entries into a form
// suitable for Quadset gathering.  The main function is to change
// packet file indexes relative to an index into packet file tokens
// from a CountedPacketFileReaderPool.  This allows the reference to the
// original packet file to be carried through the merge.

class TranslatingTerrainTraverser : public MergeSource<TerrainQuadsetItem> {
 public:
  TranslatingTerrainTraverser(
      geterrain::CountedPacketFileReaderPool *reader_pool,
      const std::string &index_path)
      : MergeSource<TerrainQuadsetItem>("Translate_" + index_path),
        index_path_(index_path),
        source_traverser_("Traverse_" + index_path,
                          reader_pool->file_pool(),
                          index_path),
        source_index_(0),
        current_(QuadtreePath(), geterrain::TerrainPacketItem()),
        finished_(false) {
    // Add packet file members to reader pool
    {
      const geindex::IndexBundleReader &reader =
        source_traverser_.GetIndexBundleReader();
      for (std::uint32_t p = 0; p < reader.PacketFileCount(); ++p) {
        std::string packetfile = reader.GetPacketFile(p);
        // skip removed packetfiles (delta index)
        if (packetfile.empty()) {
          tokens_.push_back(PacketFileReaderToken());
        } else {
          tokens_.push_back(reader_pool->Add(packetfile));
        }
      }
    }

    Advance();
  }
  virtual ~TranslatingTerrainTraverser() {}

  virtual const MergeType &Current() const { return current_; }

  virtual bool Advance() {
    if (finished_)
      return false;

    {                                   // restrict scope of source_current
      while (source_index_ >= source_traverser_.Current().size()) {
        source_index_ = 0;
        finished_ = !source_traverser_.Advance();
        if (finished_)
          return false;
      }

      const TmeshTraverser::EntryType
        &source_current(source_traverser_.Current().at(source_index_));
      QuadtreePath qt_path = source_traverser_.Current().qt_path();
      ++source_index_;

      std::uint32_t fileNum = source_current.dataAddress.fileNum;
      if (fileNum >= tokens_.size()) {
        throw khSimpleException("TranslatingTerrainTraverser::Advance(): ")
          << "fileNum(" << fileNum
          << ") out of range(" << tokens_.size()
          << ") at qt_path \"" << qt_path.AsString().c_str()
          << "\"";
      }

      current_ =
        TerrainQuadsetItem(
            qt_path,
            geterrain::TerrainPacketItem(
                tokens_.at(fileNum),
                source_current.dataAddress.fileOffset,
                source_current.dataAddress.size,
                source_current.version,
                source_current.insetId));
    }
    return true;
  }

  virtual void Close() {
    source_traverser_.Close();
    finished_ = true;
  }

 private:
  const std::string index_path_;
  TmeshTraverser source_traverser_;
  size_t source_index_;
  std::vector<PacketFileReaderToken> tokens_;
  TerrainQuadsetItem current_;
  bool finished_;
};

}  // namespace

int main(int argc, char **argv) {
  // On successful completion, print out the output file sizes.
  std::vector<std::string> output_files;
  try {
    BEGIN_PERF_LOGGING(parse_args, "parse", "arguments");
    std::string progname = argv[0];

    // Process commandline options
    int argn;
    bool help = false;
    std::string outdir;
    int index_version = 0;
    int sortbuf = kDefaultSortBufferMegabytes;
    
    //initialize to 0 to make it easier to determine if parameter was passed
    std::uint32_t numcpus = 0,
           numCompressThreads = 0;
    PERF_CONF_LOGGING( "proc_exec_config_default_numcpus", taskName, numcpus );
    std::uint32_t read_cache_max_blocks = kDefaultReadCacheBlocks;
    std::uint32_t read_cache_block_size = kDefaultReadCacheBlockKilobyteSize;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("output", outdir);
    options.opt("indexversion", index_version);
    options.opt("sortbuf", sortbuf);
    options.opt("numCompressThreads",numCompressThreads,
               &khGetopt::RangeValidator<std::uint32_t, 1, kMaxNumJobsLimit_2>);
    options.opt("numcpus", numcpus,
                &khGetopt::RangeValidator<std::uint32_t, 1, kMaxNumJobsLimit_2>);
    PERF_CONF_LOGGING( "proc_exec_config_cli_numcpus", taskName, numcpus );
    options.opt("read_cache_max_blocks", read_cache_max_blocks,
                &khGetopt::RangeValidator<std::uint32_t, 0, 1024>);
    options.opt("read_cache_block_size", read_cache_block_size,
                &khGetopt::RangeValidator<std::uint32_t, 1, 1024>);
    if (!options.processAll(argc, argv, argn)) {
      usage(progname);
    }
    if (help) {
      usage(progname);
    }

    if (argn == argc) {
      usage(progname, "No input indices specified");
    }
    unsigned int cmdDefaultCPUs = CommandlineNumCPUsDefault();
    unsigned int numavailable = GetMaxNumJobs();

    /* 
        Choosing number of CPUs:

        numcpus: initially, the command line parameter --numcpus
	
        CommandLineNumCPUsDefault(): defined in /etc/opt/google/systemrc via maxjobs tag

        GetMaxNumJobs(): the number of available jobs, defined as the minimum value of either:
        1. the currently available jobs or the default number of jobs i.e. kMaxNumJobs 
            (is 8) multiplied by KH_MAX_NUM_JOBS_COEFF, which is a compiler option
        2. kMaxNumJobsDefault = 8
        3. environment variable, KH_GOOGLE_MAX_NUM_JOBS, if defined
        4. the number of currently available cpus, obtained through sysconf

        The methodology first checks if the --numcpus flag was present, if
        it was, it will go with what was passed, no matter the number. If
        it was not, it will take the minimum value of what is defined in the
        systemrc file and what is currently available (values described 
        above). If the maxjobs tag in systemrc is invalid (i.e. less than
        1) it will default to the available CPUs.

         
        --numcpus is deprecated in favor of --numCompressThreads. One
        or the other may be passed in, but not both.
    */
 
    if (numCompressThreads)
    {
       if (numcpus) {
         usage(progname,"--numcpus deprecated in favor of --numCompressedThreads");
       } else {
          // if numcpus is not present, but numCompressThreads is, set
          // numcpus equal to numCompressThreads
          numcpus = numCompressThreads;
       }
    }
    
    if (!numcpus) {
	if (cmdDefaultCPUs <= 1) {
          cmdDefaultCPUs = numavailable;
	}
        numcpus = std::min(cmdDefaultCPUs,numavailable);
    }
   
    PERF_CONF_LOGGING( "proc_exec_vcpu_count", taskName, numcpus );

    notify(NFY_WARN, "gecombineterrain numcpus %llu",
           static_cast<long long unsigned int>(numcpus));
    notify(NFY_WARN, "gecombineterrain CommandLineNumCPUsDefault(): %llu",
           static_cast<long long unsigned int>(cmdDefaultCPUs));
    notify(NFY_WARN, "gecombineterrain GetMaxNumJobs(): %llu",
           static_cast<long long unsigned int>(numavailable)); 
    
    // Validate commandline options
    if (!outdir.size()) {
      usage(progname, "No output specified");
    }
    if (index_version <= 0) {
      usage(progname, "Index version not specified or <= 0");
    }
    if (numcpus < 1) {
      usage(progname, "Number of CPUs should not be less than 1");
    }
    if (sortbuf <= 0) {
      notify(NFY_FATAL, "--sortbuf must be > 0, is %d", sortbuf);
    }
    END_PERF_LOGGING(parse_args);

    // Create a merge of the terrain indices
    JOBSTATS_BEGIN(job_stats, MERGER_CREATED);    // validate
    BEGIN_PERF_LOGGING(create_merger, "create", "merger");

    // We'll need to limit the number of filebundles opened by the filepool
    // at a single time, to keep from overflowing memory.
    // Allow 50 files for other operations outside the filepool.
    int max_open_fds = GetMaxFds(-50);
    PERF_CONF_LOGGING( "proc_exec_config_max_open_fds", taskName, max_open_fds );
    // Read Cache is enabled only if read_cache_max_blocks is > 2.
    if (read_cache_max_blocks < 2) {
      notify(NFY_WARN, "Read caching is disabled. This will cause %s"
                       "to be much slower. To enable, set the "
                       "read_cache_blocks setting\n"
                       "to a number 2 or greater.\n", argv[0]);
    } else {
      // Get the physical memory size to help choose the read_cache_max_blocks.
      std::uint64_t physical_memory_size = GetPhysicalMemorySize();
      PERF_CONF_LOGGING( "proc_exec_config_memsize", taskName, physical_memory_size );
      if (physical_memory_size == 0) {
        physical_memory_size = kDefaultMinMemoryAssumed;
        PERF_CONF_LOGGING( "proc_exec_config_memsize", taskName, physical_memory_size );
        notify(NFY_WARN, "Physical Memory available not found. "
               "Assuming min recommended system size: %llu bytes",
               static_cast<long long unsigned int>(physical_memory_size));
      } else {
        notify(NFY_NOTICE, "Physical Memory available: %llu bytes",
               static_cast<long long unsigned int>(physical_memory_size));
      }

      // Convert this read cache block size from kilobytes to bytes.
      read_cache_block_size *= 1024U;

      // Figure out the worst case size of the read cache
      // (if all of max_open_fds are open simultaneously)
      std::uint64_t estimated_read_cache_bytes = max_open_fds *
        static_cast<std::uint64_t>(read_cache_max_blocks * read_cache_block_size);
      notify(NFY_NOTICE,
             "Read Cache Settings: %u count %u byte blocks per resource "
             "(max files open set to %u)\n"
             "This will use approximately %llu bytes in memory.",
             read_cache_max_blocks, read_cache_block_size, max_open_fds,
             static_cast<long long unsigned int>(estimated_read_cache_bytes));
      if (estimated_read_cache_bytes > physical_memory_size) {
        // If our worst case read cache blows out our memory, then
        // lower the max_open_fds to bring it to within 90% of the memory.
        // Be careful with overflow here.
        max_open_fds = (physical_memory_size * 90ULL)/
          (100ULL * read_cache_max_blocks * read_cache_block_size);
        notify(NFY_WARN, "The estimated read cache size (%llu bytes) exceeds\n"
                         "the Physical Memory available: %llu bytes.\n"
                         "We are reducing the max files open to %d to eliminate"
                         "memory overruns.\n",
               static_cast<long long unsigned int>(estimated_read_cache_bytes),
               static_cast<long long unsigned int>(physical_memory_size),
               max_open_fds);
      }
    }

    geFilePool file_pool(max_open_fds);
    geterrain::CountedPacketFileReaderPool packet_reader_pool(
        "TerrainReaderPool",
        file_pool);
    // Note: read cache's will not work without at least 2 blocks.
    if (read_cache_max_blocks >= 2) {
      packet_reader_pool.EnableReadCache(read_cache_max_blocks,
                                         read_cache_block_size);
    }

    khDeleteGuard<TerrainMergeType> merger(
        TransferOwnership(new TerrainMergeType("Terrain Merger")));

    // Print the input file sizes for diagnostic log file info.
    std::vector<std::string> input_files;

    fprintf(stderr, "index version: %d\n", index_version);
    for (int i = argn; i < argc; ++i) {
      notify(NFY_INFO, "Opening terrain index: %s", argv[i]);

      merger->AddSource(
          TransferOwnership(
              new TranslatingTerrainTraverser(&packet_reader_pool,
                                              argv[i])));
      input_files.push_back(argv[i]);
    }
    khPrintFileSizes("Input File Sizes", input_files);

    merger->Start();
    END_PERF_LOGGING(create_merger);
    JOBSTATS_END(job_stats, MERGER_CREATED);

    // Feed this merge into a QuadsetGather operation
    JOBSTATS_BEGIN(job_stats, GATHERER_CREATED);    // validate
    BEGIN_PERF_LOGGING(create_gatherer, "create", "gatherer");

    qtpacket::QuadsetGather<geterrain::TerrainPacketItem>
      gather("TerrainQuadsetGather", TransferOwnership(merger));

    // Create the output packetfile
    geterrain::TerrainCombiner combiner(packet_reader_pool, outdir, numcpus);
    combiner.StartThreads();
    notify(NFY_DEBUG, "started combineterrain");

    // We need to wrap the combiner with a try/catch because otherwise, the
    // exception causes a deconstructor failure which masks the real error
    // which could be a CRC error in one of the terrain packets.
    std::string error_message;
    try {
      do {
        combiner.CombineTerrainPackets(gather.Current());
      } while (gather.Advance());
    } catch (const khAbortedException &e) {
      notify(NFY_FATAL, "Unable to proceed: See previous warnings: %s",
             e.what());
    } catch (const std::exception &e) {
      notify(NFY_FATAL, "%s", e.what());
    } catch (...) {
      notify(NFY_FATAL, "Unknown error");
    }

    notify(NFY_DEBUG, "waiting for compress and write threads to finish");
    combiner.WaitForThreadsToFinish();
    notify(NFY_DEBUG, "closing the gatherer");
    gather.Close();
    END_PERF_LOGGING(create_gatherer);
    JOBSTATS_END(job_stats, GATHERER_CREATED);

    // Finish the packet file
    JOBSTATS_BEGIN(job_stats, COMBINE);    // validate
    BEGIN_PERF_LOGGING(start_combiner, "start", "combiner");
    notify(NFY_DEBUG, "writing the packet index");
    combiner.Close(static_cast<size_t>(sortbuf) * 1024 * 1024);
    END_PERF_LOGGING(start_combiner);
    JOBSTATS_END(job_stats, COMBINE);
    // On successful completion, print the output file sizes.
    output_files.push_back(outdir);
  } catch (const khAbortedException &e) {
    notify(NFY_FATAL, "Unable to proceed: See previous warnings");
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  // at the end, call dump all
  JOBSTATS_DUMPALL();

  // On successful completion, print the output file sizes.
  // The print occurs here to allow progress to go out of scope.
  khPrintFileSizes("Output File Sizes", output_files);
  return 0;
}
