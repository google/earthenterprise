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


#include "common/serverdb/map_tile_utils.h"

#include "common/tile_resampler.h"
#include "common/geGdalUtils.h"
#include "common/generic_utils.h"

#include "common/proto_streaming_imagery.h"
#include "common/khTileAddr.h"
#include "common/khSimpleException.h"

namespace {

const int kPngDataNumChannels = 4;
const int kPngAlphaNumChannels = 1;
const int kJpegNumChannels = 3;

// Auxiliary function to validate accepted types of image data in GE protobuf
// Imagery packet.
bool IsValidGePbImageType(
    const ::keyhole::EarthImageryPacket::Codec& image_type) {
  switch (image_type) {
    case ::keyhole::EarthImageryPacket::JPEG:
      return true;

    // Not expecting a jpeg2000 and png.
    case ::keyhole::EarthImageryPacket::JPEG2000:
    case ::keyhole::EarthImageryPacket::PNG_RGBA:
      return false;

    default:
      return false;
  }
}

// Auxiliary function to validate accepted types of alpha data in GE protobuf
// Imagery packet.
bool IsValidGePbAlphaType(
    const ::keyhole::EarthImageryPacket::SeparateAlphaType& image_type) {
  switch (image_type) {
    case ::keyhole::EarthImageryPacket::PNG:
      return true;
    default:
      return false;
  }
}

// Auxiliary function to validate accepted GE protobuf imagery packet.
// Function throws khSimpleNotFoundException in case of invalid packet.
void ValidateGePbImageryPacket(const geEarthImageryPacket& imagery_pb) {
  if (!IsValidGePbImageType(imagery_pb.ImageType())) {
    throw khSimpleNotFoundException(
        "Unsupported type of image data in imagery packet.");
  }

  if (imagery_pb.HasImageAlpha() &&
      (!IsValidGePbAlphaType(imagery_pb.GetAlphaType()))) {
    throw khSimpleNotFoundException(
        "Unsupported type of alpha data in imagery packet.");
  }
}

// Resamples jpeg tile and returns in input buffer.
// Function throws khSimpleNotFoundException on failure.
void ResampleJpegTile(std::string* const buffer,
                      const QuadtreePath& qpath_parent,
                      const QuadtreePath& qpath_child) {
  TileResampler tile_resampler(ImageryQuadnodeResolution, kJpegNumChannels);

  // This code nicely re-samples the tile and returns with the
  // re-sampled tile in the input buffer. If it returns false,
  // something bad happened.
  if (!tile_resampler.ResampleTileInString(buffer, qpath_parent, qpath_child)) {
    throw khSimpleNotFoundException("Tile not found due to re-sampling "
                                    "error ")
        << "'" << qpath_parent.AsString() << "'"
        << "from" << "'" << qpath_child.AsString() << "'.";
  }
}

// Resamples 4-band PNG tile and returns in input buffer.
// Function throws khSimpleNotFoundException on failure.
void ResamplePngTile(std::string* const buffer,
                         const QuadtreePath& qpath_parent,
                         const QuadtreePath& qpath_child) {
  const bool IS_PNG = true;  // create PNG resampler.
  TileResampler tile_resampler(ImageryQuadnodeResolution,
                               kPngDataNumChannels,
                               IS_PNG);

  // This code nicely re-samples the tile and returns with the
  // re-sampled tile in the input buffer. If it returns false,
  // something bad happened.
  if (!tile_resampler.ResampleTileInString(buffer, qpath_parent, qpath_child)) {
    throw khSimpleNotFoundException("Tile not found due to re-sampling "
                                    "error ")
        << "'" << qpath_parent.AsString() << "'"
        << "from" << "'" << qpath_child.AsString() << "'.";
  }
}

// Resamples 1-band PNG tile and returns in input buffer.
// Function throws khSimpleNotFoundException on failure.
void ResamplePngAlphaTile(std::string* const buffer,
                          const QuadtreePath& qpath_parent,
                          const QuadtreePath& qpath_child) {
  const bool IS_PNG = true;  // create PNG resampler.
  TileResampler tile_resampler(ImageryQuadnodeResolution,
                               kPngAlphaNumChannels,
                               IS_PNG);

  // This code nicely re-samples the tile and returns with the
  // re-sampled tile in the input buffer. If it returns false,
  // something bad happened.
  if (!tile_resampler.ResampleTileInString(buffer, qpath_parent, qpath_child)) {
    throw khSimpleNotFoundException("Tile not found due to re-sampling "
                                    "error ")
        << "'" << qpath_parent.AsString() << "'"
        << "from" << "'" << qpath_child.AsString() << "'.";
  }
}

// Writes Imagery packet to the buffer in specified format.
// If format is not specified (an empty string), write in either jpeg-format
// (there is no alpha-data) or png-format (there is alpha-data).
// image_data: jpeg buffer of image data.
//     NOTE: The image_data will be destroyed since we use std::swap to copy
//     input buffer to output one.
// image_alpha: jpeg buffer of alpha data.
const MimeType ReportImageryTile(std::string* const image_data,
                                 const std::string* image_alpha,
                                 std::string* const buf,
                                 const std::string& format) {
  // Check that input and output buffers are different ones since we blend
  // to output buffer.
  assert(image_data != buf);

  MimeType content_type = MapTileUtils::kNotFoundMimeType;
  if ((format.empty() && image_alpha) ||
      format == MapTileUtils::kPngMimeType.GetLiteral()) {
    content_type = MapTileUtils::kPngMimeType;
    geGdalUtils::BlendToPng(*image_data, image_alpha, buf);
  } else {
    // NOTE: using swap for faster copying (no memory deallocation/allocation).
    std::swap(*buf, *image_data);
    content_type = MapTileUtils::kJpegMimeType;
  }
  return content_type;
}

}  // namespace


// Mime type constants
const MimeType MapTileUtils::kUnknownMimeType("unknown");
const MimeType MapTileUtils::kOctetMimeType("application/octet-stream");
const MimeType MapTileUtils::kJpegMimeType("image/jpeg");
const MimeType MapTileUtils::kPngMimeType("image/png");
const MimeType MapTileUtils::kJavascriptMimeType("text/javascript");
const MimeType MapTileUtils::kRedirectMimeType("redirect");
const MimeType MapTileUtils::kNotFoundMimeType("not found");

const char* const MapTileUtils::kNoDataJpg = "/maps/mapfiles/empty.jpg";
const char* const MapTileUtils::kNoDataPng = "/maps/mapfiles/transparent.png";
const char* const MapTileUtils::kOutOfCutJpg = "/maps/mapfiles/out_of_cut.jpg";
const char* const MapTileUtils::kOutOfCutPng = "/maps/mapfiles/out_of_cut.png";


const MimeType MapTileUtils::PrepareImageryMapsTile(
    const QuadtreePath& qpath_parent,
    const QuadtreePath& qpath,
    std::string* const buf,
    const bool report_in_source_format,
    const std::string& format) {
  MimeType content_type = MapTileUtils::kNotFoundMimeType;

  const char* buffer = buf->data();
  bool found_non_pb_image = false;
  bool is_png_image = false;
  // Check for jpeg.
  if (gecommon::IsJpegBuffer(buffer)) {
    content_type = MapTileUtils::kJpegMimeType;
    found_non_pb_image = true;
  } else if (gecommon::IsPngBuffer(buffer)) {
    content_type = MapTileUtils::kPngMimeType;
    found_non_pb_image = true;
    is_png_image = true;
  }

  std::string* image_data = 0;
  std::string* image_alpha = 0;
  geEarthImageryPacket imagery_pb;

  if (found_non_pb_image) {
    // Note: if we need to resample below, then it will be
    // resampled in source buffer.
    image_data = buf;
  } else {
    if (!(imagery_pb.ParseFromString(*buf) && imagery_pb.HasImageData())) {
      throw khSimpleNotFoundException(
          "Expected imagery protocol buffer in tile ") <<
          "'" << qpath.AsString() << "'.";
    }

    // Validates GE protobuf packet.
    ValidateGePbImageryPacket(imagery_pb);

    image_data = imagery_pb.MutableImageData();

    if (imagery_pb.HasImageAlpha()) {
      image_alpha = imagery_pb.MutableImageAlpha();
    }
  }

  // Need to check if the tile we got was from the requested path, if not,
  // we need to re-sample it.
  if (qpath_parent != qpath) {
    if (found_non_pb_image) {
      if (is_png_image) {
        assert(content_type.Equals(MapTileUtils::kPngMimeType));
        // The ResamplePngTile throws khSimpleNotFoundException on failure.
        ResamplePngTile(image_data, qpath_parent, qpath);
      } else {
        assert(content_type.Equals(MapTileUtils::kJpegMimeType));
        // The ResampleJpegTile throws khSimpleNotFoundException on failure.
        ResampleJpegTile(image_data, qpath_parent, qpath);
      }
    } else {
      // The ResampleJpegTile throws khSimpleNotFoundException on failure.
      ResampleJpegTile(image_data, qpath_parent, qpath);

      if (image_alpha) {
        // The ResamplePngAlphaTile throws khSimpleNotFoundException on failure.
        ResamplePngAlphaTile(image_alpha, qpath_parent, qpath);
      }
    }
  }

  if (report_in_source_format) {
    if (!found_non_pb_image) {
      // Update source buffer in case there have been resampling.
      if (qpath_parent != qpath) {
        // Serialize resampled GE protobuf tile to source buffer.
        buf->resize(imagery_pb.ByteSize());
        imagery_pb.SerializeToArray(&(*buf)[0], buf->size());
      }
      content_type = MapTileUtils::kOctetMimeType;
    }
  } else {
    // Note: override content type depends on specified format.
    if (found_non_pb_image) {
      // Note: here we get either jpeg or png.
      // Convert to png if requested format is png, while source buffer is jpeg,
      // or convert to jpeg if requested format is jpeg, while source buffer is
      // png, otherwise report as is.
      if (format == MapTileUtils::kPngMimeType.GetLiteral() &&
          !is_png_image) {
        std::string image_blend;
        geGdalUtils::BlendToPng(*image_data, NULL, &image_blend);
        std::swap(*buf, image_blend);
        content_type = MapTileUtils::kPngMimeType;
      } else if (format == MapTileUtils::kJpegMimeType.GetLiteral() &&
                 is_png_image) {
        std::string image_out;
        geGdalUtils::ConvertToJpeg(*image_data, &image_out);
        std::swap(*buf, image_out);
        content_type = MapTileUtils::kJpegMimeType;
      }
    } else {  // protobuf imagery tile.
      // Convert GE protobuf tile to jpeg/png depends on presence of alpha-data
      // and requested format.
      content_type = ReportImageryTile(image_data, image_alpha, buf, format);
    }
  }
  return content_type;
}
