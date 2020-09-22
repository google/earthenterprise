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


#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <bitset>

#include <khraster/ProductLevelReaderCache.h>
#include <khraster/khRasterProduct.h>
#include <khraster/Interleave.h>
#include <khraster/CachedRasterInsetReaderImpl.h>
#include <khTileAddr.h>
#include <notify.h>
#include <khGetopt.h>
#include <khstl.h>
#include <common/khConstants.h>
#include <khFileUtils.h>
#include <khGeomUtils.h>
#include <compressor.h>
#include <khProgressMeter.h>

#include <autoingest/AssetVersion.h>


static const std::string kKMLHeader(
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<kml xmlns=\"http://earth.google.com/kml/2.1\">\n");

void usage(const std::string& progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  /*
   * Usage intentionally excludes a few options because they shouldn't ever
   * be needed by end users and will only make the tool more complicated:
   *   --min_lod         Specify the minLodPixels value, default is .5 * tilesize
   *   --max_lod         Specify the maxLodPixels value, default is 4 * tilesize
   *   --break_imgdir    Break-up imagery path to reduce filecount, default is 5 chars
   */
  fprintf(stderr,
          "\nusage: %s [options] {--assetref <assetref> | --kip <raster.kip>"
          " --kmp <mask.kmp>}\n"
          "   Supported options are:\n"
          "      --asset <assetref>       Specify the imagery asset\n"
          "      --kip <imagery product>  Specify the imagery product *.kip\n"
          "      --kmp <mask product>     Specify the mask product *.kmp\n"
          "      --url <url root>         All links have this URL root\n"
          "      --output <dirname>       Use dirname for output\n"
          "      --layer_name <name>      Layer name to use in client\n"
          "      --tile_size <size>       Output tilesize (256, 512 or 1024)\n"
          "      --kml_only               Generate KML files only (no imagery)\n"
          "      --no_kmz                 Don't use KMZ, only KML\n"
          "      --jpg_quality <qual>     Set quality for jpg compression\n"
          "      --drawOrder <offset>     Constant added to the computed <drawOrder>\n"
          "      --debug                  Add debug geometry\n"
          "      --north_boundary <double> Crop to north_boundary"
          " (latitude in decimal deg)\n"
          "      --south_boundary <double> Crop to south_boundary"
          " (latitude in decimal deg)\n"
          "      --east_boundary <double> Crop to east_boundary"
          " (longitude in decimal deg)\n"
          "      --west_boundary <double> Crop to west_boundary"
          " (longitude in decimal deg)\n"
          "      --help | -?:             Display this usage message\n",
          progn.c_str());
  exit(1);
}

typedef std::map<std::string, khExtents<double> > ImageExtentsMap;
typedef ImageExtentsMap::iterator ImageExtentsMapIterator;

// The format for time stamp is YYYY-MM-DD which is same as that of
// khRasterProduct::GetAcquisitionDate
bool OutputIndex(const khExtents<double>& extents,
                 const std::string& url_root, const std::string& time_stamp);
bool DrillDown(const std::string& name, const khTileAddr& addr,
               khProgressMeter* progress);
void AddChild(std::ostringstream* os, const std::string& name,
              const khTileAddr& addr);
void InsertRegion(std::ostringstream* os, int indent,
                  const khExtents<double>& extents, int min_lod, int max_lod);
void InsertRegionNoLodCalc(std::ostringstream* os, int indent,
                           const khExtents<double>& extents,
                           int min_lod, int max_lod);
void AddDebugGeometry(std::ostringstream* os, const khExtents<double>& extents,
                      const std::string& image_name, int draw_order);
void ExtractImageryTile(const std::string& name, const khTileAddr& addr,
                        khOpacityMask::OpacityType *opacity,
                        ImageExtentsMap* images);

/*
 * collect statistics of output tiles
 */
class TileStats {
 public:
  int opacity[4];
  int total;

  int jpg_uncompressed;
  int jpg_compressed;
  int jpg_total;

  int png_uncompressed;
  int png_compressed;
  int png_total;

  int total_tiles;

  TileStats() {
    memset(opacity, 0, 4);
    jpg_uncompressed = 0;
    jpg_compressed = 0;
    jpg_total = 0;
    png_uncompressed = 0;
    png_compressed = 0;
    png_total = 0;
  }

  void Dump(int total_tiles) {
    for (int o = 0; o < 4; ++o)
      printf("%s : %d (%f)\n",
             khOpacityMask::toString((khOpacityMask::OpacityType)o).c_str(),
             opacity[o], (float)opacity[o] / (float)total);
    printf("total: %d\n\n", total);
    printf("JPG Tiles: %d  ", jpg_total);
    if (jpg_total) {
       printf("avg tile sz: %f, compression ratio: %f\n",
              (float)jpg_compressed / (float)jpg_total,
              (float)jpg_compressed / (float)jpg_uncompressed);
    } else {
      printf("\n");
    }
    printf("PNG Tiles: %d  ", png_total);
    if (png_total) {
      printf("avg tile sz: %f, compression ratio: %f\n",
             (float)png_compressed / (float)png_total,
             (float)png_compressed / (float)png_uncompressed);
    } else {
      printf("\n");
    }
    printf("Transparent tiles discarded: %d\n",
           total_tiles - jpg_total - png_total);
  }

  void AddOpacity(khOpacityMask::OpacityType type) {
    ++opacity[type];
    ++total;
  }

  void AddJPG(int uncompressed, int compressed) {
    jpg_uncompressed += uncompressed;
    jpg_compressed += compressed;
    ++jpg_total;
  }

  void AddPNG(int uncompressed, int compressed) {
    png_uncompressed += uncompressed;
    png_compressed += compressed;
    ++png_total;
  }
};

/*
 * Global variables are used here because various subroutines
 * need to access them.
 *
 * This cpp file is a self-contained application so a containing
 * class is overkill.
 */
namespace {

TileStats tile_stats;
khDeleteGuard<khRasterProduct> rp_rgb;
khDeleteGuard<khRasterProduct> rp_alpha;
std::string out_dir;
std::string layer_name("Fusion generated KML Super-overlay");
bool kml_only = false;
bool no_kmz = false;
std::string file_ext;
bool debug = false;
int min_lod = INT_MIN;
int max_lod = INT_MIN;
int tile_size = 256;
int draw_order_offset = 0;
unsigned int image_dir_break = 5;
CachingProductTileReader_Alpha inset_alpha_reader(100, 15);
Compressor* jpg_compressor = NULL;
int jpg_quality = 80;
Compressor* png_compressor = NULL;

AlphaProductTile   alpha_tile;
unsigned char              interleave_buffer[RasterProductTileResolution *
                                     RasterProductTileResolution *
                                     4];
} // unnamed namespace


int main(int argc, char *argv[]) {
  std::string progname = argv[0];

  // process commandline options
  int argn;
  bool help = false;
  std::string url_root;
  std::string mask_product;
  std::string imagery_product;
  std::string assetref;
  double north_boundary = 90.0;
  double south_boundary = -90.0;
  double east_boundary = 180.0;
  double west_boundary = -180.0;

  khGetopt options;
  options.flagOpt("help", help);
  options.flagOpt("?", help);
  options.flagOpt("kml_only", kml_only);
  options.flagOpt("no_kmz", no_kmz);
  options.flagOpt("debug", debug);
  options.opt("kip", imagery_product);
  options.opt("kmp", mask_product);
  options.opt("assetref", assetref);
  options.opt("output", out_dir);
  options.opt("url", url_root);
  options.opt("jpg_quality", jpg_quality);
  options.opt("tile_size", tile_size);
  options.opt("layer_name", layer_name);
  options.opt("drawOrder", draw_order_offset);
  options.opt("north_boundary", north_boundary);
  options.opt("south_boundary", south_boundary);
  options.opt("east_boundary", east_boundary);
  options.opt("west_boundary", west_boundary);
  // unadvertised options
  options.opt("min_lod", min_lod);
  options.opt("max_lod", max_lod);
  options.opt("break_imgdir", image_dir_break);

  if (!options.processAll(argc, argv, argn))
    usage(progname);
  if (help)
    usage(progname);
  if (argn != argc)  // nothing can fall
    usage(progname);

  if (RasterProductTileResolution % tile_size != 0)
    notify(NFY_FATAL, "Tile size %d is invalid!", tile_size);

  // reasonable defaults that are probably good enough
  // though admittedly much more testing is needed
  if (min_lod == INT_MIN)
    min_lod = tile_size >> 1;
  if (max_lod == INT_MIN)
    max_lod = tile_size << 2;

  file_ext = no_kmz ? ".kml" : ".kmz";

  if (!assetref.empty()) {
    if (!imagery_product.empty() || !mask_product.empty()) {
      notify(NFY_FATAL, "--assetref or --kip/--kmp must be specified");
    }
    std::string ref;
    try {
      ref = AssetDefs::GuessAssetVersionRef(assetref, std::string());
    } catch (const std::exception &e) {
      notify(NFY_FATAL, "%s", e.what());
    } catch (...) {
      notify(NFY_FATAL, "Unknown error");
    }
    AssetVersion asset_version(ref);
    if (!asset_version) {
      notify(NFY_FATAL, "Failed to open asset %s", assetref.c_str());
    }
    std::vector<std::string> outfnames;
    asset_version->GetOutputFilenames(outfnames);
    if (outfnames.size() < 1) {
      notify(NFY_FATAL, "Failed to open asset %s", assetref.c_str());
    }

    imagery_product = outfnames[0];
    if (outfnames.size() >= 2)
      mask_product = outfnames[1];
  }

  // open imagery product (rgb)
  if (imagery_product.empty() || !khHasExtension(imagery_product, ".kip"))
    notify(NFY_FATAL, "You must specify a valid imagery product (*.kip)");
  rp_rgb = khRasterProduct::Open(imagery_product);
  if (!rp_rgb || rp_rgb->type() != khRasterProduct::Imagery)
    notify(NFY_FATAL, "Failed to open imagery product %s", imagery_product.c_str());

  north_boundary = std::min(north_boundary, rp_rgb->degOrMeterNorth());
  south_boundary = std::max(south_boundary, rp_rgb->degOrMeterSouth());
  east_boundary = std::min(east_boundary, rp_rgb->degOrMeterEast());
  west_boundary = std::max(west_boundary, rp_rgb->degOrMeterWest());
  if (north_boundary <= south_boundary || east_boundary <= west_boundary) {
    notify(NFY_FATAL, "The chosen boundaries and kip result in zero area.\n");
  }
  khCutExtent::cut_extent = khExtents<double>(
      NSEWOrder, north_boundary, south_boundary, east_boundary, west_boundary);

  // open mask product (alpha) if available
  if (mask_product.empty() || !khHasExtension(mask_product, ".kmp")) {
    notify(NFY_WARN, "You didn't specify a valid mask product (*.kmp)");
  } else {
    rp_alpha = khRasterProduct::Open(mask_product);
    if (!rp_alpha || rp_alpha->type() != khRasterProduct::AlphaMask)
      notify(NFY_FATAL, "Failed to open mask product %s", mask_product.c_str());
  }

  notify(NFY_DEBUG, "extents n=%lf s=%lf e=%lf w=%lf",
         north_boundary, south_boundary, east_boundary, west_boundary);

  notify(NFY_DEBUG, "min level=%d, max level=%d",
         rp_rgb->minLevel(), rp_rgb->maxLevel());

  jpg_compressor = new JPEGCompressor(tile_size, tile_size, 3, jpg_quality);
  png_compressor = NewPNGCompressor(tile_size, tile_size, 4);

  // find the last 1x1 level and call it zero
  std::uint32_t zero_level = UINT_MAX;
  for (std::uint32_t l = rp_rgb->minLevel();
       l <= rp_rgb->maxLevel(); ++l) {
    if (rp_rgb->level(l).tileExtents().numRows() == 1 &&
        rp_rgb->level(l).tileExtents().numCols() == 1) {
      zero_level = l;
    } else {
      break;
    }
  }
  if (zero_level == UINT_MAX)
    notify(NFY_FATAL, "Unable to find a suitable zero level");

  int total_tiles = 0;
  for (std::uint32_t lev = zero_level; lev <= rp_rgb->maxLevel(); ++lev) {
    total_tiles += (rp_rgb->level(lev).tileExtents().numRows() *
                    rp_rgb->level(lev).tileExtents().numCols());
  }
  total_tiles = total_tiles * (RasterProductTileResolution / tile_size) *
                              (RasterProductTileResolution / tile_size);

  notify(NFY_NOTICE, "Begin KML super-overlay extraction...");
  if (!url_root.empty() && !EndsWith(url_root, "/"))
    url_root += "/";
  OutputIndex(khCutExtent::cut_extent,
              url_root, rp_rgb->GetAcquisitionDate());
  khProgressMeter progress(total_tiles, "Image Tiles");

  // begin recursive descent
  DrillDown("0", khTileAddr(zero_level,
            rp_rgb->level(zero_level).tileExtents().beginRow(),
            rp_rgb->level(zero_level).tileExtents().beginCol()),
            &progress);

  printf("\n");

  tile_stats.Dump(total_tiles);

  delete jpg_compressor;
  delete png_compressor;
  return 0;
}

bool DrillDown(const std::string& name, const khTileAddr& addr,
               khProgressMeter* progress) {
  // the top level min_lod must always be 0 so that this
  // overlay can be seen from space.
  bool top_level = name.length() == 1;
  int min_lod_override = top_level ? 0 : min_lod;
  int max_lod_override = top_level ? -1 : max_lod;

  if (!rp_rgb->validLevel(addr.level) ||
      !rp_rgb->level(addr.level).tileExtents().
                       ContainsRowCol(addr.row, addr.col)) {
    return false;
  }

  progress->incrementDone((RasterProductTileResolution / tile_size) *
                          (RasterProductTileResolution / tile_size));

  // consult opacity mask (if provided) to determine if we can skip
  // this tile, save as jpg or save as png
  khOpacityMask::OpacityType opacity = rp_alpha ?
      rp_alpha->opacityMask()->GetOpacity(addr) : khOpacityMask::Opaque;
  if (opacity == khOpacityMask::Transparent) {
    tile_stats.AddOpacity(opacity);
    // TODO: count transparent children and add to tile_stats
    return false;
  }

  // sometimes the extract will discard the tile due to unknown
  // opacity mask yielding completely transparent alpha
  ImageExtentsMap images;
  ExtractImageryTile(name, addr, &opacity, &images);
  tile_stats.AddOpacity(opacity);
  if (opacity == khOpacityMask::Transparent) {
    // TODO: count transparent children and add to tile_stats
    return false;
  }

  std::ostringstream os;
  os << std::fixed << std::setprecision(17);
  os << kKMLHeader << std::endl;
  os << "<Document>" << std::endl;

  int child_count = 0;

  if (DrillDown(name + "0", addr.QuadChild(0), progress)) {
    AddChild(&os, name + "0", addr.QuadChild(0));
    ++child_count;
  }

  if (DrillDown(name + "1", addr.QuadChild(1), progress)) {
    AddChild(&os, name + "1", addr.QuadChild(1));
    ++child_count;
  }

  if (DrillDown(name + "2", addr.QuadChild(2), progress)) {
    AddChild(&os, name + "2", addr.QuadChild(2));
    ++child_count;
  }

  if (DrillDown(name + "3", addr.QuadChild(3), progress)) {
    AddChild(&os, name + "3", addr.QuadChild(3));
    ++child_count;
  }

  // last level must never turn off
  if (child_count == 0)
    max_lod_override = -1;

  khExtents<double> extents = addr.degExtents(RasterProductTilespaceBase);
  InsertRegion(&os, 1, extents, min_lod_override, max_lod_override);
  int draw_order = name.length() + draw_order_offset;

  for (ImageExtentsMapIterator image = images.begin();
       image != images.end(); ++image) {

    // every document can have only one region so add sub-documents
    os << "  <Document>" << std::endl;
    int min_lod_img = min_lod_override;
    int max_lod_img = max_lod_override;
    if (min_lod_img != 0 || max_lod_img != -1) {
      const double n = std::min(extents.north(), image->second.north());
      const double s = std::max(extents.south(), image->second.south());
      const double e = std::min(extents.east(), image->second.east());
      const double w = std::max(extents.west(), image->second.west());
      const double area1 = (extents.north() - extents.south()) *
                           (extents.east() - extents.west());
      const double area2 = (n - s) * (e - w);
      if (area1 != area2) {
        double lod_ratio = sqrt(area2/area1);
        if (min_lod_img != 0) {
          min_lod_img = static_cast<int>(min_lod_img * lod_ratio);
        }
        if (max_lod_img != -1) {
          max_lod_img = static_cast<int>(max_lod_img * lod_ratio);
        }
      }
    }

    InsertRegionNoLodCalc(&os, 2, image->second, min_lod_img, max_lod_img);

    os << "    <GroundOverlay>" << std::endl;
    os << "      <drawOrder>" << draw_order << "</drawOrder>" << std::endl;
    os << "      <Icon>" << std::endl;
    os << "        <href>../" << image->first << "</href>" << std::endl;
    os << "      </Icon>" << std::endl;
    os << "      <LatLonBox>" << std::endl;
    os << "        <north>" << image->second.north() << "</north>" << std::endl;
    os << "        <south>" << image->second.south() << "</south>" << std::endl;
    os << "        <east>" << image->second.east() << "</east>" << std::endl;
    os << "        <west>" << image->second.west() << "</west>" << std::endl;
    os << "      </LatLonBox>" << std::endl;
    os << "    </GroundOverlay>" << std::endl;

    // output debug geometry
    // blue X on tile means jpg, red means png
    if (debug) {
      AddDebugGeometry(&os, image->second, image->first, draw_order * 2);
    }

    os << "  </Document>" << std::endl;
  }

  os << "</Document>" << std::endl;
  os << "</kml>" << std::endl;

  std::string kml_path(out_dir + "/kml/" + name + ".kml");
  if (!khWriteSimpleFile(kml_path, os.str().c_str(), os.str().length()))
    notify(NFY_WARN, "Problem writing kml file: %s", kml_path.c_str());
  if (!no_kmz) {
    std::string kmz_path = khReplaceExtension(kml_path, ".kmz");
    std::string cmd = "/usr/bin/zip -m -q " + kmz_path + " " + kml_path;
    if (system(cmd.c_str()) == -1)
      notify(NFY_WARN, "Failed to make kmz - %s", cmd.c_str());
  }

  return true;
}

void AddChild(std::ostringstream* os, const std::string& name,
              const khTileAddr& addr) {
  khExtents<double> extents = addr.degExtents(RasterProductTilespaceBase);
  *os << "  <NetworkLink>" << std::endl;
  *os << "    <name>" << name << "</name>" << std::endl;
  // max lod of networklink hierarchy must always be -1
  // so that you can't pass through it without loading
  InsertRegion(os, 2, extents, min_lod, -1);
  *os << "    <Link>" << std::endl;
  *os << "      <href>" << name << file_ext << "</href>" << std::endl;
  *os << "      <viewRefreshMode>onRegion</viewRefreshMode>" << std::endl;
  *os << "    </Link>" << std::endl;
  *os << "  </NetworkLink>" << std::endl;
}

bool OutputIndex(const khExtents<double>& extents,
                 const std::string& url_root, const std::string& time_stamp) {
  std::ostringstream os;
  os << std::fixed << std::setprecision(17);
  os << kKMLHeader << std::endl;
  os << "<NetworkLink>" << std::endl;
  os << "  <name>" << layer_name << "</name>" << std::endl;
  if (!time_stamp.empty() && time_stamp != kUnknownDate) {
    os << "  <TimeStamp>" << std::endl;
    // http://code.google.com/apis/kml/documentation/kmlreference.html#timestamp
    // Print TimeStamp depending on various forms of Fusion time_stamp.
    // YYYY-MM-DD -> YYYY-MM-DD (date)
    // YYYY-MM-00 -> YYYY-MM    (gYearMonth)
    // YYYY-00-00 -> YYYY       (gYear)
    const size_t remove_pos = time_stamp.find("-00", 4);
    if (remove_pos == std::string::npos) {
      os << "    <when>" << time_stamp << "</when>" << std::endl;
    } else {
      os << "    <when>" << time_stamp.substr(0, remove_pos) << "</when>"
         << std::endl;
    }
    os << "  </TimeStamp>" << std::endl;
  }
  os << "  <Style>" << std::endl;
  os << "    <ListStyle>" << std::endl;
  os << "      <listItemType>checkHideChildren</listItemType>" << std::endl;
  os << "    </ListStyle>" << std::endl;
  os << "  </Style>" << std::endl;
  InsertRegion(&os, 1, extents, 0, -1);
  os << "  <Link>" << std::endl;
  os << "    <href>" << url_root << "kml/0" << file_ext << "</href>" << std::endl;
  os << "    <viewRefreshMode>onRegion</viewRefreshMode>" << std::endl;
  os << "  </Link>" << std::endl;
  os << "</NetworkLink>" << std::endl;
  os << "</kml>" << std::endl;

  return khWriteSimpleFile(out_dir + "/index.kml", os.str().c_str(),
                           os.str().length());
}

void InsertRegion(std::ostringstream* os, int indent,
                  const khExtents<double>& extents,
                  int min_lod, int max_lod) {
  const double n = std::min(extents.north(), khCutExtent::cut_extent.north());
  const double s = std::max(extents.south(), khCutExtent::cut_extent.south());
  const double e = std::min(extents.east(), khCutExtent::cut_extent.east());
  const double w = std::max(extents.west(), khCutExtent::cut_extent.west());
  if (min_lod != 0 || max_lod != -1) {
    const double area1 = (extents.north() - extents.south()) *
                         (extents.east() - extents.west());
    const double area2 = (n - s) * (e - w);
    if (area1 != area2) {
      const double lod_ratio = sqrt(area2/area1);
      if (min_lod != 0) {
        min_lod = static_cast<int>(min_lod * lod_ratio);
      }
      if (max_lod != -1) {
        max_lod = static_cast<int>(max_lod * lod_ratio);
      }
    }
  }
  std::string sp(2 * indent, ' ');
  *os << sp << "<Region>" << std::endl;
  *os << sp << "  <LatLonAltBox>" << std::endl;
  *os << sp << "    <north>" << n << "</north>" << std::endl;
  *os << sp << "    <south>" << s << "</south>" << std::endl;
  *os << sp << "    <east>" << e << "</east>" << std::endl;
  *os << sp << "    <west>" << w << "</west>" << std::endl;
  *os << sp << "  </LatLonAltBox>" << std::endl;
  *os << sp << "  <Lod>" << std::endl;
  *os << sp << "    <minLodPixels>" << min_lod << "</minLodPixels>" << std::endl;
  *os << sp << "    <maxLodPixels>" << max_lod << "</maxLodPixels>" << std::endl;
  *os << sp << "  </Lod>" << std::endl;
  *os << sp << "</Region>" << std::endl;
}

void InsertRegionNoLodCalc(std::ostringstream* os, int indent,
                  const khExtents<double>& extents,
                  const int min_lod, const int max_lod) {
  std::string sp(2 * indent, ' ');
  *os << sp << "<Region>" << std::endl;
  *os << sp << "  <LatLonAltBox>" << std::endl;
  *os << sp << "    <north>" << extents.north() << "</north>" << std::endl;
  *os << sp << "    <south>" << extents.south() << "</south>" << std::endl;
  *os << sp << "    <east>" << extents.east() << "</east>" << std::endl;
  *os << sp << "    <west>" << extents.west() << "</west>" << std::endl;
  *os << sp << "  </LatLonAltBox>" << std::endl;
  *os << sp << "  <Lod>" << std::endl;
  *os << sp
      << "    <minLodPixels>" << min_lod << "</minLodPixels>" << std::endl;
  *os << sp
      << "    <maxLodPixels>" << max_lod << "</maxLodPixels>" << std::endl;
  *os << sp << "  </Lod>" << std::endl;
  *os << sp << "</Region>" << std::endl;
}

/*
 * Debugging geometry is useful to see when image tiles are fading in/out
 * blended tiles (PNG) will be drawn with a red outline
 * solid tiles (JPG) will be drawn with a blue outline
 */
void AddDebugGeometry(std::ostringstream* os, const khExtents<double>& extents,
                      const std::string& image_name, int draw_order) {
  std::string line_color;
  std::string poly_color;
  if(khHasExtension(image_name, "jpg")) {
    line_color = "e0ff0000";    // Blue
    poly_color = "a0ff0000";
  } else {
    line_color = "e00000ff";    // Red
    poly_color = "a00000ff";
  }
  std::string sp(4, ' ');
  int alt = 100;

  *os << sp << "<Placemark>" << std::endl;
  *os << sp << "  <drawOrder>" << draw_order << "</drawOrder>" << std::endl;
  *os << sp << "  <name>" << image_name << "</name>" << std::endl;
  *os << sp << "  <Style>" << std::endl;
  *os << sp << "    <LineStyle>" << std::endl;
  *os << sp << "      <color>" << line_color << "</color>" << std::endl;
  *os << sp << "      <width>5</width>" << std::endl;
  *os << sp << "    </LineStyle>" << std::endl;
  *os << sp << "    <PolyStyle>" << std::endl;
  *os << sp << "      <color>" << poly_color << "</color>" << std::endl;
  *os << sp << "    </PolyStyle>" << std::endl;
  *os << sp << "  </Style>" << std::endl;
  *os << sp << "  <LineString>" << std::endl;
  //*os << sp << "    <altitudeMode>relativeToGround</altitudeMode>" << std::endl;
  *os << sp << "    <tessellate>1</tessellate>" << std::endl;
  *os << sp << "    <coordinates>" << std::endl;
  *os << sp << "      " << extents.west() << ", " << extents.south() << ", " << alt << std::endl;
  *os << sp << "      " << extents.west() << ", " << extents.north() << ", " << alt << std::endl;
  *os << sp << "      " << extents.east() << ", " << extents.north() << ", " << alt << std::endl;
  *os << sp << "      " << extents.east() << ", " << extents.south() << ", " << alt << std::endl;
  *os << sp << "      " << extents.west() << ", " << extents.south() << ", " << alt << std::endl;
  *os << sp << "    </coordinates>" << std::endl;
  *os << sp << "  </LineString>" << std::endl;
  *os << sp << "</Placemark>" << std::endl;
}

typedef khRasterTile<unsigned char, RasterProductTileResolution,
                     RasterProductTileResolution, 4> FourChannelTile;
typedef khRasterTileBorrowBufs<FourChannelTile> FourChannelBorrowTile;


void ExtractImageryTile(const std::string& name, const khTileAddr& addr,
                        khOpacityMask::OpacityType *opacity,
                        ImageExtentsMap* images) {
  const bool is_monochromatic = rp_rgb->level(addr.level).IsMonoChromatic();
  ImageryProductTile rgb_tile(is_monochromatic);
  rp_rgb->level(addr.level).ReadTile(addr.row, addr.col, rgb_tile);

  if (*opacity != khOpacityMask::Opaque) {
    khRasterProductLevel* alpha_level = &rp_alpha->level(
        std::min(addr.level, rp_alpha->maxLevel()));

    inset_alpha_reader.ReadTile(alpha_level, addr, alpha_tile,
                                true /* fillMissingWithZero */);

    if (*opacity == khOpacityMask::Unknown) {
      *opacity = ComputeOpacity(alpha_tile);
    }
    if (*opacity == khOpacityMask::Transparent) {
      return;
    }
  }

  // break up image directory based on the length of the name
  std::string split_name;
  for (unsigned int n = 0; n < name.length(); n += image_dir_break) {
    if (split_name.length() != 0)
      split_name += "/";
    split_name += name.substr(n, std::min(image_dir_break,
          static_cast<unsigned int> (name.length() - n)));
  }

  const khExtents<double> extents = addr.degExtents(RasterProductTilespaceBase);
  const int grid_size = RasterProductTileResolution / tile_size;
  const double lat_step = fabs((extents.north() - extents.south())
                               / double(grid_size));
  const double lon_step = fabs((extents.east() - extents.west())
                               / double(grid_size));
  const double pxl_height = fabs((extents.north() - extents.south()) /
                                 RasterProductTileResolution);
  const double pxl_width = fabs((extents.east() - extents.west()) /
                                 RasterProductTileResolution);
  for (int row = 0; row < grid_size; ++row) {
    for (int col = 0; col < grid_size; ++col) {

      khExtents<double> sub_extents(RowColOrder,
                                   extents.south() + (lat_step * row),
                                   extents.south() + (lat_step * (row + 1)),
                                   extents.west() + (lon_step * col),
                                   extents.west() + (lon_step * (col + 1)));
      const khExtents<double> cut_extents = khExtents<double>::Intersection(
          sub_extents, khCutExtent::cut_extent);


      // ignore any sub-tiles that are outside our image extents
      if (cut_extents.degenerate()) {
        continue;
      }

      const khExtents<std::uint32_t> cut_extents_pxl(
          NSEWOrder,
          static_cast<int>(floor((cut_extents.north() - extents.south())
                            / pxl_height)),
          static_cast<int>(ceil((cut_extents.south() - extents.south())
                            / pxl_height)),
          static_cast<int>(floor((cut_extents.east() - extents.west())
                            / pxl_width)),
          static_cast<int>(ceil((cut_extents.west() - extents.west())
                            / pxl_width)));

      bool need_to_cut = !kml_only && (sub_extents != cut_extents);
      const int width = need_to_cut ? cut_extents_pxl.width() : tile_size;
      const int height = need_to_cut ? cut_extents_pxl.height() : tile_size;

      // Another degenerate case, skip this tile for this level. It will
      // eventually show up in lower levels anyway.
      // This may happen when the source image extends less than one pixel
      // in height or width in a sub-tile.
      if (width < 1 || height < 1) {
        continue;
      }

      int sub = (row * grid_size) + col;
      std::string image_name = "imagery/" + split_name + "_" + ToString(sub);

      khOpacityMask::OpacityType sub_opacity = *opacity;
      if (sub_opacity != khOpacityMask::Opaque) {
        sub_opacity = ComputeOpacity(alpha_tile, cut_extents_pxl);
      }

      // ignore transparent sub-tiles
      if (sub_opacity == khOpacityMask::Transparent)
        continue;

      Compressor * compressor = NULL;
      if (sub_opacity == khOpacityMask::Opaque) {
        // create JPG tile
        image_name += ".jpg";
        if (!kml_only) {
          ExtractAndInterleave(rgb_tile, cut_extents_pxl.beginRow(),
                               cut_extents_pxl.beginCol(),
                               cut_extents_pxl.size(),
                               StartUpperLeft, interleave_buffer);
          if (need_to_cut) {
            compressor = new JPEGCompressor(width, height, 3, jpg_quality);
          } else {
            compressor = jpg_compressor;
          }

          compressor->compress((char*)interleave_buffer);
          khWriteSimpleFile(out_dir + "/" + image_name, compressor->data(),
                            compressor->dataLen());
          tile_stats.AddJPG(width * height * 3, compressor->dataLen());
        }
      } else {
        // create PNG tile
        image_name += ".png";
        if (!kml_only) {
          unsigned char* bufs[4] = {
             rgb_tile.bufs[0],
             rgb_tile.bufs[1],
             rgb_tile.bufs[2],
             alpha_tile.bufs[0]
          };
          FourChannelBorrowTile borrow_tile(bufs, is_monochromatic);

          ExtractAndInterleave(borrow_tile, cut_extents_pxl.beginRow(),
                               cut_extents_pxl.beginCol(),
                               cut_extents_pxl.size(),
                               StartUpperLeft, interleave_buffer);
          if (need_to_cut) {
            compressor = NewPNGCompressor(width, height, 4);
          } else {
            compressor = png_compressor;
          }

          compressor->compress((char*)interleave_buffer);
          khWriteSimpleFile(out_dir + "/" + image_name, compressor->data(),
                            compressor->dataLen());
          tile_stats.AddPNG(width * height * 4, compressor->dataLen());
        }
      }
      if (need_to_cut) {
        delete compressor;
      }

      (*images)[image_name] = need_to_cut ? cut_extents : sub_extents;
    }
  }
}
