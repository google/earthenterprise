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


#include "fusion/tools/polymask.h"

#include <assert.h>
#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "common/khFileUtils.h"
#include "common/notify.h"
#include "fusion/khgdal/khgdal.h"
#include "fusion/khgdal/khGeoExtents.h"
#include "fusion/khgdal/khGDALDataset.h"
#include "fusion/khraster/khRasterProduct.h"
#include "fusion/tools/geImageWriter.h"
#include "fusion/tools/Featherer.h"

namespace fusion_tools {

const std::string PolyMask::kMaskFormat = "GTiff";

PolyMask::PolyMask() {
  mask_format_ = kMaskFormat;
  input_file_type_ = UNDEFINED;

  // Load the output driver.
  khGDALInit();
  driver_ = reinterpret_cast<GDALDriver*>(
      GDALGetDriverByName(mask_format_.c_str()));

  if (!driver_) {
    notify(NFY_FATAL, "Internal Error: Unable to load GDAL driver %s",
           mask_format_.c_str());
  }

  feather_ = 0;
  feather_border_ = true;
}

// Adds command to set the feather to be used in subsequent commands.
// feather: Feather can be negative, positive, or 0.
void PolyMask::AddCommandSetFeather(int feather) {
  commands_.push_back(SET_FEATHER);
  command_int_args_.push_back(feather);
}

// Adds command to set the flag whether borders should be feathered
// to be used in subsequent commands.
// feather_border_flag: If not zero, border will be feathered.
void PolyMask::AddCommandSetFeatherBorder(int feather_border_flag) {
  if (feather_border_flag) {
    commands_.push_back(FEATHER_BORDER_ON);
  } else {
    commands_.push_back(FEATHER_BORDER_OFF);
  }
}

// Adds command to set the base mask image. The base mask image provides
// the initial mask as well as the dimensions and the geo extent
// of the final mask.
// base_mask_file: Geotiff to be used as the starting mask.
void PolyMask::AddCommandSetBaseMask(std::string base_mask_file) {
  commands_.push_back(SET_BASE_MASK);
  command_string_args_.push_back(base_mask_file);
  // Get the dimensions and geo extent of the mask now
  ImageInfo(base_mask_file, &mask_width_, &mask_height_, &geo_extent_,
            &mask_projection_);
}

// Adds command to set the base image. The base image provides the
// dimensions and the geo extent of the final mask.
// base_image_file: Geotiff that determines dimensions of starting mask.
void PolyMask::AddCommandSetBaseImage(std::string base_image_file) {
  commands_.push_back(SET_BASE_IMAGE);
  command_string_args_.push_back(base_image_file);
  // Get the dimensions and geo extent of the mask now
  ImageInfo(base_image_file, &mask_width_, &mask_height_, &geo_extent_,
            &mask_projection_);
}

// Adds command to invert the current mask created so far.
void PolyMask::AddCommandInvert() {
  commands_.push_back(INVERT);
}

// Adds command to threshold the current mask created so far.
void PolyMask::AddCommandThreshold(int threshold) {
  commands_.push_back(THRESHOLD);
  command_int_args_.push_back(threshold);
}

// Adds the command to AND the negative image of the mask in the mask file
// with the current mask.
// mask_file: If polygons, should be .kml or .shp. If it is a raster mask,
//            then it should be a .tif file.
void PolyMask::AddCommandAndMask(std::string mask_file) {
  commands_.push_back(AND_MASK);
  command_string_args_.push_back(mask_file);
}

// Adds the command to OR in the mask in the mask file with the current mask.
// mask_file: If polygons, should be .kml or .shp. If it is a raster mask,
//            then it should be a .tif file.
void PolyMask::AddCommandOrMask(std::string mask_file) {
  commands_.push_back(OR_MASK);
  command_string_args_.push_back(mask_file);
}

// Adds the command to diff the mask in the mask file with the current mask.
// mask_file: If polygons, should be .kml or .shp. If it is a raster mask,
//            then it should be a .tif file.
void PolyMask::AddCommandDiffMask(std::string mask_file) {
  commands_.push_back(DIFF_MASK);
  command_string_args_.push_back(mask_file);
}

// Sets base image. Provides the dimensions and the geo extent
// of the final mask.
// output_file: Geotiff file that will receive the final mask.
void PolyMask::AddCommandSetOutputMask(std::string mask_file) {
  commands_.push_back(SET_OUTPUT_MASK);
  // Set it now so we can use it as a working space
  output_file_ = mask_file;
}

// Builds mask by executing commands in the order they were given.
void PolyMask::BuildMask() {
  size_t int_arg_idx = 0;
  size_t string_arg_idx = 0;
  bool base_in_place = false;

  size_t image_size = (size_t)mask_width_ * (size_t)mask_height_;

  // geotiff.cpp has an image size check against 4GB and the actual limit used
  // there is 4200000000
  if(image_size > 4200000000) {
    notify(NFY_FATAL,
      "A %d pixels x %d lines mask would be too large. Creation failed.",
       mask_width_, mask_height_);
  }
  // Buffer for the actual mask being built
  std::vector<unsigned char> mask_data(image_size);
  // Data buffer for loading masks from files
  std::vector<unsigned char> data_buffer(image_size);

  std::string file_name;
  unsigned char threshold = 0x00;
  for (size_t i = 0; i < commands_.size(); ++i) {
    switch (commands_[i]) {
      // Load the given mask as the basis upon which the new mask is created.
      case SET_BASE_MASK:
        base_mask_file_ = command_string_args_[string_arg_idx++];
        notify(NFY_NOTICE, "Setting base mask %s.", base_mask_file_.c_str());

        // Load the mask as the initial bask upon which to build.
        LoadMask(base_mask_file_, &mask_data, mask_width_, mask_height_);

        // Feather base mask if we have a non-zero feather.
        FeatherMaskData(&mask_data, feather_, 0x00);
        base_in_place = true;
        break;

      // Create an empty mask as the basis upon which the new mask is created.
      case SET_BASE_IMAGE:
        base_image_file_ = command_string_args_[string_arg_idx++];
        notify(NFY_NOTICE, "Setting base image %s.", base_image_file_.c_str());

        FillBuffer(&mask_data, 0);

        // Feather image if we have a non-zero feather.
        FeatherMaskData(&mask_data, feather_, 0xff);
        base_in_place = true;
       break;

      // Set feather to be used in subsequent commands.
      case SET_FEATHER:
        feather_ = command_int_args_[int_arg_idx++];
        notify(NFY_NOTICE, "Setting feather %d.", feather_);
        break;

      // Turn on feathering of border.
      case FEATHER_BORDER_ON:
        feather_border_ = true;
        notify(NFY_NOTICE, "Feather border is on.");
        break;

      // Turn off feathering of border.
      case FEATHER_BORDER_OFF:
        feather_border_ = false;
        notify(NFY_NOTICE, "Feather border is off.");
        break;

      // Invert the mask generated thus far.
      case INVERT:
        if (!base_in_place) {
          notify(NFY_FATAL, "Base mask has not been created.");
        }
        notify(NFY_NOTICE, "Inverting current mask.");
        InvertImage(&mask_data);
        break;

      // Threshold the mask generated thus far.
      case THRESHOLD:
        threshold = static_cast<unsigned char>(0xff & command_int_args_[int_arg_idx++]);
        if (!base_in_place) {
          notify(NFY_FATAL, "Base mask has not been created.");
        }
        ThresholdImage(&mask_data, threshold);
        break;

      // AND, OR or DIFF a mask with the new mask.
      case AND_MASK:
      case OR_MASK:
      case DIFF_MASK:
        if (!base_in_place) {
          notify(NFY_FATAL, "Base mask has not been created.");
        }
        file_name = command_string_args_[string_arg_idx++];
        BlendMasks(commands_[i],
                   file_name,
                   feather_,
                   &data_buffer,
                   &mask_data);
        break;

      // Save the new mask to the specified file.
      // TODO: Get rid of this commmand. Instead, just save the
      // TODO: mask at the end of the build. Fix progressive
      // TODO: test so it builds progressively by adding commands.
      case SET_OUTPUT_MASK:
        if (!base_in_place) {
          notify(NFY_FATAL, "Base mask has not been created.");
        }
        notify(NFY_NOTICE, "Saving mask to %s.", output_file_.c_str());

        // Save final mask
        SaveMask(&mask_data, mask_width_, mask_height_, geo_extent_);
        break;
    }
  }
}

// Add or subtract mask in given file to or from the given mask data. Mask
// is feathered by the given amount before it is blended with the given mask
// data.
// blend_type: Type of blend to perform on the two masks.
// file_name: If polygons, should be .kml or .shp. If it is a raster mask,
//            then it should be a .tif file.
// feather: Feather that should be applied to the mask before it is added to or
//          subtracted from the given mask data. If the mask is to be
//          subtracted, the negative of the feather value is used.
// data_buffer: a pre-allocated buffer for loading masks from files
// mask_data: the mask being generated
void PolyMask::BlendMasks(const BuildMaskCommand blend_type,
                          const std::string file_name,
                          const int feather,
                          std::vector<unsigned char>* data_buffer,
                          std::vector<unsigned char>* mask_data) {
  // ".kml", ".shp", or ".tif"
  // TODO: make suffixes case insensitive
  std::string suffix = file_name.substr(file_name.size() - 4);

  // Add polygon mask.
  if ((suffix == ".kml") || (suffix == ".shp")) {
    // Create the polygon mask.
    FillBuffer(data_buffer, 0);
    SaveMask(data_buffer, mask_width_, mask_height_, geo_extent_);
    // Build polygon mask in the workspace file.
    ApplyPolygon(file_name, 0xff);
  } else if (suffix == ".tif") {
    // Load tiff mask and save it into the workspace file.
    LoadMask(file_name, data_buffer, mask_width_, mask_height_);
    // We are saving and reloading with no effect if feather is 0
    SaveMask(data_buffer, mask_width_, mask_height_, geo_extent_);
  } else {
    notify(NFY_FATAL, "Unable to add mask with suffix %s.", suffix.c_str());
  }

  if (blend_type == AND_MASK) {
    FeatherMask(-feather, 0x00);
  } else {
    FeatherMask(feather, 0x00);
  }
  LoadMask(output_file_, data_buffer, mask_width_, mask_height_);
  if (blend_type == AND_MASK) {
    InvertImage(data_buffer);
  }

  // Blend the mask with the mask being built.
  switch (blend_type) {
    case AND_MASK:
    case OR_MASK:
      LogicalOpDataAndBuffer(*data_buffer, mask_data, blend_type);
      break;

    case DIFF_MASK:
      notify(NFY_NOTICE, "Diff mask with a new mask.");
      DiffDataAndBuffer(*data_buffer, mask_data);
      break;

    default:
      notify(NFY_FATAL, "Illegal blend command.");
  }
}

// Fill buffer with the given value.
void PolyMask::FillBuffer(std::vector<unsigned char>* buffer, unsigned char byte) {
  for (size_t i = 0; i < buffer->size(); ++i) {
    (*buffer)[i] = byte;
  }
}

// Apply logical operator to bytes from data and the given buffer and replaces
// the byte in the buffer.
void PolyMask::LogicalOpDataAndBuffer(const std::vector<unsigned char>& data,
                                      std::vector<unsigned char>* buffer,
                                      BuildMaskCommand logic_op) {
  // Give warning or fail if bytes are not 0x00 or 0xff?
  switch (logic_op) {
    case AND_MASK:
      notify(NFY_NOTICE, "AND-ing mask with new mask.");
      for (size_t i = 0; i < buffer->size(); ++i) {
        (*buffer)[i] &= data[i];
      }
      break;

    case OR_MASK:
      notify(NFY_NOTICE, "OR-ing mask with new mask.");
      for (size_t i = 0; i < buffer->size(); ++i) {
        (*buffer)[i] |= data[i];
      }
      break;

    default:
      notify(NFY_FATAL, "Unknown logic operator.");
  }
}

// Diff data and the given buffer. If the values are equal, set the pixel to
// 0x00, otherwise, 0xff.
void PolyMask::DiffDataAndBuffer(const std::vector<unsigned char>& data,
                                 std::vector<unsigned char>* buffer) {
  for (size_t i = 0; i < buffer->size(); ++i) {
    if ((*buffer)[i] == data[i]) {
      (*buffer)[i] = 0x00;
    } else {
      (*buffer)[i] = 0xff;
    }
  }
}

// Inverts image uchars by xor-ing them with 0xff.
void PolyMask::InvertImage(std::vector<unsigned char>* image) {
  for (size_t i = 0; i < image->size(); ++i) {
    (*image)[i] ^= 0xff;
  }
}

// Threshold image.
void PolyMask::ThresholdImage(std::vector<unsigned char>* image,
                              const unsigned char threshold) {
  notify(NFY_NOTICE, "Threshold %d", static_cast<int>(threshold));
  for (size_t i = 0; i < image->size(); ++i) {
    if ((*image)[i] <= threshold) {
      (*image)[i] = 0x00;
    } else {
      (*image)[i] = 0xff;
    }
  }
}

// Loads mask data from given file. Data should be allocated to hold
// width * height bytes.
void PolyMask::LoadMask(const std::string mask_file,
                               std::vector<unsigned char>* alpha_buffer,
                               const int width,
                               const int height) {
  GDALDataset *dataset = reinterpret_cast<GDALDataset*>(
      GDALOpen(mask_file.c_str(), GA_ReadOnly));
  GDALRasterBand *alpha_band = dataset->GetRasterBand(1);

  if (alpha_band->RasterIO(GF_Read, 0, 0,
                           width, height,
                           &((*alpha_buffer)[0]),
                           width, height,
                           GDT_Byte, 0, width)
      == CE_Failure) {
    notify(NFY_FATAL, "Could not load mask.");
  }
}

// Saves the given alpha buffer to disk. Uses the object's mask width and
// height and the object's destination mask file.
void PolyMask::SaveMask(std::vector<unsigned char>* alpha_buffer,
                        const int mask_width,
                        const int mask_height,
                        const khGeoExtents geo_extents) {
  khSize<std::uint32_t> mask_size(mask_width, mask_height);
  const char* projection = mask_projection_.c_str();
  geImageWriter::WriteAlphaImage(mask_size,
                                 &((*alpha_buffer)[0]),
                                 driver_, output_file_,
                                 geo_extents,
                                 projection,
                                 NULL);
}

// Opens an image file to get its width, height, and its geo extent.
void PolyMask::ImageInfo(std::string image_file,
                         int* width,
                         int* height,
                         khGeoExtents* geo_extents,
                         std::string* psz_projection) {
  // Open the image file as a gdal dataset.
  khGDALDataset khdataset;
  std::string kh_dataset_error;
  try {
    khdataset = khGDALDataset(image_file, "" /* use file srs */);
  } catch(const std::exception &e) {
    kh_dataset_error = e.what();
  } catch(...) {
    kh_dataset_error = "Unknown error";
  }
  GDALDataset *dataset = khdataset;
  if (!dataset) {
    dataset = reinterpret_cast<GDALDataset*>(
        GDALOpen(image_file.c_str(), GA_ReadOnly));
    if (!dataset) {
      notify(NFY_FATAL,
             "Unable to open %s. Exception: %s. Even GDALOpen failed!",
             image_file.c_str(), kh_dataset_error.c_str());
    }
  }

  // Get basic size info from image.
  *width = dataset->GetRasterXSize();
  *height = dataset->GetRasterYSize();

  // Get driver for image file.
  GDALDriver *image_driver = dataset->GetDriver();

  // Get geo extent of image file.
  if (khdataset) {
    *geo_extents = khdataset.geoExtents();
  } else {
    double geoTransform[6];
    if (dataset->GetGeoTransform(geoTransform) == CE_None) {
      khSize<std::uint32_t> rasterSize(*width, *height);
      *geo_extents = khGeoExtents(geoTransform, rasterSize);
    }
  }

  // Get the projection string of image file
  if (khdataset) {
    char *wkt_srs;
    khdataset.ogrSRS().exportToWkt(&wkt_srs);
    *psz_projection = wkt_srs;
    CPLFree(wkt_srs);
  } else {
    *psz_projection = dataset->GetProjectionRef();
  }

  if (geo_extents->empty()) {
    notify(NFY_FATAL, "Unable to determine geo extent of %s.",
           image_file.c_str());
  }

  notify(NFY_NOTICE, "%s width: %d height: %d",
         image_file.c_str(), mask_width_, mask_height_);
  notify(NFY_NOTICE, "north: %e south: %e east: %e west: %e",
         geo_extents->extents().north(),
         geo_extents->extents().south(),
         geo_extents->extents().east(),
         geo_extents->extents().west());
  notify(NFY_NOTICE, "File type: %s/%s",
         GDALGetDriverShortName(image_driver),
         GDALGetDriverLongName(image_driver));
  notify(NFY_NOTICE, "Projection : '%s'", psz_projection->c_str());
}

// Applies polygon in given file to the mask. If it is a positive mask, the
// polygon is filled with 0x00s, otherwise it is filled with 0xffs.
void  PolyMask::ApplyPolygon(const std::string& polygon_file,
                             const unsigned char pixel_byte) {
  char system_command[2048];

  // Directory containing the .kml or .shp file.
  std::string path = polygon_file.substr(0, polygon_file.rfind("/") + 1);
  // Name of layer within the .kml or .shp file.
  std::string layer = polygon_file.substr(path.size(),
                                          polygon_file.size()
                                          - path.size() - 4);
  // "kml" or "shp"
  std::string suffix = polygon_file.substr(polygon_file.size() - 4);
  // Full path less the dot suffix
  std::string base_file_path = path + layer;

  // If specified as a kml file, convert to an ESRI shp file.
  std::string polygon_shp_file;
  std::string shp_tmp_dir;
  if (suffix == ".kml") {
    ConvertKmlToShpFile(polygon_file, &polygon_shp_file, &layer, &shp_tmp_dir);
  } else if (suffix == ".shp") {
    polygon_shp_file = polygon_file;
  } else {
    notify(NFY_FATAL, "Polygon file must be .kml or .shp.");
  }

  // Burn the polygon into the mask file.
  if (snprintf(system_command, sizeof(system_command),
               "gdal_rasterize -b 1 -burn %d -l %s %s %s",
               pixel_byte, layer.c_str(),
               polygon_shp_file.c_str(), output_file_.c_str()) >=
      static_cast<int>(sizeof(system_command))) {
    notify(NFY_FATAL, "gdal_rasterize command is too long.");
  }
  notify(NFY_NOTICE, "Executing: %s", system_command);
  system(system_command);

  // Clean up if we used a temporary .shp file.
  if (suffix == ".kml") {
    khPruneDir(khDirname(polygon_shp_file));
  }
}

// Use ogr system tool for converting kml file to a temporary shp file.
void PolyMask::ConvertKmlToShpFile(const std::string polygon_kml_file,
                                      std::string* polygon_shp_file,
                                      std::string* layer,
                                      std::string* tmp_dir) {
  char path[128];
  snprintf(path, sizeof(path), "p%d", getpid());
  *tmp_dir = khCreateTmpDir(path);

  *polygon_shp_file = *tmp_dir + "/tmp.shp";
  *layer = "tmp";

  char system_command[2048];
  if (snprintf(system_command, sizeof(system_command),
               "ogr2ogr -t_srs '%s' %s %s",
               mask_projection_.c_str(),
               polygon_shp_file->c_str(), polygon_kml_file.c_str()) >=
      static_cast<int>(sizeof(system_command))) {
    notify(NFY_FATAL, "gdal_rasterize command is too long.");
  }
  notify(NFY_NOTICE, "Executing: %s", system_command);
  system(system_command);
}

// Feathers the mask edges using the in-memory featherer.
void PolyMask::FeatherMask(const int feather, const unsigned char border) {
  // Don't waste time if there is no feathering to do
  if (feather == 0) {
    return;
  }

  size_t image_size = (size_t)mask_width_ * (size_t)mask_height_;

  // geotiff.cpp has an image size check against 4GB and the actual limit used
  // there is 4200000000
  if(image_size > 4200000000) {
    notify(NFY_FATAL,
      "A %d pixels x %d lines mask would be too large. Creation failed.",
       mask_width_, mask_height_);
  }

  // Read the mask in so we can do the feathering.
  GDALDataset *dataset = reinterpret_cast<GDALDataset*>(
      GDALOpen(output_file_.c_str(), GA_Update));
  GDALRasterBand *alpha_band = dataset->GetRasterBand(1);
  std::vector<unsigned char> alpha_buffer(image_size);

  if (alpha_band->RasterIO(GF_Read, 0, 0,
                           mask_width_, mask_height_,
                           &alpha_buffer[0],
                           mask_width_, mask_height_,
                           GDT_Byte, 0, mask_width_)
      == CE_Failure) {
    notify(NFY_FATAL, "Could not reload mask.");
  }

  FeatherMaskData(&alpha_buffer, feather, border);

  // Try to write mask back out to disk.
  if (alpha_band->RasterIO(GF_Write, 0, 0,
                           mask_width_, mask_height_,
                           &alpha_buffer[0],
                           mask_width_, mask_height_,
                           GDT_Byte, 0, mask_width_)
      == CE_Failure) {
    notify(NFY_FATAL, "Could not save mask after feathering.");
  }

  // Clean up
  delete dataset;
}

// Feathers the mask edges using the in-memory featherer.
void PolyMask::FeatherMaskData(std::vector<unsigned char>* mask_data,
                               const int feather,
                               const unsigned char border) {
  // Don't waste time if there is no feathering to do
  if (feather == 0) {
    return;
  }

  int actual_border;   // actual border byte
  if (feather_border_) {
    actual_border = border;
  } else {
    actual_border = border ^ 0xff;
  }

  // A negative feather implies expanding rather than eroding.
  int actual_feather;  // actual feathering to use
  if (feather < 0) {
    actual_feather = -feather;
    actual_border = actual_border ^ 0xff;
  } else {
    actual_feather = feather;
  }

  // If negative feathering, invert image.
  if (feather < 0) {
    InvertImage(mask_data);
  }

  // Creating the object does the in-memory feathering.
  InMemoryFeatherer featherer(&(*mask_data)[0], mask_width_, mask_height_,
                              actual_feather, actual_border);

  // If negative feathering, invert image back to original polarity.
  if (feather < 0) {
    InvertImage(mask_data);
  }
}

// Check that the input file is readable.
void PolyMask::CheckInputFile(std::string input_file) {
  if (input_file_type_ != UNDEFINED) {
    notify(NFY_FATAL, "You should specify exactly one of either a --image "
           " or a --base_mask option.");
  }

  if (!input_file.empty() && !khIsFileReadable(input_file)) {
    notify(NFY_FATAL, "Cannot read input image file: %s",
           input_file.c_str());
  }
}

}  // namespace fusion_tools
