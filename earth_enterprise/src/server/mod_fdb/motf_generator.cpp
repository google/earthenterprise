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



#include "server/mod_fdb/motf_generator.h"

#include <gdalwarper.h>
#include <ogr_spatialref.h>
#include <vrtdataset.h>
#include <memdataset.h>

#include "common/khTileAddr.h"
#include "common/khConstants.h"
#include "common/serverdb/serverdbReader.h"
#include "common/serverdb/map_tile_utils.h"
#include "common/proto_streaming_imagery.h"
#include "common/geGdalUtils.h"
#include "common/generic_utils.h"

#ifdef APLOG_USE_MODULE
APLOG_USE_MODULE(fdb);
#endif

namespace {

/**
 * Structures for MotF processing.
 * MotfParams   => initialization of Mercator and Plate Carree Params.
 */
struct MotfParams{
  int xmotf;  // Mercator x tile location for use in calculations.
  double ymotf;  // Mercator y tile location for use in calculations.
  int zmotf;  // Mercator z tile location for use in calculations.
  double degpertile;  // For Plate Carree (Varies with Lat for Mercator).
  double y_merc_dist_scale;  // Merc distance scale as function of level.
  double y_0;  // y at 0 degrees Lat(both  merc and P.C.).
  double y_merc_south;  // input in mercator coordinates.
  double y_merc_north;  // input in mercator coordinates.
  double lat_south;  // lower latitude of requested mercator tile.
  double lat_north;  // upper latitude of requested mercator tile.
  double y_pc_south;  // the lower position in P.C. space
  double y_pc_north;  // the top position in P.C. space.
  double sign;  // Reverse for Southern lats.
  int tile_south;   // lower P.C. tile.
  int tile_south_pix;  // number of pixels to lower edge from low tile edge.
  int tile_north_pix;  // number of pixels to top edge from low tile edge.
};

/**
 * Structure for MotF processing.
 * Re-projection tile description.
 */
struct UpsampledTile {
  bool dataflag;  // no data if dataflag=0; data exists if dataflag=1.
  int y_up;  // the "y" value per Upsampled tile.
  int x_up;  // the "x" value per Upsampled tile.
  int xoff;  // x offset in pxls per tile.
  int yoff;  // y offset in pxls per tile.
  int xsize;  // x size in pixels per tile(=256:MotF transform).
  int ysize;  // y size in pixels per tile.
  double lat_northup;  // the top latitude per tile.
  double lat_southup;  // the lower latitude per tile.
};

/**
 * Vector of the UpsampledTile Structure.
 */
typedef std::vector<UpsampledTile> UpsampledTileVector;

const int kMotfTileSize = ImageryQuadnodeResolution;

void CreateDstTransform(const MotfParams &motf_params,
                        std::vector<double> *dst_geo_transform,
                        char **dst_srs) {
  // TODO: Use Fusion GDALDataset wrapper:KhGDALDataset.
  GDALDataset* hsrc_ds;  // Source Dataset.
  double max_dst_y, min_dst_y, max_dst_x, min_dst_x;  // Destination bounds.
  GDALDriverH hdriver = GDALGetDriverByName("GTiff");
  GDALDataType data_type = GDT_Byte;
  char  **create_options = NULL;
  int src_band_count = 1;
  std::string vsidatafile;
  hsrc_ds = geGdalVSI::VsiGdalCreateWrap(
      hdriver, &vsidatafile, kMotfTileSize, kMotfTileSize,
      src_band_count, data_type, create_options);
  CSLDestroy(create_options);
  // Set up the source transform parameters.
  double src_transform[6];
  double src_bounds[4] = { 0, 0, 0, 0 };  // Initialize source bounds.
  src_bounds[1] = fabs(motf_params.lat_north);  // North/Upper y.
  src_bounds[3] = motf_params.sign * (motf_params.lat_south);  // South/Lower y.
  src_bounds[0] = 0.0;  // West/Left x.
  src_bounds[2] = motf_params.degpertile;  // East/Right x.
  // Define the Source Spatial Reference System(SRS).
  char *src_srs = NULL;
  OGRSpatialReference ogr_srs;
  ogr_srs.SetFromUserInput(kPlateCarreeGeoCoordSysName);
  ogr_srs.exportToWkt(&src_srs);
  // Define the source georeferencing transform.
  src_transform[0] = src_bounds[0];
  src_transform[1] = (src_bounds[2] - src_bounds[0]) / kMotfTileSize;
  src_transform[2] = 0.0;
  src_transform[3] = src_bounds[1];
  src_transform[4] = 0.0;
  src_transform[5] = (src_bounds[3] - src_bounds[1]) / kMotfTileSize;
  GDALSetProjection(hsrc_ds, src_srs);  // Set source projection.
  GDALSetGeoTransform(hsrc_ds, src_transform);  // Set source transform.
  // Define the Destination Spatial Reference System(SRS).
  ogr_srs.SetFromUserInput("+proj=merc");
  ogr_srs.exportToWkt(dst_srs);
  char **srs_tokens = NULL;  // Spatial Reference System(SRS) list.
  srs_tokens = CSLSetNameValue(srs_tokens, "SRC_SRS",
                               src_srs);  // Add Source SRS.
  srs_tokens = CSLSetNameValue(srs_tokens, "DST_SRS",
                               *dst_srs);  // Add Dest SRS.
  OGRFree(src_srs);
  // Define Destination dataset bounds.
  void *transform_arg;
  double geo_transform[6];
  double dst_bounds[4];
  int npixels, nlines;  // Needed for GDALSuggestedWarpOutput2.
  transform_arg = GDALCreateGenImgProjTransformer2(hsrc_ds, NULL, srs_tokens);
  GDALTransformerInfo* transformer_info = static_cast<GDALTransformerInfo*>
      (transform_arg);
  // Let GDAL determine destination bounds:
  GDALSuggestedWarpOutput2(hsrc_ds, transformer_info->pfnTransform,
                           transform_arg, geo_transform, &npixels, &nlines,
                           dst_bounds, 0);
  min_dst_x = dst_bounds[0];
  max_dst_x = dst_bounds[2];
  if (motf_params.sign > 0) {  // Check for North or South Hemisphere.
    max_dst_y = dst_bounds[3];
    min_dst_y = dst_bounds[1];
  } else {
    max_dst_y = dst_bounds[1];
    min_dst_y = dst_bounds[3];
  }

  CSLDestroy(srs_tokens);
  GDALDestroyGenImgProjTransformer(transform_arg);
  GDALClose(hsrc_ds);
  VSIUnlink(vsidatafile.c_str());
  // Define the destination georeferencing transform.
  assert(dst_geo_transform->size() == 6);
  (*dst_geo_transform)[0] = 0.0;
  (*dst_geo_transform)[1] = (max_dst_x - min_dst_x) / kMotfTileSize;
  (*dst_geo_transform)[2] = 0.0;
  (*dst_geo_transform)[3] = max_dst_y;
  (*dst_geo_transform)[4] = 0.0;
  (*dst_geo_transform)[5] = (min_dst_y - max_dst_y) / kMotfTileSize;
}

GDALDataset* GetSrcTile(request_rec* r, const MotfParams &motf_params,
                       int levelup,
                       bool* has_alpha,
                       const UpsampledTile &upsampled_tile,
                       ServerdbReader* reader,
                       ArgMap* arg_map) {
  keyhole::EarthImageryPacket imagery_pb;
  int scaleup = 1 << levelup;
  int z_up = motf_params.zmotf + levelup;
  int x_up0 = motf_params.xmotf * scaleup;  // Beginning tile x position
  double degpertileup = motf_params.degpertile / scaleup;
  bool is_cacheable = true;
  char txt[16];
  snprintf(txt, sizeof(txt), "%d", upsampled_tile.y_up);
  (*arg_map)["row"] = txt;
  snprintf(txt, sizeof(txt), "%d", upsampled_tile.x_up);
  (*arg_map)["col"] = txt;
  snprintf(txt, sizeof(txt), "%d", z_up);
  (*arg_map)["level"] = txt;

  std::string vsidatafile;
  ServerdbReader::ReadBuffer buf;
  reader->GetData(*arg_map, buf, is_cacheable);

  bool non_pb_jpeg = gecommon::IsJpegBuffer(buf.data());
  // Create GDAL dataset from raw jpeg if source data is non protobuf jpeg
  // format(e.g. GEE4.x 2D Flat imagery packets)
  GDALDataset* hdata_ds = NULL;  // Define source bands(uncut) GDAL dataset
  if (non_pb_jpeg) {
    hdata_ds = geGdalVSI::VsiGdalOpenInternalWrap(&vsidatafile, buf);

  // Create GDAL dataset from EarthImageryPacket protobuf.
  } else if (imagery_pb.ParseFromString(buf)) {
    const std::string& image_data = imagery_pb.image_data();
    hdata_ds =  geGdalVSI::VsiGdalOpenInternalWrap(&vsidatafile, image_data);

  // Return NULL if source data is not valid EarthImageryPacket protobuf.
  } else {
    return NULL;
  }
  double src_transform[6];
  double src_bounds[4] = {0, 0, 0, 0};
  // Upper Left y:
  src_bounds[1] = fabs(upsampled_tile.lat_northup);
  // Lower Right y
  src_bounds[3] = motf_params.sign * (upsampled_tile.lat_southup);
  // Upper Left y:
  src_bounds[0] = fabs((upsampled_tile.x_up - x_up0) *
                       degpertileup);
  // Lower Right x:
  src_bounds[2] = fabs((upsampled_tile.x_up - x_up0 + 1) *
                       degpertileup);
  int src_xsize = upsampled_tile.xsize;
  int src_ysize = upsampled_tile.ysize;
  int src_band_count = GDALGetRasterCount(hdata_ds);
  // Create a new src dataset and update the bands:
  VRTDataset *pvrt_ds = static_cast<VRTDataset *>(VRTCreate(src_xsize,
                                                            src_ysize));
  // Process the new src bands.
  VRTSourcedRasterBand *pvrt_band;
  GDALRasterBand *psrc_band;
  GDALDataType eBandType;
  for (int i = 0; i < src_band_count; i++) {
    psrc_band =
        (static_cast<GDALDataset*>(hdata_ds))->GetRasterBand(i + 1);
    eBandType = psrc_band->GetRasterDataType();  // Src data type.
    // Create band i+1:
    pvrt_ds->AddBand(eBandType, NULL);
    pvrt_band =
        static_cast<VRTSourcedRasterBand*>(pvrt_ds->GetRasterBand(i + 1));
    pvrt_band->AddSimpleSource(psrc_band,
                               upsampled_tile.xoff,
                               upsampled_tile.yoff,
                               upsampled_tile.xsize,
                               upsampled_tile.ysize,
                               0, 0, src_xsize, src_ysize);
  }
  std::string vsialphafile;
  GDALDataset* halpha_ds = NULL;  // Define alpha band(uncut) GDAL dataset
  // If format request is not jpeg test if alpha channel is required.
  if ((*arg_map)["format"] != MapTileUtils::kJpegMimeType.GetLiteral()) {
    // Check if the data has an alpha band. Add the alpha band if it does.
    // if any tile has alpha band will ultimately write out as PNG.
    if (imagery_pb.has_image_alpha()) {
      *has_alpha = TRUE;
      const std::string& image_alpha = imagery_pb.image_alpha();
      halpha_ds = geGdalVSI::VsiGdalOpenInternalWrap(&vsialphafile,
                                                     image_alpha);
      psrc_band = (static_cast<GDALDataset*>(halpha_ds))
          ->GetRasterBand(1);
      eBandType = psrc_band->GetRasterDataType();
      // Create the extra band:
      pvrt_ds->AddBand(eBandType, NULL);
      pvrt_band =
          static_cast<VRTSourcedRasterBand*>(pvrt_ds->GetRasterBand(4));
      pvrt_band->AddSimpleSource(psrc_band,
                                 upsampled_tile.xoff,
                                 upsampled_tile.yoff,
                                 upsampled_tile.xsize,
                                 upsampled_tile.ysize,
                                 0, 0, src_xsize, src_ysize);
    // If no alpha band create a clear band and add it only if any previous
    // tiles contained an alpha band.
    } else if (*has_alpha) {
      psrc_band = (static_cast<GDALDataset*>(hdata_ds))->GetRasterBand(1);
      eBandType = psrc_band->GetRasterDataType();
      // Create the extra band:
      pvrt_ds->AddBand(eBandType, NULL);
      pvrt_band =
          static_cast<VRTSourcedRasterBand*>(pvrt_ds->GetRasterBand(4));
      pvrt_band->AddComplexSource(psrc_band,
                                  upsampled_tile.xoff,
                                  upsampled_tile.yoff,
                                  upsampled_tile.xsize,
                                  upsampled_tile.ysize,
                                  0, 0, src_xsize, src_ysize,
                                  255, 0, VRT_NODATA_UNSET);
    }
  }
  char *src_srs = NULL;
  OGRSpatialReference ogr_srs;
  ogr_srs.SetFromUserInput(kPlateCarreeGeoCoordSysName);
  ogr_srs.exportToWkt(&src_srs);
  pvrt_ds->SetProjection(src_srs);
  src_transform[0] = src_bounds[0];
  src_transform[1] = (src_bounds[2] - src_bounds[0]) / src_xsize;
  src_transform[2] = 0.0;
  src_transform[3] = src_bounds[1];
  src_transform[4] = 0.0;
  src_transform[5] = (src_bounds[3] - src_bounds[1]) / src_ysize;
  pvrt_ds->SetGeoTransform(src_transform);
  GDALDriverH hdriver = GDALGetDriverByName("MEM");
  int bStrict = FALSE;
  // Write the bands to a MEMDataset.
  GDALDataset* hsrc_ds = static_cast<GDALDataset*>(GDALCreateCopy(
      hdriver, "", static_cast<GDALDatasetH>(pvrt_ds), bStrict, NULL, NULL,
      NULL));
  OGRFree(src_srs);
  GDALClose((GDALDatasetH) pvrt_ds);
  GDALClose(hdata_ds);
  VSIUnlink(vsidatafile.c_str());
  if (halpha_ds != NULL) {
    GDALClose(halpha_ds);
    VSIUnlink(vsialphafile.c_str());
  }
  return hsrc_ds;
}

void WarpData(const MotfParams &motf_params, int levelup,
              const UpsampledTileVector &upsampled_tiles,
              ServerdbReader* reader, ArgMap* arg_map,
              bool has_alpha, request_rec* r) {
  int num_tiles = upsampled_tiles.size();
  assert(num_tiles > 0);
  // Define warp loop parameters.
  GDALResampleAlg resample_alg = GRA_NearestNeighbour;  // Default
  ArgMap::const_iterator it = arg_map->find("ResampleAlg");
  if (it != arg_map->end() && !it->second.empty()) {
    const std::string &resamp_str = it->second;
    const char firstchar = resamp_str[0];
    if (firstchar == 'b' || firstchar == 'B') {
      resample_alg = GRA_Bilinear;
    } else if (firstchar == 'm' || firstchar == 'M')  {
      resample_alg = GRA_Mode;
    } else if (firstchar == 'c' || firstchar == 'C')  {
      resample_alg = GRA_CubicSpline;
    } else if (firstchar == 'l' || firstchar == 'L')  {
      resample_alg = GRA_Lanczos;
    } else if (firstchar == 'a' || firstchar == 'A')  {
      resample_alg = GRA_Average;
    }
  }
  char **warp_options = NULL;
  // Maximum acceptable destination transform error given in pixel units.
  // Default error threshold = 0.125 pixels.
  double error_threshold = 0.125;
  // Data type to use during warp operation, GDT_Unknown lets the algorithm
  // select the type.
  GDALDataType working_type = GDT_Unknown;
  warp_options = CSLSetNameValue(warp_options, "INIT_DEST", NULL);
  GDALWarpOptions *gdal_warp = GDALCreateWarpOptions();
  GDALTransformerFunc transformer = GDALApproxTransform;
  GDALWarpOperation warp_operation;
  // Get the first source PC tile which makes up the mercator mosaic
  // and initialize the warp process including the destination dataset.
  GDALDataset* hsrctile1_ds;
  hsrctile1_ds = GetSrcTile(r, motf_params, levelup, &has_alpha,
                           upsampled_tiles[0], reader, arg_map);

  // Make destination data type same as source data's first band.
  GDALDataType data_type =
      GDALGetRasterDataType(GDALGetRasterBand(hsrctile1_ds, 1));
  gdal_warp->eWorkingDataType = data_type;
  // TODO: Investigate use of KhGDALDataset a smart pointer around
  // GDALdataset which may also facilitate processing of a 4th alpha band.
  OGRSpatialReference ogr_srs;
  char *dst_srs = NULL;
  std::vector<double> dst_geo_transform(6);
  CreateDstTransform(motf_params, &dst_geo_transform, &dst_srs);
  // Set static GDAL warp options
  gdal_warp->papszWarpOptions = CSLDuplicate(warp_options);
  gdal_warp->eWorkingDataType = working_type;
  gdal_warp->eResampleAlg = resample_alg;
  gdal_warp->pfnTransformer = transformer;
  gdal_warp->nBandCount = GDALGetRasterCount(hsrctile1_ds);
  gdal_warp->panSrcBands =
      static_cast<int*>(CPLMalloc(gdal_warp->nBandCount*sizeof(int)));
  gdal_warp->panDstBands =
      static_cast<int*>(CPLMalloc(gdal_warp->nBandCount*sizeof(int)));
  for (int i = 0; i < gdal_warp->nBandCount; i++) {
    gdal_warp->panSrcBands[i] = i + 1;
    gdal_warp->panDstBands[i] = i + 1;
  }
  GDALDataset* hdst1_ds;
  gdal_warp->hSrcDS = hsrctile1_ds;
  gdal_warp->pfnTransformer = GDALGenImgProjTransform;
  gdal_warp->pTransformerArg = GDALCreateGenImgProjTransformer(
      hsrctile1_ds, NULL, NULL, dst_srs, TRUE, 1.0, 0);
  GDALSetGenImgProjTransformerDstGeoTransform(gdal_warp->pTransformerArg,
                                              &dst_geo_transform[0]);
  gdal_warp->pTransformerArg = GDALCreateApproxTransformer(
      gdal_warp->pfnTransformer, gdal_warp->pTransformerArg, error_threshold);
  gdal_warp->pfnTransformer = GDALApproxTransform;
  GDALApproxTransformerOwnsSubtransformer(gdal_warp->pTransformerArg, TRUE);
  hdst1_ds = static_cast<GDALDataset*>(GDALCreateWarpedVRT(
      hsrctile1_ds, kMotfTileSize, kMotfTileSize, &dst_geo_transform[0],
      gdal_warp));
  // If more than 1 tile continue the warp, grab each remaining pc tile
  // warp it to mercator and mosaic it to mercator destination array.
  GDALDriverH hdriver;
  hdriver = GDALGetDriverByName("MEM");
  MEMDataset* hdst_ds = static_cast<MEMDataset*>
      (GDALCreateCopy(hdriver, "", hdst1_ds, FALSE, NULL, NULL, NULL));
  if (num_tiles > 1) {
    GDALSetProjection(hdst_ds, dst_srs);
    gdal_warp->hDstDS = hdst_ds;
  }
  char **srs_tokens = NULL;
  GDALDataset* hsrctile_ds;
  // TODO: Move mosaic block to separate function for readability.
  for (int i = 1; i < num_tiles; i++) {
    // Get each source pc tile which makes up the mercator mosaic
    hsrctile_ds = GetSrcTile(r, motf_params, levelup, &has_alpha,
                             upsampled_tiles[i], reader, arg_map);

    // Add the alpha band to the destination tile if any of the source tiles
    // contain the alpha band.
    assert(GDALGetRasterCount(hsrctile_ds) == 3 ||
           GDALGetRasterCount(hsrctile_ds) == 4);
    if (GDALGetRasterCount(hsrctile_ds) == 4 &&
        GDALGetRasterCount(hdst_ds) == 3) {
      hdst_ds->AddBand(data_type, NULL);
      GDALRasterBand *pvrt_band = static_cast<GDALRasterBand*>(
          hdst_ds->GetRasterBand(4));
      pvrt_band->Fill(255);
      gdal_warp->hDstDS = (GDALDatasetH)hdst_ds;
      CPLFree(gdal_warp->panSrcBands);
      CPLFree(gdal_warp->panDstBands);
      gdal_warp->nBandCount = 4;
      gdal_warp->panSrcBands =
          static_cast<int*>(CPLMalloc(gdal_warp->nBandCount*sizeof(int)));
      gdal_warp->panDstBands =
          static_cast<int*>(CPLMalloc(gdal_warp->nBandCount*sizeof(int)));
      for (int i = 0; i < gdal_warp->nBandCount; i++) {
        // Setup the source and destination band list:
        // band list indexes 0:3 -> bands 1:4.
        gdal_warp->panSrcBands[i] = i + 1;
        gdal_warp->panDstBands[i] = i + 1;
      }
    }
    void *transform_arg;
    void *gen_img_proj_arg = NULL;
    gen_img_proj_arg = GDALCreateGenImgProjTransformer2(
        hsrctile_ds, hdst_ds, srs_tokens);
    // GDALCreateApproxTransformer speeds up the warp considerably:
    transform_arg = GDALCreateApproxTransformer(
        GDALGenImgProjTransform, gen_img_proj_arg, error_threshold);
    gdal_warp->pTransformerArg = transform_arg;
    gdal_warp->hSrcDS = hsrctile_ds;
    // Initialize warp operation.
    warp_operation.Initialize(gdal_warp);
    // Combined warp and mosaic operation.
    warp_operation.ChunkAndWarpImage(0, 0, GDALGetRasterXSize(hdst_ds),
                                     GDALGetRasterYSize(hdst_ds));
    GDALDestroyApproxTransformer(transform_arg);
    GDALDestroyGenImgProjTransformer(gen_img_proj_arg);
    GDALClose(hsrctile_ds);
  }
  CSLDestroy(srs_tokens);
  GDALDataset* hout_ds;
  char **papszOptions = NULL;
  vsi_l_offset buffer_length;
  GByte *membuf;
  std::string vsioutfile;
  const std::string& format = (*arg_map)["format"];
  if ((format.empty() && has_alpha) ||
      format == MapTileUtils::kPngMimeType.GetLiteral()) {
    // Create a PNG output if alpha channel.
    hdriver = GDALGetDriverByName("PNG");
    // PNG ZLEVEL controls the compression, ZLEVEL="6" is the default.
    papszOptions = CSLSetNameValue(papszOptions, "ZLEVEL", "6");
    r->content_type = MapTileUtils::kPngMimeType.GetLiteral();
  } else {
    // Create a JPEG output if no alpha channel.
    hdriver = GDALGetDriverByName("JPEG");
    r->content_type = MapTileUtils::kJpegMimeType.GetLiteral();
  }
  // GDALCreateCopy 4th param = FALSE: copy may adapt as needed for output
  // format.
  hout_ds = geGdalVSI::VsiGdalCreateCopyWrap(hdriver, &vsioutfile, hdst_ds,
                                             FALSE, papszOptions, NULL, NULL);
  char **papszFileList = GDALGetFileList(hout_ds);
  GDALClose(hout_ds);
  // VSIGetMemFileBuffer returns a pointer to the buffer underlying the vsi
  // file. If 3rd arg=TRUE the vsi file object will be deleted.
  membuf =  VSIGetMemFileBuffer(vsioutfile.c_str(), &buffer_length, TRUE);
  CSLDestroy(papszOptions);
  ap_rwrite(&membuf[0], buffer_length, r);
  CPLFree(membuf);
  GDALClose(hdst_ds);
  GDALClose(hdst1_ds);
  OGRFree(dst_srs);
  CSLDestroy(warp_options);
  GDALDestroyWarpOptions(gdal_warp);
  GDALClose(hsrctile1_ds);
  for (int i = 0; papszFileList[i] != NULL; i++) {
    if (papszFileList[i] != vsioutfile) {
      VSIUnlink(papszFileList[i]);
    }
  }
  CSLDestroy(papszFileList);
}

// GetUpsampledTiles determines the plate carree(P.C.) map tiles
// which are necessary to produce a single 256x256 pixel mercator map tile.
// The tile information is output in a UpsampledTileVector structure
// vector which lists the x(x_up) and y(y_up) P.C. tile descriptors (at given
// level z_up), cut lines of each P.C. tile([xoff,xsize],[yoff,ysize]), and the
// northern most and southern most latitudes of each plate carree at the cut
// lines(lat_southup,lat_northup). The P.C. tiles are pulled from levels(if
// available) above the requested mercator tile to produce the required mercator
// level resolution. The ServerdbReader::CheckLevel routine checks for available
// P.C. database data and sets UpsampledTileVector.dataflag[ii] = 0 if none is
// available for required P.C. tile "tnum=ii".
int GetUpsampledTiles(const MotfParams &motf_params,
                      int levelup,
                      UpsampledTileVector* upsampled_tiles,
                      ServerdbReader* reader,
                      ArgMap* arg_map,
                      request_rec* r) {
  upsampled_tiles->clear();
  UpsampledTile nexttile;
  int num_tiles = 0;  // This routine is designed to loop until num_tiles > 0.
  // Plate Carree(P.C.) tile size (Varies w/Lat for Mercator(merc)).
  const double &degpertile = motf_params.degpertile;
  const double lat_south = motf_params.lat_south;  // the lower lat.
  const double lat_north = motf_params.lat_north;  // the top lat.
  const double y_pc_south = motf_params.y_pc_south;  // the lower P.C. space y.
  const double y_pc_north = motf_params.y_pc_north;  // the top P.C. space y.
  const int xmotf = motf_params.xmotf;
  int scaleup = pow(2, levelup);
  int z_up = motf_params.zmotf + levelup;
  double degpertileup = degpertile / scaleup;
  // Check if the PC dimensions extend < 1/2 pixel into a bounding north or
  // south PC tile. If so, exclude the <1/2 pixel and set the north or south
  // tile bounds to the next closest PC tile.
  const double half_pix = (0.5 / kMotfTileSize);
  double tiletest = scaleup * y_pc_north + scaleup - 1;
  int tilenorthup = static_cast<int>(ceil(tiletest));
  tiletest = tilenorthup - tiletest;
  if (tiletest < half_pix) {
    tilenorthup = static_cast<int>(ceil(scaleup * y_pc_north)) + scaleup;
  }
  tiletest = scaleup * y_pc_south + scaleup - 1;
  int tilesouthup = static_cast<int>(ceil(tiletest));
  tiletest = 1.0 - (tilesouthup - tiletest);
  if (tiletest < half_pix) {
    tilesouthup = static_cast<int>(floor(scaleup * y_pc_south)) + scaleup - 1;
  }
  int y_0up = pow(2, z_up - 1) - 1;  // y (both merc and P.C.) at 0 degrees Lat.
  int tile_south_pixup = static_cast<int>(
      floor(kMotfTileSize * (tilesouthup -
                             (scaleup * y_pc_south + scaleup - 1))));
  int tile_north_pixup = static_cast<int>(
      ceil(kMotfTileSize * (tilenorthup -
                            (scaleup * y_pc_north + scaleup - 1))));
  int tnum = 0;

  std::uint32_t channel;
  reader->GetUint32Arg((*arg_map)["channel"], &channel);
  std::uint32_t version;
  reader->GetUint32Arg((*arg_map)["version"], &version);
  bool VERSION_MATTERS = true;
  geindex::TypedEntry::ReadKey read_key(version, channel,
                                        geindex::TypedEntry::Imagery,
                                        VERSION_MATTERS);

  if (levelup > 0) {
    int nx = pow(2, levelup);
    int ny = tilesouthup - tilenorthup + 1;
    for (int ii = 0; ii < nx; ++ii) {
      // Set Top edge tile on column ii.
      int column = scaleup * xmotf + ii;
      nexttile.y_up = tilenorthup;
      nexttile.x_up = column;
      nexttile.lat_southup = (y_0up - nexttile.y_up) * degpertileup;
      nexttile.lat_northup = lat_north;  // highest Lat(North).
      nexttile.xoff = 0;
      nexttile.xsize = kMotfTileSize;
      nexttile.yoff = kMotfTileSize - tile_north_pixup;
      nexttile.ysize = tile_north_pixup;

      int levelok = reader->CheckLevel(read_key, static_cast<std::uint32_t>(z_up),
                                       static_cast<std::uint32_t>(nexttile.y_up),
                                       static_cast<std::uint32_t>(nexttile.x_up));
      nexttile.dataflag = levelok;
      if (levelok == 2) {
        return 0;
      }

      tnum++;
      upsampled_tiles->push_back(nexttile);
      for (int jj = 1; jj < ny - 1; ++jj) {
        // Set tiles on middle of column ii.
        nexttile.y_up = (*upsampled_tiles)[tnum - 1].y_up + 1;
        nexttile.x_up = column;
        nexttile.lat_southup = (y_0up - nexttile.y_up) * degpertileup;
        nexttile.lat_northup = (y_0up - (*upsampled_tiles)[tnum - 1].y_up) *
            degpertileup;
        nexttile.xoff = 0;
        nexttile.xsize = kMotfTileSize;
        nexttile.yoff = 0;
        nexttile.ysize = kMotfTileSize;
        // Test Level.

        int levelok = reader->CheckLevel(read_key, static_cast<std::uint32_t>(z_up),
                                         static_cast<std::uint32_t>(nexttile.y_up),
                                         static_cast<std::uint32_t>(nexttile.x_up));
        nexttile.dataflag = levelok;
        if (levelok == 2) {
          return 0;
        }

        tnum++;
        upsampled_tiles->push_back(nexttile);
      }
      if (ny ==  1) {
      // Set if single y tile on column ii.
        (*upsampled_tiles)[tnum - 1].lat_southup = lat_south;
        (*upsampled_tiles)[tnum - 1].ysize = tile_north_pixup -
            tile_south_pixup;
      } else {
      // Set tile on bottom of column ii
        nexttile.y_up = (*upsampled_tiles)[tnum - 1].y_up + 1;
        nexttile.x_up = column;
        nexttile.lat_southup = lat_south;
        nexttile.lat_northup = static_cast<double>
            (y_0up - (*upsampled_tiles)[tnum - 1].y_up) * degpertileup;
        nexttile.xoff = 0;
        nexttile.xsize = kMotfTileSize;
        nexttile.yoff = 0;
        nexttile.ysize = kMotfTileSize - tile_south_pixup;
        // Test Level.

        int levelok = reader->CheckLevel(read_key, static_cast<std::uint32_t>(z_up),
                                         static_cast<std::uint32_t>(nexttile.y_up),
                                         static_cast<std::uint32_t>(nexttile.x_up));
        nexttile.dataflag = levelok;
        if (levelok == 2) {
          return 0;
        }

        ++tnum;
        upsampled_tiles->push_back(nexttile);
      }
    }
  } else if (levelup == 0) {
    // No uplevel is req'd or no uplevel is possible(ie required upsampling
    // tiles have not been found and levelup has been reduced to 0).
    nexttile.y_up = tilenorthup;
    nexttile.x_up = scaleup * xmotf;
    int levelok = reader->CheckLevel(read_key, static_cast<std::uint32_t>(z_up),
                                     static_cast<std::uint32_t>(nexttile.y_up),
                                     static_cast<std::uint32_t>(nexttile.x_up));
    bool is_cutter = ((*arg_map)["ct"] == "c");
    // If ancestor data exists below requested level(levelok=2) and cutter is
    // enabled do not attempt resampling. MotF will then return HTTP_NOT_FOUND.
    // If only levelok=2 (cutter not enabled) then MotF will use PC source data
    // resampled from the lower levels.
    if (levelok == 2 && is_cutter) {
      return 0;
    }
    nexttile.dataflag = levelok;
    // edge if 1 tile:
    nexttile.lat_southup = (y_0up - nexttile.y_up) * degpertileup;
    nexttile.lat_northup = lat_north;  // This edge is the high Lat(North).
    if (tilesouthup - tilenorthup == 1) {  // 2 tiles if true.
      nexttile.xoff = 0;
      nexttile.xsize = kMotfTileSize;
      nexttile.yoff = kMotfTileSize - tile_north_pixup;
      nexttile.ysize = tile_north_pixup;

      upsampled_tiles->push_back(nexttile);
      nexttile.y_up = nexttile.y_up + 1;
      nexttile.x_up = nexttile.x_up;
      int levelok = reader->CheckLevel(read_key, static_cast<std::uint32_t>(z_up),
                                       static_cast<std::uint32_t>(nexttile.y_up),
                                       static_cast<std::uint32_t>(nexttile.x_up));
      if (levelok == 2 && is_cutter) {
        return 0;
      }
      nexttile.lat_southup = lat_south;
      nexttile.lat_northup = (y_0up -
                              (*upsampled_tiles)[0].y_up) * degpertileup;
      nexttile.xoff = 0;
      nexttile.xsize = kMotfTileSize;
      nexttile.yoff = 0;
      nexttile.ysize = kMotfTileSize - tile_south_pixup;
      tnum = 2;

      upsampled_tiles->push_back(nexttile);
    } else {
      nexttile.lat_southup = lat_south;
      nexttile.xoff = 0;
      nexttile.xsize = kMotfTileSize;
      nexttile.yoff = kMotfTileSize - tile_north_pixup;
      nexttile.ysize = tile_north_pixup - tile_south_pixup;
      tnum = 1;
      upsampled_tiles->push_back(nexttile);
    }
  }
  num_tiles = tnum;
  return num_tiles;
}

}  // namespace

namespace geserver {

MotfGenerator::MotfGenerator() {
}

// MotfGenerator::GenerateMotfTile will convert a requested mercator (x,y,z)
// tile address to a Mercator tile image by reprojecting the necesssary plate
// carree(P.C.) tiles using GDAL functions. GenerateMotfTile determines the
// basic parameters for reprojection then submits the params to the
// GetUpsampledTiles function outputing the structure vector which enables the
// proper GDAL reprojection.
int MotfGenerator::GenerateMotfTile(ServerdbReader* reader,
                                    ArgMap* arg_map,
                                    request_rec* r) {
  MotfParams motf_params;
  motf_params.xmotf = atoi((*arg_map)["x"].c_str());  // Merc. x.
  motf_params.ymotf = strtod((*arg_map)["y"].c_str(), NULL);  // Mercator y.
  motf_params.zmotf = atoi((*arg_map)["z"].c_str());  // Merc. z.
  motf_params.degpertile =  (360) / pow(2, motf_params.zmotf);  // For P.C.
  // Merc scale func of level:
  motf_params.y_merc_dist_scale = M_PI / pow(2, motf_params.zmotf - 1);
  // y at 0 degrees Lat(both mercator(merc) and plate carree(pc)):
  motf_params.y_0 = pow(2, motf_params.zmotf - 1) - 1;
  motf_params.y_merc_south = (motf_params.y_0 - motf_params.ymotf) *
       motf_params.y_merc_dist_scale;  // merc coord.
  motf_params.y_merc_north = (motf_params.y_0 + 1 - motf_params.ymotf) *
       motf_params.y_merc_dist_scale;  // merc coord.
  motf_params.lat_south = (atan2(exp(motf_params.y_merc_south), 1.0) -
                           M_PI / 4.0) * 2.0 * 180 / M_PI;
  motf_params.lat_north = (atan2(exp(motf_params.y_merc_north), 1.0) -
                           M_PI / 4.0) * 2.0 * 180 / M_PI;
  motf_params.y_pc_south = - motf_params.lat_south/motf_params.degpertile +
      motf_params.y_0;  // lower position in P.C.
  motf_params.y_pc_north = - motf_params.lat_north/motf_params.degpertile +
      motf_params.y_0;  // top position in P.C.
  motf_params.sign =
      (motf_params.zmotf != 0 && motf_params.y_merc_south < 0.) ? -1 : 1;
  // lower P.C. tile:
  motf_params.tile_south = static_cast<int>(ceil(motf_params.y_pc_south));
  // pixels from low edge to lower P.C. tile:
  motf_params.tile_south_pix = static_cast<int>(
      ceil(kMotfTileSize * (motf_params.tile_south - motf_params.y_pc_south)));
  // pixels from top edge to lower P.C. tile:
  motf_params.tile_north_pix = static_cast<int>(
      ceil(kMotfTileSize *(motf_params.tile_south - motf_params.y_pc_north)));
  // levelup calculation is based on P.C. y dimension cropped width
  // in pixels which decreases as latitudes increase or decrease from the
  // equator for any given Mercator single tile request.
  // Sampled up (higher resolution) tiles will be needed to maintain
  // the required resolution at the requested mercator level. So P.C. tiles are
  // obtained from level = z + levelup.
  int levelup = floor(.5 + log((256. / (motf_params.tile_north_pix -
                                        motf_params.tile_south_pix))) / log(2));
  // The number of tiles needed for to transform to Mercator: Loop until >0:
  int num_tiles = 0;
  // Set of upsampled tiles required to transform to Mercator.
  UpsampledTileVector upsampled_tiles;
  // Add 1 to levelup to compensate the while loop decrement below.
  levelup++;
  while ((num_tiles == 0) && (levelup >= 0)) {
    levelup--;  // Reduce Sample up level until tiles are found.
    num_tiles = GetUpsampledTiles(motf_params, levelup,
                                  &upsampled_tiles, reader, arg_map, r);
  }
  if (num_tiles > 0) {
    // Check if all the PC tile data used to create the full MotF mosaic exists.
    // If there are non-existant tiles, remove them and also add a transparent
    // alpha band to the MotF mosaic. Abort if all tiles are missing.
    bool has_alpha = FALSE;
    
    upsampled_tiles.erase(std::remove_if(upsampled_tiles.begin(), upsampled_tiles.end(),
        [&](const UpsampledTile& upsampled_tile) {
          if (upsampled_tile.dataflag != 0) {
            // Check to make sure that the tile actually contains
            // data, because blank tiles may not
            ServerdbReader::ReadBuffer buf;
            bool is_cacheable;
            char txt[16];
            int z_up = motf_params.zmotf + levelup;

            snprintf(txt, sizeof(txt), "%d", upsampled_tile.y_up);
            (*arg_map)["row"] = txt;
            snprintf(txt, sizeof(txt), "%d", upsampled_tile.x_up);
            (*arg_map)["col"] = txt;
            snprintf(txt, sizeof(txt), "%d", z_up);
            (*arg_map)["level"] = txt;

            reader->GetData(*arg_map, buf, is_cacheable);

            if (buf.length() > 0) {
              // false, in this case, means don't remove
              return false;
            }
          }
          return true;
        }), upsampled_tiles.end());

    if (upsampled_tiles.size() == 0) return HTTP_NOT_FOUND;
    if (num_tiles != static_cast<int>(upsampled_tiles.size()))
      has_alpha = TRUE;
    WarpData(motf_params, levelup, upsampled_tiles, reader, arg_map,
             has_alpha, r);
    return OK;
  }
  return HTTP_NOT_FOUND;
}

}  // namespace geserver
