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

// Supports generation of masks from shape (.kml and .shp) and tiff files.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_TOOLS_POLYMASK_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_TOOLS_POLYMASK_H_

#include <string>
#include <vector>

#include "fusion/khgdal/khGeoExtents.h"
#include "common/base/macros.h"
#include "khgdal/khgdal.h"

namespace fusion_tools {

class PolyMaskTest;

// Creates a mask using geocoded polygons in .kml or .shp files and tiff masks
// added to or subtracted from a base mask determined by a image or mask
// geotiff file. If a base image is used, the geo extent and the dimensions are
// used to create an empty mask upon which other masks can be added.
//
// The polygon layer name in the .kml or .shp files should match the file name
// (e.g. mypolygon.kml should have the polygon in a layer called mypolygon).
//
// Relies on gdal_rasterize to fill the polygons. Polygons can extend beyond
// the extents of the image file. Those regions that extend beyond the image
// are ignored.
//
// This class is not safe for multi-threading.
class PolyMask {
 public:
  PolyMask();

  ~PolyMask() {}

  // Adds command to set the feather to be used in subsequent commands.
  // feather: Feather can be negative, positive, or 0
  void AddCommandSetFeather(int feather);

  // Adds command to set the flag whether borders should be feathered
  // to be used in subsequent commands.
  // feather_border_flag: If not zero, border will be feathered
  void AddCommandSetFeatherBorder(int feather_border_flag);

  // Adds command to set the base mask image. The base mask image provides
  // the initial mask as well as the dimensions and the geo extent
  // of the final mask.
  // bask_mask_file: Geotiff to be used as the starting mask
  void AddCommandSetBaseMask(std::string bask_mask_file);

  // Adds command to set the base image. The base image provides the
  // dimensions and the geo extent of the final mask.
  // bask_image_file: Geotiff that determines dimensions of starting mask
  void AddCommandSetBaseImage(std::string bask_image_file);

  // Adds command to invert the current mask created so far.
  void AddCommandInvert();

  // Adds the command to threshold the mask at the given threshold.
  // threshold: Threshold at or below which all pixels are set to 0x00. All
  //            other pixels are set to 0xff.
  void AddCommandThreshold(int threshold);

  // Adds the command to AND the negative image of the mask in the mask file
  // with the current mask.
  // mask_file: If polygons, should be .kml or .shp. If it is a raster mask,
  //            then it should be a .tif file.
  void AddCommandAndMask(std::string mask_file);

  // Adds the command to OR the mask in the mask file with the current mask.
  // mask_file: If polygons, should be .kml or .shp. If it is a raster mask,
  //            then it should be a .tif file.
  void AddCommandOrMask(std::string mask_file);

  // Adds the command to diff the mask in the mask file with the current mask.
  // mask_file: If polygons, should be .kml or .shp. If it is a raster mask,
  //            then it should be a .tif file.
  void AddCommandDiffMask(std::string mask_file);

  // Sets the file where the final mask will be saved. This file is also used
  // as a workspace while building the mask.
  // output_file: Geotiff file that will receive the final mask
  void AddCommandSetOutputMask(std::string output_file);

  // Builds mask based on the geo extent and dimensions of the base
  // mask or image file. Uses pre-parsed commands that are executed in the
  // order they were given to set feathers, to add and/or subtract polygon and
  // tiff masks, and to invert the generated mask at any given point.
  void BuildMask();

 private:
  friend class PolyMaskTest;

  // Default format for mask files
  static const std::string kMaskFormat;
  // Path to the mask output file
  std::string output_file_;
  // Format of the mask file
  std::string mask_format_;
  // Base mask that also defines geo extent and dimensions of generated mask
  std::string base_mask_file_;
  // Base image for defining geo extent and dimensions of generated mask
  std::string base_image_file_;

  // Types of files to use for creating the bask mask
  enum InputType {
    UNDEFINED = 0,
    IMAGE,
    MASK
  };

  // Build commands for creating a mask
  enum BuildMaskCommand {
    SET_BASE_MASK,
    SET_BASE_IMAGE,
    SET_FEATHER,
    FEATHER_BORDER_ON,
    FEATHER_BORDER_OFF,
    INVERT,
    THRESHOLD,
    AND_MASK,
    OR_MASK,
    DIFF_MASK,
    SET_OUTPUT_MASK,
  };

  // The commands for building the mask
  std::vector<BuildMaskCommand> commands_;
  // String arguments for the build commands
  std::vector<std::string> command_string_args_;
  // Integer arguments for the build commands
  std::vector<int> command_int_args_;

  // Whether base image file is a mask or an image
  InputType input_file_type_;

  // Current feather value to be used on the next pass
  int feather_;
  // Flag whether feather should include the border
  bool feather_border_;
  // Geo extent of the mask
  khGeoExtents geo_extent_;
  // Pixel width of the mask
  int mask_width_;
  // Pixel height of the mask
  int mask_height_;
  // mask projection in either proj4 or OGC-WKT
  std::string mask_projection_;

  // GDAL driver for writing the mask file
  GDALDriver* driver_;

  // Adds mask in the given file with the given feather to the mask data.
  void BlendMasks(const BuildMaskCommand blend_type,
                  const std::string file_name,
                  const int feather,
                  std::vector<unsigned char>* data_buffer,
                  std::vector<unsigned char>* mask_data);

  // Fills buffer with the given value.
  void FillBuffer(std::vector<unsigned char>* buffer, unsigned char byte);

  // Applies logical operator to bytes from data and the given buffer and
  // replaces the bytes in the buffer.
  void LogicalOpDataAndBuffer(const std::vector<unsigned char>& data,
                              std::vector<unsigned char>* buffer,
                              BuildMaskCommand logic_op);

  // If uchars in data vector are equal to those in buffer vector
  // the pixel is set to 0x00, otherwise, 0xff.
  static void DiffDataAndBuffer(const std::vector<unsigned char>& data,
                                 std::vector<unsigned char>* buffer);

  // If uchars in data vector and buffer vector are 0xff, then the buffer
  // pixel is set to 0xff, otherwise 0xff.
  static void SameWhite(const std::vector<unsigned char>& data,
                        std::vector<unsigned char>* buffer);

  // If uchars in data vector and buffer vector are not 0x00, then the buffer
  // pixel is set to 0xff, otherwise 0xff.
  static void SameNotBlack(const std::vector<unsigned char>& data,
                           std::vector<unsigned char>* buffer);


  // Inverts all uchars in image.
  static void InvertImage(std::vector<unsigned char>* image);

  // Threshold image so that all values at or below threshold are 0x00 and all
  // above are 0xff. Can be used to "de-feather" an image.
  static void ThresholdImage(std::vector<unsigned char>* image,
                             const unsigned char threshold_byte);

  // Loads mask data from given file. Data should be allocated to hold
  // width * height bytes.
  static void LoadMask(const std::string mask_file,
                       std::vector<unsigned char>* alpha_buffer,
                       const int width,
                       const int height);

  // Save alpha buffer to the object's mask file with the given geo extents.
  void SaveMask(std::vector<unsigned char>* alpha_buffer,
                const int width,
                const int height,
                const khGeoExtents geo_extents);

  // Opens an image file to get its width, height, geo extent and projection.
  void ImageInfo(std::string image_file,
                 int* width,
                 int* height,
                 khGeoExtents* geo_extents,
                 std::string* psz_projection);

  // Adds polygon from polygon file to the mask. If is_positive_mask is true,
  // the polygon is filled with 0x00, otherwise it is filled with 0xff.
  void ApplyPolygon(const std::string& polygon_file,
                    const unsigned char pixel_byte);

  // Creates a temporary directory and converts the kml to a shp file
  // in that directory. Other associated files are also created.
  void ConvertKmlToShpFile(const std::string polygon_kml_file,
                           std::string* polygon_shp_file,
                           std::string* layer,
                           std::string* tmp_dir);

  // Feathers the edges of the mask. Border should usually match the base
  // mask value which is based on is_pass0_positive_mask_ (see InitMask).
  void FeatherMask(const int feather, const unsigned char border);

  void FeatherMaskData(std::vector<unsigned char>* mask_data,
                       const int feather,
                       const unsigned char border);

  // Check that the input file is readable and that another one hasn't been
  // specified.
  void CheckInputFile(std::string input_file);

  DISALLOW_COPY_AND_ASSIGN(PolyMask);
};

}  // namespace fusion_tools

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_TOOLS_POLYMASK_H_
