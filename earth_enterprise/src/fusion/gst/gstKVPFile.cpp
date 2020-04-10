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


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>

#include <khException.h>
#include <gstKVPFile.h>
#include <gstKVPTable.h>
#include <gstFileUtils.h>
#include <gstRecord.h>
#include <gstSource.h>
#include <gst/.idl/gstKVPAsset.h>
#include <khFileUtils.h>
#include <gstJobStats.h>
#include <common/khConstants.h>

//
// Keyhole Vector Product
//

gstKVPFile::gstKVPFile(const char* n)
    : gstMemory(n) {
  file_descriptor_ = -1;

  memset(static_cast<void*>(&file_header_), 0, sizeof(FileHeaderV2));

  file_header_.magic = KVP_MAGIC;

  // version 2 adds geometric type to file header
  file_header_.version = 2;
  file_header_.primType = gstUnknown;

  current_offset_ = sizeof(FileHeaderV2);

  modified_ = false;
}

gstKVPFile::~gstKVPFile() {
  Close();
}

gstStatus gstKVPFile::Open(gstReadMode mode) {
  // safety check, don't let file be opened twice
  if (file_descriptor_ != -1)
    return GST_UNKNOWN;

  mode_ = mode;
  assert(mode == GST_READONLY || mode == GST_WRITEONLY);

  mode_t readmode = (mode == GST_READONLY) ? O_RDONLY : O_WRONLY;

  gstFileInfo fi(name());

  //
  // if file already exists, open it
  //
  if (fi.status() == GST_OKAY) {
    file_descriptor_ = ::open(name(), O_LARGEFILE | readmode, 0666);

    if (file_descriptor_ == -1) {
      notify(NFY_WARN, "Unable to open KVP source (r): %s", name());
      return GST_OPEN_FAIL;
    }

    return ReadHeader();
  } else if (mode == GST_WRITEONLY) {
    //
    // if it doesn't exist and mode is write,
    // try to create a new file
    //
    file_descriptor_ = ::open(name(), O_CREAT | O_LARGEFILE | readmode, 0666);
    if (file_descriptor_ == -1) {
      notify(NFY_WARN, "Unable to create new file: %s", name());
      return GST_OPEN_FAIL;
    }
  } else {
    notify(NFY_WARN, "Unable to open KVP source (rw): %s", name());
    return GST_OPEN_FAIL;
  }

  return GST_OKAY;
}


bool gstKVPFile::ReadIndex() {
  gstFileInfo kdx(name());
  kdx.SetExtension("kvindx");

  if (kdx.status() != GST_OKAY)
    return false;

  int kdxfd = ::open(kdx.name(), O_LARGEFILE | O_RDONLY, 0666);
  if (kdxfd == -1) {
    notify(NFY_WARN, "Unable to open index file %s", kdx.name());
    return false;
  }

  khReadFileCloser closer(kdxfd);

  RecordPos* index = static_cast<RecordPos*>(malloc(sizeof(RecordPos) *
                                                    file_header_.numRecords));
  gstFileIO mb(kdxfd, file_header_.numRecords * sizeof(RecordPos),
               reinterpret_cast<char*>(index));

  if (mb.read(0) != GST_OKAY) {
    notify(NFY_WARN, "Unable to read record index from source: %s", kdx.name());
    free(index);
    return false;
  }

  index_map_.init(file_header_.numRecords, index);

  return true;
}


gstStatus gstKVPFile::ReadHeader() {
  //
  // load file header
  //

  // first assume this is v2
  ssize_t bytesread = ::read(file_descriptor_, &file_header_,
                             sizeof(FileHeaderV2));
  if (bytesread == -1) {
    notify(NFY_WARN, "Unable to read file header of source: %s", name());
    return GST_OPEN_FAIL;
  }

  //
  // confirm version
  //
  if (bytesread >= static_cast<ssize_t>(sizeof(FileHeaderV1)) &&
      file_header_.version == 1) {
    // file is version 1, convert header
    FileHeaderV1 v1;
    memcpy(&v1, &file_header_, sizeof(FileHeaderV1));
    // first three fields mem copy properly: magic, version, numRecords,
    // copy the rest individually
    file_header_.primType = gstUnknown;
    file_header_.xmin = v1.xmin;
    file_header_.ymin = v1.ymin;
    file_header_.xmax = v1.xmax;
    file_header_.ymax = v1.ymax;
  } else if (bytesread != sizeof(FileHeaderV2) || file_header_.version != 2) {
    notify(NFY_WARN, "Corrupt file (bad header size or version number): %s",
           name());
    return GST_OPEN_FAIL;
  }


  //
  // validate header
  //
  if (file_header_.magic != KVP_MAGIC) {
    notify(NFY_WARN, "Corrupt file (bad header magic): %s", name());
    return GST_OPEN_FAIL;
  }

  bounding_box_.init(file_header_.xmin, file_header_.xmax,
                     file_header_.ymin, file_header_.ymax);

  if (!ReadIndex())
    return GST_READ_FAIL;

  return GST_OKAY;
}


gstStatus gstKVPFile::Flush() {
  if (mode_ != GST_WRITEONLY)
    return GST_OKAY;

  if (file_descriptor_ == -1)
    return GST_WRITE_FAIL;

  if (!modified_)
    return GST_OKAY;

  // update min/max values
  file_header_.xmin = bounding_box_.w;
  file_header_.xmax = bounding_box_.e;
  file_header_.ymin = bounding_box_.s;
  file_header_.ymax = bounding_box_.n;

  //
  // write header
  //
  if (::lseek64(file_descriptor_, 0, SEEK_SET) == -1 ||
      ::write(file_descriptor_, reinterpret_cast<char*>(&file_header_),
              sizeof(FileHeaderV2)) != sizeof(FileHeaderV2)) {
    notify(NFY_WARN, "Unable to write file header");
    return GST_WRITE_FAIL;
  }

  //
  // write index
  //
  gstFileInfo kdxinfo(name());
  kdxinfo.SetExtension("kvindx");
  int kdxfd = ::open(kdxinfo.name(), O_CREAT | O_LARGEFILE | O_WRONLY | O_TRUNC,
                     0666);
  if (kdxfd == -1) {
    notify(NFY_WARN, "Unable to create record index");
    return GST_OPEN_FAIL;
  }

  khWriteFileCloser closer(kdxfd);

  if (::write(kdxfd, reinterpret_cast<char*>(index_map_.array()),
              file_header_.numRecords * sizeof(RecordPos)) == -1) {
    notify(NFY_WARN, "Unable to write record index");
    return GST_WRITE_FAIL;
  }

  modified_ = false;

  return GST_OKAY;
}


gstStatus gstKVPFile::Close() {
  Flush();

  if (file_descriptor_ != -1) {
    int stat = ::close(file_descriptor_);
    file_descriptor_ = -1;
    if (stat != 0)
      return GST_UNKNOWN;
  }

  return GST_OKAY;
}


gstBBox gstKVPFile::GetGeodeBox(std::uint32_t idx) {
  assert(index_map_.length() != 0);
  assert(idx < file_header_.numRecords);

  const RecordPos& rpos = index_map_[idx];
  return gstBBox(rpos.xmin, rpos.xmax, rpos.ymin, rpos.ymax);
}

gstGeodeHandle gstKVPFile::GetGeode(std::uint32_t idx) {
  assert(index_map_.length() != 0);
  assert(idx < file_header_.numRecords);


  const RecordPos& rpos = index_map_[idx];

  // ensure our raw buffer is big enough
  rawBuf.reserve(rpos.size);

  if (::lseek64(file_descriptor_, rpos.offset, SEEK_SET) == -1) {
    throw khErrnoException(kh::tr("Unable to seek"));
  } else {
    if (!khReadAll(file_descriptor_, &rawBuf[0], rpos.size)) {
      throw khErrnoException(kh::tr("Unable to read"));
    }
  }

  gstGeodeHandle new_geode = gstGeodeImpl::FromRaw(&rawBuf[0]);
  if ( new_geode ) {
    new_geode->SetBoundingBox(rpos.xmin, rpos.xmax, rpos.ymin, rpos.ymax);
  } else {
    throw khException(kh::tr("Unable to interpret raw feature"));
  }
  return new_geode;
}

gstKVPFile::RecordPos gstKVPFile::Allocate(gstGeodeHandle geode) {
  RecordPos rpos;
  rpos.size = geode->RawSize();
  rpos.offset = current_offset_;
  rpos.xmin = geode->BoundingBox().w;
  rpos.xmax = geode->BoundingBox().e;
  rpos.ymin = geode->BoundingBox().s;
  rpos.ymax = geode->BoundingBox().n;

  current_offset_ += rpos.size;
  index_map_.append(rpos);
  file_header_.numRecords = index_map_.length();

  return rpos;
}


gstStatus gstKVPFile::AddGeode(gstGeodeHandle geode) {
  // we must be in write mode to accept new geodes
  if (mode_ != GST_WRITEONLY) {
    notify(NFY_WARN, "Unsupported writing to a READONLY file. ");
    return GST_UNSUPPORTED;
  }

  RecordPos rpos = Allocate(geode);

  // ensure our raw buffer is big enough
  rawBuf.reserve(rpos.size);

  if (geode->ToRaw(&rawBuf[0]) == nullptr) {
    notify(NFY_WARN, "Failed to convert geode to raw buffer");
    return GST_UNKNOWN;
  }

  bounding_box_.Grow(geode->BoundingBox());

  //
  // write new record at the index position
  // and slide index to end of this new record
  //

  gstFileIO mb(file_descriptor_, rpos.size, &rawBuf[0]);
  if (mb.write(rpos.offset) != GST_OKAY) {
    notify(NFY_WARN, "Cannot write new record to file %s", name());
    return GST_WRITE_FAIL;
  }

  modified_ = true;

  return GST_OKAY;
}


//
// ----------------------------------------------------------------------
//


gstKVGeomFormat::gstKVGeomFormat(const char* n)
    : gstFormat(n) {
  kvp_file_ = new gstKVPFile(n);
  kvp_table_ = new gstKVPTable(khReplaceExtension(n, ".kvattr").c_str());
  noXForm = true;
}

gstKVGeomFormat::~gstKVGeomFormat() {
  delete kvp_file_;
  delete kvp_table_;
}

gstStatus gstKVGeomFormat::OpenFile() {
  gstStatus stat = kvp_file_->OpenForRead();
  if (stat != GST_OKAY)
    return stat;

  stat = kvp_table_->Open(GST_READONLY);

  if (stat == GST_OKAY) {
    gstLayerDef& ldef = AddLayer(kvp_file_->NumRecords(),
                                 kvp_table_->GetHeader());
    ldef.SetBoundingBox(kvp_file_->BoundingBox());
  } else {
    gstLayerDef& ldef = AddLayer(kvp_file_->NumRecords(),
                                 gstHeaderImpl::Create());
    ldef.SetBoundingBox(kvp_file_->BoundingBox());
    delete kvp_table_;
    kvp_table_ = nullptr;
  }

  return GST_OKAY;
}


gstStatus gstKVGeomFormat::CloseFile() {
  if (kvp_table_)
    kvp_table_->Close();
  gstStatus stat = kvp_file_->Close();

  return stat;
}


gstGeodeHandle gstKVGeomFormat::GetFeatureImpl(std::uint32_t layer, std::uint32_t fid) {
  // should be checked by gstSource before calling me
  assert(layer < NumLayers());
  assert(layer == 0);
  assert(fid < NumFeatures(layer));

  return kvp_file_->GetGeode(fid);
}

gstBBox gstKVGeomFormat::GetFeatureBoxImpl(std::uint32_t layer, std::uint32_t fid) {
  // should be checked by gstSource before calling me
  assert(layer < NumLayers());
  assert(layer == 0);
  assert(fid < NumFeatures(layer));

  return kvp_file_->GetGeodeBox(fid);
}

#ifdef JOBSTATS_ENABLED
enum {JOBSTATS_GETATTR_GEOM};
static gstJobStats::JobName KVGeomJobNames[] = {
  {JOBSTATS_GETATTR_GEOM, "Get Attribute "}
};
gstJobStats* geom_stats = new gstJobStats("KVGEOM FMT", KVGeomJobNames, 1);
#endif

gstRecordHandle gstKVGeomFormat::GetAttributeImpl(std::uint32_t layer, std::uint32_t fid) {
  // should be checked by gstSource before calling me
  assert(layer < NumLayers());
  assert(layer == 0);
  assert(fid < NumFeatures(layer));

  JOBSTATS_SCOPED(geom_stats, JOBSTATS_GETATTR_GEOM);
  if (kvp_table_) {
    return kvp_table_->Row(fid);
  }

  throw khException(kh::tr("No attributes available"));
}

//
// ----------------------------------------------------------------------------
//

gstKVPFormat::gstKVPFormat(const char* n)
    : gstFormat(n) {
  noXForm = true;
}

gstKVPFormat::~gstKVPFormat() {
  CloseFile();
}

gstStatus gstKVPFormat::OpenFile() {
  QString configName(name());
  if (configName.endsWith(QString(".kvp")))
    configName = configName + "/";
  if (configName.endsWith(QString(".kvp/")))
    configName = configName + kHeaderXmlFile.c_str();

  gstKVPAsset kvpasset;
  if (!kvpasset.Load(configName.latin1())) {
    notify(NFY_WARN, "Error opening %s: %s", FormatName(), configName.latin1());
    return GST_OPEN_FAIL;
  }

  gstBBox max_box;
  int total_features = 0;
  int source_key = 0;
  // diameter of average feature of layer.
  double average_feature_diameter = .0;

  feature_map_[0] = 0;
  for (std::vector<gstKVPAsset::Source>::const_iterator it =
         kvpasset.sources.begin();
       it != kvpasset.sources.end(); ++it) {
    const char* fname = (*it).file.c_str();
    gstFileInfo fi(fname);
    if (fi.status() != GST_OKAY) {
      fi = gstFileInfo(gstFileInfo(configName.latin1()).dirName(), fname);
      if (fi.status() != GST_OKAY) {
        notify(NFY_WARN, "Unable to find source: %s", fname);
        continue;
      }
    }

    sources_.push_back(new gstSource(fi.name()));
    feature_counts_.push_back((*it).featureCount);
    total_features += (*it).featureCount;
    feature_map_[total_features] = source_key++;

    max_box.Grow(gstBBox((*it).xmin, (*it).xmax, (*it).ymin, (*it).ymax));

    average_feature_diameter += (*it).averageFeatureDiameter;

    notify(NFY_DEBUG, "  Adding source: %s, %d",
           (*it).file.c_str(), (*it).featureCount);
  }

  average_feature_diameter /= kvpasset.sources.size();

  gstHeaderHandle hdr = gstHeaderImpl::Create();
  if (kvpasset.header.size()) {
    for (unsigned int f = 0; f < kvpasset.header.size(); ++f)
      hdr->addSpec(kvpasset.header[f].name, kvpasset.header[f].type);
  }

  notify(NFY_DEBUG, "Total Sources = %lu",
         static_cast<long unsigned>(kvpasset.sources.size()));
  notify(NFY_DEBUG, "Total Features = %d", total_features);

  //  gstLayerDef& ldef = AddLayer(kvp_file_->file_header_.primType,
  //                               total_features,
  //                               hdr);
  gstLayerDef& ldef = AddLayer(gstPrimType(kvpasset.primType),
                               total_features, hdr);
  ldef.SetBoundingBox(max_box);
  ldef.SetAverageFeatureDiameter(average_feature_diameter);

  ldef.SetMinResolutionLevel(kvpasset.minResolutionLevel);
  ldef.SetMaxResolutionLevel(kvpasset.maxResolutionLevel);
  ldef.SetEfficientResolutionLevel(kvpasset.efficientResolutionLevel);

  opened_source_ = -1;
  last_source_id_ = -1;
  last_range_start_ = UINT_MAX;
  last_range_end_ = UINT_MAX;

  return GST_OKAY;
}

gstStatus gstKVPFormat::CloseFile() {
  for (std::vector<gstSource*>::iterator it = sources_.begin();
       it != sources_.end(); ++it)
    (*it)->unref();

  sources_.clear();
  feature_counts_.clear();
  feature_map_.clear();
  opened_source_ = -1;
  last_source_id_ = -1;

  return GST_OKAY;
}

#ifdef JOBSTATS_ENABLED
enum {JOBSTATS_GETATTR, JOBSTATS_GETFEATURE, JOBSTATS_GETBOX,
      JOBSTATS_DETSRC, JOBSTATS_OPENSRC};
static gstJobStats::JobName KVPFormatJobNames[] = {
  {JOBSTATS_GETATTR,    "Get Attribute   "},
  {JOBSTATS_GETFEATURE, "Get Feature     "},
  {JOBSTATS_GETBOX,     "Get Feature Box "},
  {JOBSTATS_DETSRC,     "Determine Src   "},
  {JOBSTATS_OPENSRC,    "Open Source     "}
};
gstJobStats* kvp_stats = new gstJobStats("KVP FMT", KVPFormatJobNames, 5);
#endif

gstRecordHandle gstKVPFormat::GetAttributeImpl(std::uint32_t layer, std::uint32_t fid) {
  // should be checked by gstSource before calling me
  assert(layer < NumLayers());
  assert(layer == 0);
  assert(fid < NumFeatures(layer));

  JOBSTATS_SCOPED(kvp_stats, JOBSTATS_GETATTR);

  std::uint32_t true_fid;
  int src = DetermineSource(fid, &true_fid);
  if (src == -1) {
    throw khException(kh::tr("Unable to determine source"));
  }

  return sources_[src]->GetAttributeOrThrow(0, true_fid);
}

gstGeodeHandle gstKVPFormat::GetFeatureImpl(std::uint32_t layer, std::uint32_t fid) {
  // should be checked by gstSource before calling me
  assert(layer < NumLayers());
  assert(layer == 0);
  assert(fid < NumFeatures(layer));

  JOBSTATS_SCOPED(kvp_stats, JOBSTATS_GETFEATURE);

  std::uint32_t true_fid;
  int src = DetermineSource(fid, &true_fid);
  if (src == -1) {
    throw khException(kh::tr("Unable to determine source"));
  }

  return sources_[src]->GetFeatureOrThrow(0, true_fid, false);
}

gstBBox gstKVPFormat::GetFeatureBoxImpl(std::uint32_t layer, std::uint32_t fid) {
  // should be checked by gstSource before calling me
  assert(layer < NumLayers());
  assert(layer == 0);
  assert(fid < NumFeatures(layer));

  JOBSTATS_SCOPED(kvp_stats, JOBSTATS_GETBOX);

  std::uint32_t true_fid;
  int src = DetermineSource(fid, &true_fid);
  if (src == -1) {
    throw khException(kh::tr("Unable to determine source"));
  }

  return sources_[src]->GetFeatureBoxOrThrow(0, true_fid);
}

// figure out which source this feature comes from and make
// sure it is currently opened
int gstKVPFormat::DetermineSource(std::uint32_t feature_id, std::uint32_t* real_feature_id) {
  JOBSTATS_SCOPED(kvp_stats, JOBSTATS_DETSRC);
  // sequential reading can avoid the lookup almost entirely
  if (last_source_id_ != -1 &&
      feature_id >= last_range_start_ && feature_id < last_range_end_) {
    // no need to attempt to open source since this feature is coming from the
    // same source as the previous request
    *real_feature_id = feature_id - last_range_start_;
    return last_source_id_;
  }

  std::map<int, std::uint32_t>::const_iterator upper_pos =
      feature_map_.upper_bound(feature_id);
  if (upper_pos == feature_map_.end())
    return -1;
  std::map<int, std::uint32_t>::const_iterator back_one = upper_pos;
  --back_one;
  int source_id = upper_pos->second;
  if (!OpenSource(source_id))
    return -1;
  *real_feature_id = feature_id - back_one->first;

  last_source_id_ = source_id;
  last_range_start_ = back_one->first;
  last_range_end_ = upper_pos->first;

  return source_id;
}


bool gstKVPFormat::OpenSource(int id) {
  JOBSTATS_SCOPED(kvp_stats, JOBSTATS_OPENSRC);
  if (id == opened_source_)
    return true;
  return CheckOpenSource(id);
}

bool gstKVPFormat::CheckOpenSource(int id) {
  if (opened_source_ != -1)
    sources_[opened_source_]->Close();

  if (sources_[id]->Open() != GST_OKAY) {
    static int msgcount = 0;
    msgcount++;
    if (msgcount < 10) {
      notify(NFY_WARN, "Unable to open source: %s", sources_[id]->name());
    } else if (msgcount == 10) {
      notify(NFY_WARN,
             "Unable to open source: %s (suppressing further messages)",
             sources_[id]->name());
    }
    return false;
  }

  opened_source_ = id;

  return true;
}
