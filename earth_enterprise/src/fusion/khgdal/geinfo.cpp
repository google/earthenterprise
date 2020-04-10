/******************************************************************************
 ***  Keyhole Note:
 ***
 ***  This file is derived from a GDAL utility file of a similar name.  We've
 ***  modified (will modify) it to make it look and feel more like our own.
 ******************************************************************************/
/******************************************************************************
 * Original Copyright Notice from GDAL as follows:
 * ****************************************************************************
 * Copyright (c) 1998, Frank Warmerdam
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * ***************************************************************************/


#include <gdal.h>
#include <cpl_string.h>
#include <ogr_spatialref.h>
#include "khgdal.h"
#include "khConstants.h"
#include "khGDALDataset.h"
#include <khGetopt.h>
#include <khGuard.h>
#include <khFileUtils.h>
#include <khraster/khRasterProduct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <packetfile/packetfile.h>
#include <packetfile/packetfilereader.h>
#include <packetfile/packetindex.h>
#include <geindex/IndexBundle.h>
#include <geindex/Traverser.h>
#include <filebundle/filebundlereader.h>
#include <qtpacket/quadtreepacket.h>
#include <packetcompress.h>


static int
GDALInfoReportCorner( const OGRSpatialReference *srs,
                      const double *geoxform,
                      const char * corner_name,
                      double x, double y );


/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

void Usage()
{
  fprintf(stderr,
          "\n"
          "usage: geinfo [options] <input>\n"
          "   Supported options are:\n"
          "      --help|-?:    Show this message\n"
          "      --formats:    Show all supported formats\n"
          "      --minmax:     Compute and show min/max pixel values\n"
          "      --sample:     Sample the pixel data (no print)\n"
          "      --nogcp:      Don't show ground control points\n"
          "      --nometa:     Don't show meta data\n"
          "      --writeprj <prjfile>: Write a .prj file with the input's projection\n"
          "      --writetfw <tfwfile>: Write a .tfw file with the input's extents\n"
          "      -a_srs <override srs>: Use given SRS\n"
          "      --srs <override srs>: Use given SRS\n"
          "      --dump:        Used by some formats to dump contents\n"
          "\n"
          );
  exit( 1 );
}


/************************************************************************/
/*                                main()                                */
/************************************************************************/
#define GetNextArg() ((argn < argc) ? argv[argn++] : 0)

int main( int argc, char ** argv )

{
  GDALRasterBandH     hBand;
  int                 i, iBand;
  char                **papszMetadata;
  bool help = false;
  bool bComputeMinMax = false;
  bool bSample = false;
  bool bShowGCPs = true;
  bool bShowMetadata = true;
  bool showFormats = false;
  bool showdms = false;
  bool dump = false;
  bool qtdump = false;
  const char          *pszFilename = NULL;
  std::string prjfile;
  std::string tfwfile;
  std::string overridesrs;

  khGDALInit();

  // process commandline options
  try {
    int argn;
    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.flagOpt("formats", showFormats);
    options.flagOpt("minmax", bComputeMinMax);
    options.flagOpt("sample", bSample);
    options.flagOpt("gcp",  bShowGCPs);
    options.flagOpt("meta", bShowMetadata);
    options.opt("writeprj", prjfile);
    options.opt("writetfw", tfwfile);
    options.opt("a_srs", overridesrs);
    options.opt("srs", overridesrs);
    options.flagOpt("showdms", showdms);
    options.flagOpt("dump", dump);
    options.flagOpt("qtdump", qtdump);

    if (!options.processAll(argc, argv, argn)) {
      Usage();
    }
    if (help) {
      Usage();
    }

    if (qtdump) {
      dump = true;
    }

    if (showFormats) {
      printf( "Supported Formats:\n" );
      for( int iDr = 0; iDr < GDALGetDriverCount(); iDr++ ) {
        GDALDriverH hDriver = GDALGetDriver(iDr);

        printf( "  %s: %s\n",
                GDALGetDriverShortName( hDriver ),
                GDALGetDriverLongName( hDriver ) );
      }
      exit( 0 );
    }

    // process rest of commandline arguments
    pszFilename = GetNextArg();
    if (!pszFilename) {
      fprintf(stderr, "<input> not specified\n");
      Usage();
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }


// ****************************************************************************
// ***  See if it's one of fusion's incarnations
// ****************************************************************************
  if (khDirExists(pszFilename)) {
    try {
      // check for packetfile
      if (PacketFile::IsPacketFile(pszFilename)) {
        geFilePool file_pool;
        std::string filename = pszFilename;
        PacketIndexReader idx_reader(
            file_pool, khComposePath(filename, PacketFile::kIndexBase));
        if (!dump) {
          printf("File Type:   Google Earth Packet File\n");
          printf("Num Packets: %llu\n",
                 (long long unsigned)idx_reader.NumPackets());
          printf("Have CRCs: %s\n",
                 idx_reader.data_has_crc() ? "true" :"false");
          {
            FileBundleReader bundle_reader(file_pool, filename);
            std::vector<std::string> segment_list;
            std::vector<std::string> orig_list;
            bundle_reader.GetSegmentList(&segment_list);
            bundle_reader.GetSegmentOrigPaths(&orig_list);
            printf("Segments:\n");
            printf("   %s", FileBundle::kHeaderFileName.c_str());
            if (!bundle_reader.hdr_orig_abs_path().empty() &&
                (filename != bundle_reader.hdr_orig_abs_path())) {
              printf(" -> %s\n", bundle_reader.hdr_orig_abs_path().c_str());
            } else {
              printf("\n");
            }
            for (unsigned int i = 0; i < segment_list.size(); ++i) {
              printf("   %s", segment_list[i].c_str());
              if (!orig_list[i].empty() && (filename != orig_list[i])) {
                printf(" -> %s\n", orig_list[i].c_str());
              } else {
                printf("\n");
              }
            }
          }
        } else {
          khDeleteGuard<PacketFileReaderBase> reader;
          LittleEndianReadBuffer read_buf;
          LittleEndianReadBuffer decompress_buf;
          if (qtdump) {
            reader = TransferOwnership(
                new PacketFileReaderBase(file_pool, filename));
          }

          // output header to stderr, so scripts that process this output
          // don't have to worry about it
          fprintf(stderr, "addr,offset,size,extra\n");
          PacketIndexEntry entry;
          while (idx_reader.ReadNext(&entry)) {
            printf("%s,%llu,%u,%u\n",
                   entry.qt_path().AsString().c_str(),
                   (long long unsigned)entry.position(),
                   (unsigned)entry.record_size(),
                   (unsigned)entry.extra());
            if (qtdump) {
              size_t read = reader->ReadAtCRC(entry.position(),
                                              read_buf, entry.record_size());
              if (!KhPktDecompress(&read_buf[0], read, &decompress_buf)) {
                notify(NFY_FATAL, "Unable to decompress qtpacket '%s'",
                       entry.qt_path().AsString().c_str());
              }
              qtpacket::KhQuadTreePacket16 qtpack;
              decompress_buf >> qtpack;
              std::string dump = qtpack.ToString(entry.qt_path().Level() == 0,
                                                 false /* no details */);
              printf("----------\n%s\n----------\n", dump.c_str());
            }
          }
        }
        exit(0);
      }

      // check for geindex
      if (geindex::IsGEIndex(pszFilename)) {
        geFilePool file_pool;
        std::string filename = pszFilename;
        geindex::Header header(file_pool, pszFilename);
        if (!dump) {
          printf("File Type:           Google Earth Index\n");
          printf("File Format Version: %u\n",
                 (unsigned)header.fileFormatVersion);
          printf("Content Description: %s\n", header.contentDesc.c_str());
          printf("Unused space:        %llu\n",
                 (long long unsigned)header.wastedSpace);
          printf("Slot type:           %s\n",
                 header.slotsAreSingle ? "single" : "multiple");
          {
            FileBundleReader bundle_reader(file_pool, filename);
            std::vector<std::string> segment_list;
            std::vector<std::string> orig_list;
            bundle_reader.GetSegmentList(&segment_list);
            bundle_reader.GetSegmentOrigPaths(&orig_list);
            printf("Segments:\n");
            printf("   %s", FileBundle::kHeaderFileName.c_str());
            if (!bundle_reader.hdr_orig_abs_path().empty() &&
                (filename != bundle_reader.hdr_orig_abs_path())) {
              printf(" -> %s\n", bundle_reader.hdr_orig_abs_path().c_str());
            } else {
              printf("\n");
            }
            for (unsigned int i = 0; i < segment_list.size(); ++i) {
              printf("   %s", segment_list[i].c_str());
              if (!orig_list[i].empty() && (filename != orig_list[i])) {
                printf(" -> %s\n", orig_list[i].c_str());
              } else {
                printf("\n");
              }
            }
          }
          printf("Packetfiles:\n");
          for (unsigned int i = 0; i < header.PacketFileCount(); ++i) {
            std::string packetfile = header.GetPacketFile(i);
            if (packetfile.empty()) {
              printf("\t%d:<unused>\n", i);
            } else {
              printf("\t%d:%s\n", i, packetfile.c_str());
            }
          }
        } else {
          typedef MergeSource<geindex::TraverserValue<
            geindex::AllInfoBucket::SlotStorageType> > TraverserBase;
          khDeleteGuard<TraverserBase> traverser;

          if (header.contentDesc == kImageryType) {
            typedef geindex::Traverser<geindex::BlendBucket> BlendTraverser;
            traverser = TransferOwnership(
                new geindex::AllInfoAdaptingTraverser<BlendTraverser>(
                    "AdaptingTraverser", geindex::TypedEntry::Imagery,
                    TransferOwnership(
                        new BlendTraverser(
                            "Traverser", file_pool, pszFilename)),
                    0 /* unused override channel id */));
          } else if (header.contentDesc == kTerrainType) {
            typedef geindex::Traverser<geindex::BlendBucket> BlendTraverser;
            traverser = TransferOwnership(
                new geindex::AllInfoAdaptingTraverser<BlendTraverser>(
                    "AdaptingTraverser", geindex::TypedEntry::Terrain,
                    TransferOwnership(
                        new BlendTraverser(
                            "Traverser", file_pool, pszFilename)),
                    0 /* unused override channel id */));
          } else if (header.contentDesc == kVectorGeType) {
            typedef geindex::Traverser<geindex::VectorBucket> VectorTraverser;
            traverser = TransferOwnership(
                new geindex::AllInfoAdaptingTraverser<VectorTraverser>(
                    "AdaptingTraverser", geindex::TypedEntry::VectorGE,
                    TransferOwnership(
                        new VectorTraverser(
                            "Traverser", file_pool, pszFilename)),
                    0 /* unused overrid channel id */));
          } else if (header.contentDesc == kVectorMapsType) {
            typedef geindex::Traverser<geindex::VectorBucket> VectorTraverser;
            traverser = TransferOwnership(
                new geindex::AllInfoAdaptingTraverser<VectorTraverser>(
                    "AdaptingTraverser", geindex::TypedEntry::VectorMaps,
                    TransferOwnership(
                        new VectorTraverser(
                            "Traverser", file_pool, pszFilename)),
                    0 /* unused override channel id */));
          } else if (header.contentDesc == kVectorMapsRasterType) {
            typedef geindex::Traverser<geindex::VectorBucket> VectorTraverser;
            traverser = TransferOwnership(
                new geindex::AllInfoAdaptingTraverser<VectorTraverser>(
                    "AdaptingTraverser", geindex::TypedEntry::VectorMapsRaster,
                    TransferOwnership(
                        new VectorTraverser(
                            "Traverser", file_pool, pszFilename)),
                    0 /* unused override channel id*/));
          } else if (header.contentDesc == "CombinedTmesh") {
            typedef geindex::Traverser<geindex::CombinedTmeshBucket>
              CombinedTmeshTraverser;
            traverser = TransferOwnership(
                new geindex::AllInfoAdaptingTraverser<CombinedTmeshTraverser>(
                    "AdaptingTraverser", geindex::TypedEntry::Terrain,
                    TransferOwnership(
                        new CombinedTmeshTraverser(
                            "Traverser", file_pool, pszFilename)),
                    0 /* unused override channel id */));
          } else if (header.contentDesc == kUnifiedType) {
            typedef geindex::Traverser<geindex::UnifiedBucket>
              UnifiedTraverser;
            traverser = TransferOwnership(
                new geindex::AllInfoAdaptingTraverser<UnifiedTraverser>(
                    "AdaptingTraverser",
                    geindex::TypedEntry::QTPacket /* unused */,
                    TransferOwnership(
                        new UnifiedTraverser(
                            "Traverser", file_pool, pszFilename)),
                    0 /* unused override channel id */));
          } else {
            notify(NFY_FATAL, "I don't know how to dump a '%s' index",
                   header.contentDesc.c_str());
          }
          // output header to stderr, so scripts that process this output
          // don't have to worry about it
          fprintf(stderr, "addr,type,channel,version,file,offset,size,insetId\n");
          do {
            const TraverserBase::MergeType &slot = traverser->Current();
            for (unsigned int i = 0; i < slot.size(); ++i) {
                printf("%s,%s,%u,%u,%u,%llu,%u,%u\n",
                       slot.qt_path().AsString().c_str(),
                       ToString(slot[i].type).c_str(),
                       (unsigned)slot[i].channel,
                       (unsigned)slot[i].version,
                       (unsigned)slot[i].dataAddress.fileNum,
                       (long long unsigned) slot[i].dataAddress.fileOffset,
                       (unsigned)slot[i].dataAddress.size,
                       (unsigned)slot[i].insetId);
            }
          } while (traverser->Advance());
        }
        exit(0);
      }

      // check for raster product
      khDeleteGuard<khRasterProduct> rp(khRasterProduct::Open(pszFilename));
      if (rp) {
        printf( "File Type: Keyhole %s Raster Product\n",
                ToString(rp->type()).c_str());
        printf( "Levels: %u -> %u\n", rp->minLevel(), rp->maxLevel());
        printf( "Extents (NSEW): %.17f, %.17f, %.17f, %.17f\n",
                rp->degOrMeterNorth(), rp->degOrMeterSouth(),
                rp->degOrMeterEast(), rp->degOrMeterWest());
        for (unsigned int i = rp->minLevel(); i <= rp->maxLevel(); ++i) {
          khRasterProduct::Level &level(rp->level(i));
          printf( "Level Tile Extents %2u: (xywh) %u, %u, %u, %u\n", i,
                  level.tileExtents().beginX(),
                  level.tileExtents().beginY(),
                  level.tileExtents().width(),
                  level.tileExtents().height());
        }
#if 0
        for (unsigned int i = rp->minLevel(); i <= rp->maxLevel(); ++i) {
          khRasterProduct::Level &level(rp->level(i));
          printf( "Level Extents %u: %.12f,%.12f,%.12f,%.12f\n", i,
                  level.degTileNorth(), level.degTileSouth(),
                  level.degTileEast(), level.degTileWest());
        }
#endif
        exit(0);
      }
    } catch (const std::exception &e) {
      notify(NFY_FATAL, "%s", e.what());
    } catch (...) {
      notify(NFY_FATAL, "Unknown error");
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Open dataset.                                                   */
  /* -------------------------------------------------------------------- */
  khGDALDataset khDS;
  std::string khDSError;
  try {
    khDS = khGDALDataset(pszFilename, overridesrs);
  } catch (const std::exception &e) {
    khDSError = e.what();
  } catch (...) {
    khDSError = "Unknown error";
  }

  GDALDataset *dataset = khDS;
  if (!dataset) {
    dataset = (GDALDataset*)GDALOpen( pszFilename, GA_ReadOnly );
    if (!dataset) {
      fprintf( stderr, "Open failed\n");
      exit( 1 );
    }
  }


  /* -------------------------------------------------------------------- */
  /*      Gather (AND SAVE) the common stuff                              */
  /* -------------------------------------------------------------------- */
  khSize<std::uint32_t> rasterSize(dataset->GetRasterXSize(),
                            dataset->GetRasterYSize());

  /* -------------------------------------------------------------------- */
  /*      Report general info.                                            */
  /* -------------------------------------------------------------------- */
  GDALDriver *driver = dataset->GetDriver();
  printf( "File Type:   %s/%s\n",
          GDALGetDriverShortName( driver ),
          GDALGetDriverLongName( driver ) );

  printf( "Raster Size: %d,%d\n", rasterSize.width, rasterSize.height);

  /* -------------------------------------------------------------------- */
  /*      Report Geotransform.                                            */
  /* -------------------------------------------------------------------- */
  khGeoExtents geoExtents;
  if (khDS) {
    geoExtents = khDS.geoExtents();
  } else {
    double geoTransform[6];
    if (dataset->GetGeoTransform(geoTransform) == CE_None) {
      geoExtents = khGeoExtents(geoTransform, rasterSize);
    }
  }
  if (!geoExtents.empty()) {
    printf("Extents:     (ns) %.12f,%.12f\n"
           "             (ew) %.12f,%.12f\n",
           geoExtents.extents().north(),
           geoExtents.extents().south(),
           geoExtents.extents().east(),
           geoExtents.extents().west());
    printf("Pixel Size:  (xy) %.12f,%.12f\n",
           geoExtents.pixelWidth(),
           geoExtents.pixelHeight());

    if (!tfwfile.empty()) {
      if (!GDALWriteWorldFile(khDropExtension(tfwfile).c_str(),
                              khExtension(tfwfile, false).c_str(),
                              (double*)geoExtents.geoTransform())) {
        fprintf( stderr, "Unable to write %s\n",
                 tfwfile.c_str());
        exit( 1 );
      }
    }
  } else {
    printf("Extents:     ***** NONE *****\n");
    printf("Pixel Size:  ***** NONE *****\n");
  }


  /* -------------------------------------------------------------------- */
  /*      Report Keyhole Extents.                                         */
  /* -------------------------------------------------------------------- */
  printf("Keyhole Normalized {\n");
  if (khDS) {
    try {
      khSize<std::uint32_t> rasterSize = khDS.normalizedRasterSize();
      khGeoExtents   geoExtents = khDS.normalizedGeoExtents();
      unsigned int           toplevel   = khDS.normalizedTopLevel();
      printf("     Raster Size: %d,%d\n",
             rasterSize.width, rasterSize.height);
      printf("     Extents:     (ns) %.12f,%.12f\n"
             "                  (ew) %.12f,%.12f\n",
             geoExtents.extents().north(),
             geoExtents.extents().south(),
             geoExtents.extents().east(),
             geoExtents.extents().west());
      if (showdms) {
        printf("    Extents(DMS):(ns) %s,",
                GDALDecToDMS(geoExtents.extents().north(), "Lat", 2));
        printf("%s\n",
               GDALDecToDMS(geoExtents.extents().south(), "Lat", 2));
        printf("                 (ew) %s,",
               GDALDecToDMS(geoExtents.extents().east(),  "Long", 2));
        printf("%s\n",
               GDALDecToDMS(geoExtents.extents().west(),  "Long", 2));
      }
      printf("     Pixel Size:  (xy) %.12f,%.12f\n",
             geoExtents.pixelWidth(),
             geoExtents.pixelHeight());
      printf("     Top level:   %u\n", toplevel);
      if (khDS.needReproject()) {
        printf("     Need Reproject\n");
      }
#if 1
      // I don't know if we want to advertise this.
      if (khDS.needSnapUp()) {
        printf("     Need SnapUp\n");
      }
#endif
      // I don't know if we want to advertise this.
      if (khDS.normIsCropped()) {
        printf("     Cropped\n");
      }
    } catch (const std::exception &e) {
      printf("    ***** %s *****\n", e.what());
    } catch (...) {
      printf("    ***** Unknown error *****\n");
    }
  } else {
    printf("    ***** %s *****\n", khDSError.c_str());
  }
  printf("}\n");


  /* -------------------------------------------------------------------- */
  /*      Report SRS.                                                     */
  /* -------------------------------------------------------------------- */
  OGRSpatialReference tmpSRS;
  const OGRSpatialReference *srs = 0;
  if (khDS) {
    srs = &khDS.ogrSRS();
  } else {
    std::string srsStr = dataset->GetProjectionRef();
    if (srsStr.size()) {
      const char *wkt = srsStr.c_str();
      if ((tmpSRS.importFromWkt((char**)&wkt) == OGRERR_NONE) &&
          tmpSRS.GetRoot()) {
        srs = &tmpSRS;
      }
    }
  }
  if (srs) {
    char *prettyWkt = 0;
    srs->exportToPrettyWkt(&prettyWkt);
    printf( "Spatial Reference System:\n%s\n", prettyWkt );

    if (!prjfile.empty()) {
      khWriteFileCloser prjFile(::open(prjfile.c_str(),
                                       O_CREAT|O_TRUNC|O_WRONLY, 0666));
      if (!prjFile.valid()) {
        fprintf( stderr, "Unable to open %s: %s\n",
                 prjfile.c_str(), khstrerror(errno).c_str());
        exit( 1 );
      }

      if (!khWriteAll(prjFile.fd(), prettyWkt, strlen(prettyWkt))) {
        fprintf( stderr, "Unable to write %s: %s\n",
                 prjfile.c_str(), khstrerror(errno).c_str());
        exit( 1 );
      }
      write(prjFile.fd(), "\n", 1);
    }

    CPLFree( prettyWkt );
  }


  /* -------------------------------------------------------------------- */
  /*      Report GCPs.                                                    */
  /* -------------------------------------------------------------------- */
  void* hDataset = dataset;
  if( bShowGCPs && GDALGetGCPCount( hDataset ) > 0 )
  {
    printf( "GCP Projection: %s\n", GDALGetGCPProjection(hDataset) );
    for( i = 0; i < GDALGetGCPCount(hDataset); i++ )
    {
      const GDAL_GCP      *psGCP;

      psGCP = GDALGetGCPs( hDataset ) + i;

      printf( "GCP[%3d]: Id=%s, Info=%s\n"
              "          (%g,%g) -> (%g,%g,%g)\n",
              i, psGCP->pszId, psGCP->pszInfo,
              psGCP->dfGCPPixel, psGCP->dfGCPLine,
              psGCP->dfGCPX, psGCP->dfGCPY, psGCP->dfGCPZ );
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Report metadata.                                                */
  /* -------------------------------------------------------------------- */
  papszMetadata = GDALGetMetadata( hDataset, NULL );
  if( bShowMetadata && CSLCount(papszMetadata) > 0 )
  {
    printf( "Metadata:\n" );
    for( i = 0; papszMetadata[i] != NULL; i++ )
    {
      printf( "  %s\n", papszMetadata[i] );
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Report subdatasets.                                             */
  /* -------------------------------------------------------------------- */
  papszMetadata = GDALGetMetadata( hDataset, "SUBDATASETS" );
  if( CSLCount(papszMetadata) > 0 )
  {
    printf( "Subdatasets:\n" );
    for( i = 0; papszMetadata[i] != NULL; i++ )
    {
      printf( "  %s\n", papszMetadata[i] );
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Report corners.                                                 */
  /* -------------------------------------------------------------------- */
  if (!geoExtents.empty()) {
    printf( "Corner Coordinates:\n" );
    GDALInfoReportCorner( srs, geoExtents.geoTransform(),
                          "Upper Left", 0.0, 0.0 );
    GDALInfoReportCorner( srs, geoExtents.geoTransform(),
                          "Lower Left", 0.0, rasterSize.height);
    GDALInfoReportCorner( srs, geoExtents.geoTransform(),
                          "Upper Right", rasterSize.width, 0.0 );
    GDALInfoReportCorner( srs, geoExtents.geoTransform(),
                          "Lower Right",
                          rasterSize.width, rasterSize.height);
    GDALInfoReportCorner( srs, geoExtents.geoTransform(),
                          "Center",
                          rasterSize.width/2.0, rasterSize.height/2.0);
  }

  /* ==================================================================== */
  /*      Loop over bands.                                                */
  /* ==================================================================== */
  for( iBand = 0; iBand < GDALGetRasterCount( hDataset ); iBand++ )
  {
    double      dfMin, dfMax, adfCMinMax[2], dfNoData;
    int         bGotMin, bGotMax, bGotNodata;
    int         nBlockXSize, nBlockYSize;

    hBand = GDALGetRasterBand( hDataset, iBand+1 );

    if( bSample )
    {
      float afSample[10000];
      int   nCount;

      nCount = GDALGetRandomRasterSample( hBand, 10000, afSample );
      printf( "Got %d samples.\n", nCount );
    }

    GDALGetBlockSize( hBand, &nBlockXSize, &nBlockYSize );
    printf( "Band %d Block=%dx%d Type=%s, ColorInterp=%s\n", iBand+1,
            nBlockXSize, nBlockYSize,
            GDALGetDataTypeName(
                GDALGetRasterDataType(hBand)),
            GDALGetColorInterpretationName(
                GDALGetRasterColorInterpretation(hBand)) );

    if( GDALGetDescription( hBand ) != NULL
        && strlen(GDALGetDescription( hBand )) > 0 )
      printf( "  Description = %s\n", GDALGetDescription(hBand) );

    dfMin = GDALGetRasterMinimum( hBand, &bGotMin );
    dfMax = GDALGetRasterMaximum( hBand, &bGotMax );
    if( bGotMin || bGotMax || bComputeMinMax )
    {
      printf( "  " );
      if( bGotMin )
        printf( "Min=%.3f ", dfMin );
      if( bGotMax )
        printf( "Max=%.3f ", dfMax );

      if( bComputeMinMax )
      {
        GDALComputeRasterMinMax( hBand, TRUE, adfCMinMax );
        printf( "  Computed Min/Max=%.3f,%.3f",
                adfCMinMax[0], adfCMinMax[1] );
      }

      printf( "\n" );
    }

    dfNoData = GDALGetRasterNoDataValue( hBand, &bGotNodata );
    if( bGotNodata )
    {
      printf( "  NoData Value=%g\n", dfNoData );
    }

    if( GDALGetOverviewCount(hBand) > 0 )
    {
      int         iOverview;

      printf( "  Overviews: " );
      for( iOverview = 0;
           iOverview < GDALGetOverviewCount(hBand);
           iOverview++ )
      {
        GDALRasterBandH hOverview;

        if( iOverview != 0 )
          printf( ", " );

        hOverview = GDALGetOverview( hBand, iOverview );
        printf( "%dx%d",
                GDALGetRasterBandXSize( hOverview ),
                GDALGetRasterBandYSize( hOverview ) );
      }
      printf( "\n" );
    }

    if( GDALHasArbitraryOverviews( hBand ) )
    {
      printf( "  Overviews: arbitrary\n" );
    }

    if( strlen(GDALGetRasterUnitType(hBand)) > 0 )
    {
      printf( "  Unit Type: %s\n", GDALGetRasterUnitType(hBand) );
    }

    papszMetadata = GDALGetMetadata( hBand, NULL );
    if( CSLCount(papszMetadata) > 0 )
    {
      printf( "Metadata:\n" );
      for( i = 0; papszMetadata[i] != NULL; i++ )
      {
        printf( "  %s\n", papszMetadata[i] );
      }
    }

    if( GDALGetRasterColorInterpretation(hBand) == GCI_PaletteIndex )
    {
      GDALColorTableH     hTable;
      int                 i;

      hTable = GDALGetRasterColorTable( hBand );
      printf( "  Color Table (%s with %d entries)\n",
              GDALGetPaletteInterpretationName(
                  GDALGetPaletteInterpretation( hTable )),
              GDALGetColorEntryCount( hTable ) );

      for( i = 0; i < GDALGetColorEntryCount( hTable ); i++ )
      {
        GDALColorEntry  sEntry;

        GDALGetColorEntryAsRGB( hTable, i, &sEntry );
        printf( "  %3d: %d,%d,%d,%d\n",
                i,
                sEntry.c1,
                sEntry.c2,
                sEntry.c3,
                sEntry.c4 );
      }
    }
  }

  if (!khDS) {
    GDALClose( dataset );
  } else {
    khDS.release();
  }

  GDALDumpOpenDatasets( stderr );

  GDALDestroyDriverManager();
}

/************************************************************************/
/*                        GDALInfoReportCorner()                        */
/************************************************************************/

static int
GDALInfoReportCorner( const OGRSpatialReference *srs,
                      const double *geoxform,
                      const char * corner_name,
                      double x, double y )
{
  printf( "%-11s ", corner_name );

  /* -------------------------------------------------------------------- */
  /*      Transform the point into georeferenced coordinates.             */
  /* -------------------------------------------------------------------- */
  double      dfGeoX, dfGeoY;
  if (geoxform) {
    dfGeoX = geoxform[0] + geoxform[1] * x + geoxform[2] * y;
    dfGeoY = geoxform[3] + geoxform[4] * x + geoxform[5] * y;
  } else {
    printf( "(%7.1f,%7.1f)\n", x, y );
    return FALSE;
  }

  /* -------------------------------------------------------------------- */
  /*      Report the georeferenced coordinates.                           */
  /* -------------------------------------------------------------------- */
  if( ABS(dfGeoX) < 181 && ABS(dfGeoY) < 91 ) {
    printf( "(%12.7f,%12.7f) ", dfGeoX, dfGeoY );

  } else {
    printf( "(%12.3f,%12.3f) ", dfGeoX, dfGeoY );
  }

  /* -------------------------------------------------------------------- */
  /*      Report transformed coordinates                                  */
  /* -------------------------------------------------------------------- */
  if (srs) {
    // cLone just the GeosCS part (drop transformation)
    OGRSpatialReference *latlon = srs->CloneGeogCS();
    OGRCoordinateTransformation *transform =
      (OGRCoordinateTransformation*)
      OCTNewCoordinateTransformation((void*)srs, latlon );
    if (transform) {
      if (OCTTransform(transform,1,&dfGeoX,&dfGeoY,NULL)) {
        printf( "(%s,", GDALDecToDMS( dfGeoX, "Long", 2 ) );
        printf( "%s)", GDALDecToDMS( dfGeoY, "Lat", 2 ) );
      }
      delete transform;
    }
    delete latlon;

  }
  printf( "\n" );
  return TRUE;
}
