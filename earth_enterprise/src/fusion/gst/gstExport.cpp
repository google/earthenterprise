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


#include <gstExport.h>

#include <third_party/rsa_md5/crc32.h>
#include <unistd.h>
#include <algorithm>
#include <qstring.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <qcolor.h>

#include <notify.h>
#include <gstGeode.h>
#include <gstValue.h>
#include <gstFileUtils.h>
#include <gstFeature.h>
#include <gstSite.h>
#include <gstQuadAddress.h>
#include <gstMemoryPool.h>
#include <gstGridUtils.h>

#include <packet.h>
#include <khConstants.h>
#include <khstrconv.h>
#include <khGuard.h>
#include <packetfile/packetfilewriter.h>
#include <packetcompress.h>

static const unsigned int MAX_URL_LENGTH = 1024;

// -----------------------------------------------------------------------------

class gstPacketFileExporter : public gstExporter {
 public:
  gstPacketFileExporter(geFilePool &file_pool, const std::string& path);
  ~gstPacketFileExporter();

  virtual bool MaxPacketSize(int sz) const { return sz <= (int)kMaxPacketSize; }
  virtual bool TargetPacketSize(int sz) const { return sz <= (int)kTargetPacketSize; }

  virtual bool Open();
  virtual int BuildPacket(const gstQuadAddress& addr,
                          const std::vector<gstFeatureSet>& fset,
                          const std::vector<gstSiteSet>& sset);
  virtual bool Save();
  virtual void Close();

 private:
  bool AddLandmark(etLandmarkPacketData* data, gstVertex v,
                   gstRecordHandle rec, const gstSite* site);
  bool AddStreet(etStreetPacketData* data, const gstGeodeHandle &geodeh,
                 gstRecordHandle rec,
                 const gstFeatureConfigs* feature_configs);
  bool AddPolyline(etPolyLinePacketData* data, const gstGeodeHandle &geodeh,
                   gstRecordHandle rec,
                   const gstFeatureConfigs* feature_configs);
  bool AddPolygon(etPolygonPacketData* data, const gstGeodeHandle &geodeh,
                  gstRecordHandle rec,
                  const gstFeatureConfigs* feature_configs);

  geFilePool &file_pool_;
  khDeleteGuard<PacketFileWriter> writer_;
  std::vector<char> compressed_buffer_;
  gstQuadAddress pak_addr_;

  etDrawablePacket* lump_pak_;

  gstMemoryPool mem_pool_;

  char* write_buffer_;
};

// -----------------------------------------------------------------------------

gstPacketFileExporter::gstPacketFileExporter(geFilePool &file_pool,
                                             const std::string& path)
    : gstExporter(path),
      file_pool_(file_pool),
      writer_(),
      compressed_buffer_(kCompressedBufferSize + kCRC32Size),
      lump_pak_(NULL) {
  write_buffer_ = static_cast<char*>(malloc(kMaxPacketSize));
  memset(write_buffer_, 0, kMaxPacketSize);
}

gstPacketFileExporter::~gstPacketFileExporter() {
  free(write_buffer_);
  delete lump_pak_;
}

bool gstPacketFileExporter::Open() {
  try {
    writer_ = TransferOwnership(new PacketFileWriter(file_pool_, name_));
  } catch(const std::exception& e) {
    notify(NFY_WARN, "Failed to open packetfile \"%s\" : %s",
           name_.c_str(), e.what());
    return false;
  }
  return true;
}

int gstPacketFileExporter::BuildPacket(const gstQuadAddress& addr,
                                const std::vector<gstFeatureSet>& fset,
                                const std::vector<gstSiteSet>& sset) {
  pak_addr_ = addr;
  delete lump_pak_;
  lump_pak_ = NULL;

  bool pakFail = true;
  khArray<etDataPacket*> allpaks;

  for (std::vector<gstFeatureSet>::const_iterator it = fset.begin();
       it != fset.end(); ++it) {
    const gstFeatureSet& set = (*it);

    if (set.feature_configs->config.featureType == VectorDefs::LineZ) {
      if (set.feature_configs->config.drawAsRoads) {
        etStreetPacket* pak = new etStreetPacket;
        allpaks.append(pak);
        pak->initUTF8(set.glist.size());
        for (unsigned int g = 0; g < set.glist.size(); ++g) {
          if (!AddStreet(pak->getPtr(g), set.glist[g], set.rlist[g],
                         set.feature_configs))
            goto abortpacket;
        }
        pak->pointerToOffset();
      } else {
        etPolyLinePacket* pak = new etPolyLinePacket;
        allpaks.append(pak);
        pak->init(set.glist.size());
        for (unsigned int g = 0; g < set.glist.size(); ++g) {
          if (!AddPolyline(pak->getPtr(g), set.glist[g], set.rlist[g],
                           set.feature_configs))
            goto abortpacket;
        }
        pak->pointerToOffset();
      }

    } else if (set.feature_configs->config.featureType ==
               VectorDefs::PolygonZ) {
      etPolygonPacket* pak = new etPolygonPacket;
      allpaks.append(pak);
      pak->init(set.glist.size());
      for (unsigned int g = 0; g < set.glist.size(); ++g) {
        if (!AddPolygon(pak->getPtr(g), set.glist[g], set.rlist[g],
                        set.feature_configs))
          goto abortpacket;
      }
      pak->pointerToOffset();
    }
  }

  for (std::vector<gstSiteSet>::const_iterator it = sset.begin();
       it != sset.end(); ++it) {
    const gstSiteSet& set = (*it);

    // all sites are now guaranteed to be Landmarks, we convert them in the
    // afterload
    etLandmarkPacket* pak = new etLandmarkPacket;
    allpaks.append(pak);
    pak->init(set.vlist.size());
    for (unsigned int g = 0; g < set.vlist.size(); ++g) {
      if (!AddLandmark(pak->getPtr(g), set.vlist[g], set.rlist[g], set.site))
        goto abortpacket;
    }
    pak->pointerToOffset();
  }

  lump_pak_ = new etDrawablePacket;
  lump_pak_->init(allpaks.length());

  // collect all the individual packets
  for (unsigned int pk = 0; pk < allpaks.length(); ++pk)
    memcpy(lump_pak_->getPtr(pk), allpaks[pk], sizeof(etDataPacket));

  lump_pak_->pointerToOffset();

  // if we get here, the pak is good
  pakFail = false;

abortpacket:
  for (unsigned int pk = 0; pk < allpaks.length(); ++pk)
    delete allpaks[pk];

  //
  // release any alocated memory here
  //
  mem_pool_.ResetAll();

  return pakFail ? -1 : lump_pak_->getsavesize();
}


bool gstPacketFileExporter::AddStreet(etStreetPacketData* pakdata,
                               const gstGeodeHandle &geodeh,
                               gstRecordHandle rec,
                               const gstFeatureConfigs* feature_configs) {
  assert(rec->Field(0) != NULL);

  const gstGeode *geode = static_cast<const gstGeode*>(&(*geodeh));

  if (geode->TotalVertexCount() > USHRT_MAX)
    return false;

  // numLevel has been replaced by bitFlags
  pakdata->bitFlags = 0;
  StyleConfig::AltitudeMode altitudeMode =
      feature_configs->config.style.altitudeMode;

  if (altitudeMode == StyleConfig::Relative) {
    pakdata->bitFlags = 1;
  } else if (altitudeMode == StyleConfig::Absolute) {
    pakdata->bitFlags = 2;
  }

  if (feature_configs->config.style.extrude == true)
    pakdata->bitFlags |= 0x04;

  if (rec->Field(0)->IsEmpty()) {
    pakdata->name.set("");
  } else {
    pakdata->name.set(rec->Field(0)->ValueAsString().c_str());
  }

  pakdata->texId_deprecated = 0;

  pakdata->style = feature_configs->config.style.id;

  pakdata->numPt = geode->VertexCount(0);

  pakdata->localPt = static_cast<etVec3d*>(mem_pool_.Allocate(
                                               geode->VertexCount(0) *
                                               sizeof(etVec3d)));

  double zValue = 0;
  bool enableCustomHeight = feature_configs->config.style.enableCustomHeight;
  if (altitudeMode != StyleConfig::ClampToGround &&
      enableCustomHeight) {
    // expanded value of height field
    gstValue* height_var = rec->Field(1);
    assert(height_var != NULL);
    if (height_var) {
      double heightVal = height_var->ValueAsDouble();
      double scale = feature_configs->config.style.customHeightScale;
      double offset = feature_configs->config.style.customHeightOffset;
      zValue = (heightVal * scale + offset) / khEarthRadius;
    }
  }

  for (unsigned int pt = 0; pt < geode->VertexCount(0); ++pt) {
    const gstVertex& vert = geode->GetVertex(0, pt);
    pakdata->localPt[pt].elem[0] = vert.x * 2. - 1.;
    pakdata->localPt[pt].elem[1] = vert.y * 2. - 1.;
    if (altitudeMode == StyleConfig::ClampToGround) {
      pakdata->localPt[pt].elem[2] = 0;
    } else {
      if (enableCustomHeight) {
        pakdata->localPt[pt].elem[2] = zValue;
      } else {
        pakdata->localPt[pt].elem[2] = vert.z / khEarthRadius;
      }
    }
  }

  return true;
}


bool gstPacketFileExporter::AddPolyline(etPolyLinePacketData* pakdata,
                                 const gstGeodeHandle &geodeh,
                                 gstRecordHandle rec,
                                 const gstFeatureConfigs* feature_configs) {
  const gstGeode *geode = static_cast<const gstGeode*>(&(*geodeh));

  if (geode->TotalVertexCount() > USHRT_MAX)
    return false;

  // pakdata->name.set(geode->name() ? geode->name() : "");
  pakdata->name.set("");

  // not used by polylines
  pakdata->texId = 0;

  // numLevel has been replaced by bitFlags
  pakdata->bitFlags = 0;
  StyleConfig::AltitudeMode altitudeMode =
      feature_configs->config.style.altitudeMode;

  if (altitudeMode == StyleConfig::Relative) {
    pakdata->bitFlags = 1;
  } else if (altitudeMode == StyleConfig::Absolute) {
    pakdata->bitFlags = 2;
  }

  if (feature_configs->config.style.extrude == true)
    pakdata->bitFlags |= 0x04;

  pakdata->style = feature_configs->config.style.id;

  pakdata->numPt = geode->VertexCount(0);

  pakdata->localPt = static_cast<etVec3d*>(mem_pool_.Allocate(
                                               geode->VertexCount(0) *
                                               sizeof(etVec3d)));

  double zValue = 0;
  bool enableCustomHeight = feature_configs->config.style.enableCustomHeight;
  if (altitudeMode != StyleConfig::ClampToGround &&
      enableCustomHeight) {
    // expanded height field
    gstValue* height_var = rec->Field(1);
    assert(height_var != NULL);
    if (height_var) {
      double heightVal = height_var->ValueAsDouble();
      double scale = feature_configs->config.style.customHeightScale;
      double offset = feature_configs->config.style.customHeightOffset;
      zValue = (heightVal * scale + offset) / khEarthRadius;
    }
  }

  for (unsigned int pt = 0; pt < geode->VertexCount(0); ++pt) {
    const gstVertex& vert = geode->GetVertex(0, pt);
    pakdata->localPt[pt].elem[0] = vert.x * 2. - 1.;
    pakdata->localPt[pt].elem[1] = vert.y * 2. - 1.;
    if (altitudeMode == StyleConfig::ClampToGround) {
      pakdata->localPt[pt].elem[2] = 0;
    } else {
      if (enableCustomHeight) {
        pakdata->localPt[pt].elem[2] = zValue;
      } else {
        pakdata->localPt[pt].elem[2] = vert.z / khEarthRadius;
      }
    }
  }

  return true;
}


bool gstPacketFileExporter::AddPolygon(etPolygonPacketData* pakdata,
                                const gstGeodeHandle &geodeh,
                                gstRecordHandle rec,
                                const gstFeatureConfigs* feature_configs) {
  if (geodeh->PrimType() == gstMultiPolygon ||
      geodeh->PrimType() == gstMultiPolygon25D ||
      geodeh->PrimType() == gstMultiPolygon3D) {
    notify(NFY_FATAL, "%s: Improper feature type %d.",
           __func__, geodeh->PrimType());
  }

  const gstGeode *geode = static_cast<const gstGeode*>(&(*geodeh));

  notify(NFY_DEBUG,
         "gstPacketFileExporter::AddPolygon(): Type %d, TotalVertexCount: %u",
         geode->PrimType(), geode->TotalVertexCount());

  if (geode->TotalVertexCount() > USHRT_MAX)
    return false;

  // pakdata->name.set( geode->name() ? geode->name() : "" );
  pakdata->name.set("");

  // numLevel has been replaced by bitFlags
  pakdata->bitFlags = 0;
  StyleConfig::AltitudeMode altitudeMode =
      feature_configs->config.style.altitudeMode;

  if (altitudeMode == StyleConfig::Relative)
    pakdata->bitFlags = 1;
  else if (altitudeMode == StyleConfig::Absolute)
    pakdata->bitFlags = 2;

  if (feature_configs->config.style.extrude == true)
    pakdata->bitFlags |= 0x04;

  pakdata->style = feature_configs->config.style.id;
  pakdata->properties = 0;

  pakdata->numPt = geode->VertexCount(0);

  pakdata->localPt = static_cast<etVec3d*>(mem_pool_.Allocate(
                                               geode->VertexCount(0) *
                                               sizeof(etVec3d)));


  double zValue = 0;
  bool enableCustomHeight = feature_configs->config.style.enableCustomHeight;
  if (altitudeMode != StyleConfig::ClampToGround && enableCustomHeight) {
    // expanded height field
    gstValue* height_var = rec->Field(1);
    assert(height_var != NULL);
    if (height_var) {
      double heightVal = height_var->ValueAsDouble();
      double scale = feature_configs->config.style.customHeightScale;
      double offset = feature_configs->config.style.customHeightOffset;
      zValue = (heightVal * scale + offset) / khEarthRadius;
    }
  }

  for (unsigned int pt = 0; pt < geode->VertexCount(0); ++pt) {
    const gstVertex& vert = geode->GetVertex(0, pt);
    pakdata->localPt[pt].elem[0] = vert.x * 2. - 1.;
    pakdata->localPt[pt].elem[1] = vert.y * 2. - 1.;
    if (altitudeMode == StyleConfig::ClampToGround) {
      pakdata->localPt[pt].elem[2] = 0;
    } else {
      if (enableCustomHeight) {
        pakdata->localPt[pt].elem[2] = zValue;
      } else {
        pakdata->localPt[pt].elem[2] = vert.z / khEarthRadius;
      }
    }
  }

  pakdata->numEdgeFlags = geode->VertexCount(0);

  const std::vector<std::int8_t>& flags = geode->EdgeFlags();
  pakdata->edgeFlags = static_cast<bool*>(mem_pool_.Allocate(
                                              pakdata->numEdgeFlags * sizeof(bool)));
  for (unsigned int i = 0; i < pakdata->numEdgeFlags; ++i) {
    pakdata->edgeFlags[i] = (flags[i] != kNormalEdge);
  }

  return true;
}

#ifndef NDEBUG
#include <ctype.h>
void dumptext(const char* prefix, const char* src) {
  char tmp[strlen(src)*4];
  const char* srcEnd = src + strlen(src);
  char* dest = tmp;
  while (src < srcEnd) {
    if (isprint(*src)) {
      *dest++ = *src;
    } else {
      int len = sprintf(dest, "%%%02x;", *src);
      dest += len;
    }
    ++src;
  }
  *dest = 0;
  fprintf(stderr, "%s: %s\n", prefix, tmp);
}
#endif

bool gstPacketFileExporter::AddLandmark(etLandmarkPacketData* pakdata,
                                 gstVertex vert,
                                 gstRecordHandle rec,
                                 const gstSite* site) {
  char * empty = static_cast<char*>(mem_pool_.Allocate(2));
  empty[0] = '\0';
  empty[1] = '\0';
  unsigned char u_empty[1] = { 0 };

  // numLevel has been replaced by bitFlags
  pakdata->bitFlags = 0;
  StyleConfig::AltitudeMode altitudeMode = site->config.style.altitudeMode;
  if (altitudeMode == StyleConfig::Relative) {
    pakdata->bitFlags = 1;
  } else if (altitudeMode == StyleConfig::Absolute) {
    pakdata->bitFlags = 2;
  }

  if (site->config.style.extrude == true)
    pakdata->bitFlags |= 0x04;

  pakdata->style = site->config.style.id;

  // amin: Starting fusion version 2.6 we are no longer supporting Keyhole 2.x
  // clients and therefore do not need the icon names in the packets.
  pakdata->iconName_deprecated = empty;

  // Label & popup dialog line #1
  gstValue* label = rec->Field(0);
  assert(label != NULL);
  if (!label->IsEmpty())
    pakdata->name.set(label->ValueAsString().c_str());
  else
    pakdata->name.set("");

  // description
  gstValue* desc = rec->Field(1);
  assert(desc != NULL);
  if (!desc->IsEmpty()) {
    std::string src = desc->ValueAsString();
    size_t sz = src.length();
    char * ptr = static_cast<char*>(mem_pool_.Allocate(sz + 1));
    strncpy(ptr, src.c_str(), sz + 1);
    while (sz && isspace(ptr[sz - 1])) {
      ptr[sz - 1] = '\0';
      --sz;
    }
    pakdata->description = ptr;
  } else {
    pakdata->description = empty;
  }

  // temporary
  pakdata->reference = u_empty;
  pakdata->referenceSize = 0;
  pakdata->referenceType = RFTYPE_NONE;

  pakdata->numPt = 1;

  pakdata->localPt = static_cast<etVec3d*>(mem_pool_.Allocate(sizeof(etVec3d)));

  pakdata->localPt[0].elem[0] = vert.x * 2. - 1.;
  pakdata->localPt[0].elem[1] = vert.y * 2. - 1.;
  if (altitudeMode == StyleConfig::ClampToGround) {
    pakdata->localPt[0].elem[2] = 0;
  } else {
    bool enableCustomHeight = site->config.style.enableCustomHeight;
    if (enableCustomHeight) {
      gstValue* height_var = rec->Field(2);
      assert(height_var != NULL);
      if (height_var) {
        double heightVal = height_var->ValueAsDouble();
        double scale = site->config.style.customHeightScale;
        double offset = site->config.style.customHeightOffset;
        pakdata->localPt[0].elem[2] = (heightVal * scale + offset) /
                                      khEarthRadius;
      }
    } else {
      pakdata->localPt[0].elem[2] = vert.z / khEarthRadius ;
    }
  }

  return true;
}


bool gstPacketFileExporter::Save() {
  std::uint32_t actualSize = lump_pak_->save(write_buffer_, kMaxPacketSize);

  notify(NFY_DEBUG, "Saving quad packet size:%d level:%u row:%u col:%u",
         actualSize, pak_addr_.level(), pak_addr_.row(), pak_addr_.col());

  if (actualSize < lump_pak_->packetHeader.dataBufferOffset +
      lump_pak_->packetHeader.dataBufferSize) {
    notify(NFY_WARN, "Saving packet has a mismatched size,  %u != %lu",
           actualSize,
           static_cast<long unsigned>(lump_pak_->packetHeader.dataBufferOffset
                                      +lump_pak_->packetHeader.dataBufferSize));
    return false;
  }

  if (actualSize > kMaxPacketSize) {
    notify(NFY_WARN, "Extending past max %d bytes,  actual size = %d",
           kMaxPacketSize, actualSize);
    return false;
  }

  size_t compressed_size = kCompressedBufferSize;
  if (!KhPktCompressWithBuffer(write_buffer_, actualSize,
                               &compressed_buffer_[0],
                               &compressed_size)) {
    notify(NFY_WARN, "Unable to compress %u bytes", (unsigned)compressed_size);
    return false;
  }

  writer_->WriteAppendCRC(QuadtreePath(pak_addr_.level(), pak_addr_.row(),
                                       pak_addr_.col()),
                          &compressed_buffer_[0],
                          compressed_size+kCRC32Size);

  return true;
}

void gstPacketFileExporter::Close() {
  writer_->Close();
}


gstExporter* NewPacketFileExporter(geFilePool &file_pool,
                                   const std::string& path) {
  return new gstPacketFileExporter(file_pool, path);
}
