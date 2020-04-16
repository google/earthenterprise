/******************************************************************************
 * gereproject.cpp
 *
 * Project:  High Performance Image Reprojector
 * Purpose:  Test program for high performance warper API.
 *
 ******************************************************************************
 * Copyright (c) 2002, i3 - information integration and imaging
 *                          Fort Collin, CO
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
 ******************************************************************************/

#include "gdalwarper.h"
#include "cpl_string.h"
#include "ogr_srs_api.h"
#include "ogr_spatialref.h"
#include <khgdal/khgdal.h>
#include <khTileAddr.h>
#include "khGDALDataset.h"

static GDALDatasetH
GDALWarpCreateOutput( GDALDatasetH hSrcDS, const char *pszFilename,
                      const char *pszFormat, const char *pszSourceSRS,
                      const char *pszTargetSRS, int nOrder,
                      char **papszCreateOptions, GDALDataType eDT,
                      bool queryOnly);

static double          dfMinX=0.0, dfMinY=0.0, dfMaxX=0.0, dfMaxY=0.0;
static double          dfXRes=0.0, dfYRes=0.0;
static int             nForcePixels=0, nForceLines=0, bQuiet = FALSE;
static bool            snapup = false;

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
  printf(
      "Usage: gereproject [--version] [--formats] [--query] [--snapup]\n"
      "    [-s_srs srs_def] [-t_srs srs_def] [-order n] [-et err_threshold]\n"
      "    [-te xmin ymin xmax ymax] [-tr xres yres] [-ts width height]\n"
      "    [-wo \"NAME=VALUE\"] [-ot Byte/Int16/...] [-wt Byte/Int16]\n"
      "    [-rn] [-rb] [-rc] [-rcs] [-wm memory_in_mb] [-multi] [-q]\n"
      "    [-srcnodata value [value...]] [-dstnodata value [value...]]\n"
      "    [-of format] [-co \"NAME=VALUE\"]* srcfile dstfile\n" );
  exit( 1 );
}

/************************************************************************/
/*                             SanitizeSRS                              */
/************************************************************************/

char *SanitizeSRS( const char *pszUserInput )

{
  OGRSpatialReferenceH hSRS;
  char *pszResult = NULL;

  CPLErrorReset();

  hSRS = OSRNewSpatialReference( NULL );
  if( OSRSetFromUserInput( hSRS, pszUserInput ) == OGRERR_NONE )
    OSRExportToWkt( hSRS, &pszResult );
  else
  {
    CPLError( CE_Failure, CPLE_AppDefined,
              "Translating source or target SRS failed:\n%s",
              pszUserInput );
    exit( 1 );
  }

  OSRDestroySpatialReference( hSRS );

  return pszResult;
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int argc, char ** argv )

{
  GDALDatasetH        hSrcDS, hDstDS;
  const char         *pszFormat = "HFA";
  char               *pszTargetSRS = NULL;
  char               *pszSourceSRS = NULL;
  const char         *pszSrcFilename = NULL, *pszDstFilename = NULL;
  int                 bCreateOutput = TRUE, i, nOrder = 0;
  void               *hTransformArg, *hGenImgProjArg=NULL, *hApproxArg=NULL;
  char               **papszWarpOptions = NULL;
  double             dfErrorThreshold = 0.125;
  double             dfWarpMemoryLimit = 200 * 1024 * 1024;
  GDALTransformerFunc pfnTransformer = NULL;
  char                **papszCreateOptions = NULL;
  GDALDataType        eOutputType = GDT_Unknown, eWorkingType = GDT_Unknown;
  GDALResampleAlg     eResampleAlg = GRA_Cubic;
  const char          *pszSrcNodata = NULL;
  const char          *pszDstNodata = NULL;
  int                 bMulti = FALSE;
  bool                queryOnly = false;

  khGDALInit();

  /* -------------------------------------------------------------------- */
  /*      Parse arguments.                                                */
  /* -------------------------------------------------------------------- */
  for( i = 1; i < argc; i++ )
  {
    if( EQUAL(argv[i],"--version") )
    {
      printf( "%s\n", GDALVersionInfo( "--version" ) );
      exit( 0 );
    }
    else if( EQUAL(argv[i],"--formats") )
    {
      int iDr;

      printf( "Supported Formats:\n" );
      for( iDr = 0; iDr < GDALGetDriverCount(); iDr++ )
      {
        GDALDriverH hDriver = GDALGetDriver(iDr);

        printf( "  %s: %s\n",
                GDALGetDriverShortName( hDriver ),
                GDALGetDriverLongName( hDriver ) );
      }

      exit( 0 );
    }
    else if( EQUAL(argv[i],"--query") ) {
      queryOnly = true;
    }
    else if( EQUAL(argv[i],"--snapup") ) {
      snapup = true;
    }
    else if( EQUAL(argv[i],"-co") && i < argc-1 )
    {
      papszCreateOptions = CSLAddString( papszCreateOptions, argv[++i] );
      bCreateOutput = TRUE;
    }
    else if( EQUAL(argv[i],"-wo") && i < argc-1 )
    {
      papszWarpOptions = CSLAddString( papszWarpOptions, argv[++i] );
    }
    else if( EQUAL(argv[i],"-multi") )
    {
      bMulti = TRUE;
    }
    else if( EQUAL(argv[i],"-q") )
    {
      bQuiet = TRUE;
    }
    else if( EQUAL(argv[i],"-of") && i < argc-1 )
    {
      pszFormat = argv[++i];
      bCreateOutput = TRUE;
    }
    else if( EQUAL(argv[i],"-t_srs") && i < argc-1 )
    {
      pszTargetSRS = SanitizeSRS(argv[++i]);
    }
    else if( EQUAL(argv[i],"-s_srs") && i < argc-1 )
    {
      pszSourceSRS = SanitizeSRS(argv[++i]);
    }
    else if( EQUAL(argv[i],"-order") && i < argc-1 )
    {
      nOrder = atoi(argv[++i]);
    }
    else if( EQUAL(argv[i],"-et") && i < argc-1 )
    {
      dfErrorThreshold = atof(argv[++i]);
    }
    else if( EQUAL(argv[i],"-wm") && i < argc-1 )
    {
      if( atof(argv[i+1]) < 4000 )
        dfWarpMemoryLimit = atof(argv[i+1]) * 1024 * 1024;
      else
        dfWarpMemoryLimit = atof(argv[i+1]);
      i++;
    }
    else if( EQUAL(argv[i],"-srcnodata") && i < argc-1 )
    {
      pszSrcNodata = argv[++i];
    }
    else if( EQUAL(argv[i],"-dstnodata") && i < argc-1 )
    {
      pszDstNodata = argv[++i];
    }
    else if( EQUAL(argv[i],"-tr") && i < argc-2 )
    {
      dfXRes = atof(argv[++i]);
      dfYRes = fabs(atof(argv[++i]));
      bCreateOutput = TRUE;
    }
    else if( EQUAL(argv[i],"-ot") && i < argc-1 )
    {
      int iType;

      for( iType = 1; iType < GDT_TypeCount; iType++ )
      {
        if( GDALGetDataTypeName((GDALDataType)iType) != NULL
            && EQUAL(GDALGetDataTypeName((GDALDataType)iType),
                     argv[i+1]) )
        {
          eOutputType = (GDALDataType) iType;
        }
      }

      if( eOutputType == GDT_Unknown )
      {
        printf( "Unknown output pixel type: %s\n", argv[i+1] );
        Usage();
        exit( 2 );
      }
      i++;
      bCreateOutput = TRUE;
    }
    else if( EQUAL(argv[i],"-wt") && i < argc-1 )
    {
      int iType;

      for( iType = 1; iType < GDT_TypeCount; iType++ )
      {
        if( GDALGetDataTypeName((GDALDataType)iType) != NULL
            && EQUAL(GDALGetDataTypeName((GDALDataType)iType),
                     argv[i+1]) )
        {
          eWorkingType = (GDALDataType) iType;
        }
      }

      if( eWorkingType == GDT_Unknown )
      {
        printf( "Unknown output pixel type: %s\n", argv[i+1] );
        Usage();
        exit( 2 );
      }
      i++;
    }
    else if( EQUAL(argv[i],"-ts") && i < argc-2 )
    {
      nForcePixels = atoi(argv[++i]);
      nForceLines = atoi(argv[++i]);
      bCreateOutput = TRUE;
    }
    else if( EQUAL(argv[i],"-te") && i < argc-4 )
    {
      dfMinX = atof(argv[++i]);
      dfMinY = atof(argv[++i]);
      dfMaxX = atof(argv[++i]);
      dfMaxY = atof(argv[++i]);
      bCreateOutput = TRUE;
    }
    else if( EQUAL(argv[i],"-rn") )
      eResampleAlg = GRA_NearestNeighbour;

    else if( EQUAL(argv[i],"-rb") )
      eResampleAlg = GRA_Bilinear;

    else if( EQUAL(argv[i],"-rc") )
      eResampleAlg = GRA_Cubic;

    else if( EQUAL(argv[i],"-rcs") )
      eResampleAlg = GRA_CubicSpline;

    else if( argv[i][0] == '-' )
      Usage();
    else if( pszSrcFilename == NULL )
      pszSrcFilename = argv[i];
    else if( pszDstFilename == NULL )
      pszDstFilename = argv[i];
    else
      Usage();
  }

  if( pszDstFilename == NULL && !queryOnly)
    Usage();

  /* -------------------------------------------------------------------- */
  /*      Open source dataset.                                            */
  /* -------------------------------------------------------------------- */

  // try to open as a khGDALDataset so we can pick up
  // sidecars and support our formats (like KHVR)
  khGDALDataset khDS;
  try {
    khDS = khGDALDataset(pszSrcFilename);
  } catch (...) {
    // ignore all errors here
  }
  hSrcDS = khDS;

  // if we couldn't optn it, fal back to the normal GDAL open
  if (!hSrcDS) {
    hSrcDS = GDALOpen( pszSrcFilename, GA_ReadOnly );
  }

  if( hSrcDS == NULL )
    exit( 2 );

  if( pszSourceSRS == NULL )
  {
    if( GDALGetProjectionRef( hSrcDS ) != NULL
        && strlen(GDALGetProjectionRef( hSrcDS )) > 0 )
      pszSourceSRS = CPLStrdup(GDALGetProjectionRef( hSrcDS ));

    else if( GDALGetGCPProjection( hSrcDS ) != NULL
             && strlen(GDALGetGCPProjection(hSrcDS)) > 0
             && GDALGetGCPCount( hSrcDS ) > 1 )
      pszSourceSRS = CPLStrdup(GDALGetGCPProjection( hSrcDS ));
    else
      pszSourceSRS = CPLStrdup("");
  }

  if( pszTargetSRS == NULL ) {
    OGRSpatialReference srs;
    srs.SetWellKnownGeogCS("WGS84");
    char *tmpWkt;
    srs.exportToWkt(&tmpWkt);
    pszTargetSRS = CPLStrdup(tmpWkt);
  }


  /* -------------------------------------------------------------------- */
  /*      Does the output dataset already exist?                          */
  /* -------------------------------------------------------------------- */
  if (queryOnly) {
    hDstDS = 0;
  } else {
    CPLPushErrorHandler( CPLQuietErrorHandler );
    hDstDS = GDALOpen( pszDstFilename, GA_Update );
    CPLPopErrorHandler();

    if( hDstDS != NULL && bCreateOutput ) {
      fprintf( stderr,
               "Output dataset %s exists,\n"
               "but some commandline options were provided indicating a new dataset\n"
               "should be created.  Please delete existing dataset and run again.\n",
               pszDstFilename );
      exit( 1 );
    }
  }

  /* -------------------------------------------------------------------- */
  /*      If not, we need to create it.                                   */
  /* -------------------------------------------------------------------- */
  if( hDstDS == NULL )
  {
    // if queryOnly, this will never return
    hDstDS = GDALWarpCreateOutput( hSrcDS, pszDstFilename, pszFormat,
                                   pszSourceSRS, pszTargetSRS, nOrder,
                                   papszCreateOptions, eOutputType,
                                   queryOnly);

    if( CSLFetchNameValue( papszWarpOptions, "INIT_DEST" ) == NULL )
      papszWarpOptions = CSLSetNameValue(papszWarpOptions,
                                         "INIT_DEST", "0");
    CSLDestroy( papszCreateOptions );
    papszCreateOptions = NULL;
  }

  if( hDstDS == NULL )
    exit( 1 );

  /* -------------------------------------------------------------------- */
  /*      Create a transformation object from the source to               */
  /*      destination coordinate system.                                  */
  /* -------------------------------------------------------------------- */
  hTransformArg = hGenImgProjArg =
                  GDALCreateGenImgProjTransformer( hSrcDS, pszSourceSRS,
                                                   hDstDS, pszTargetSRS,
                                                   TRUE, 1000.0, nOrder );

  if( hTransformArg == NULL )
    exit( 1 );

  pfnTransformer = GDALGenImgProjTransform;

  /* -------------------------------------------------------------------- */
  /*      Warp the transformer with a linear approximator unless the      */
  /*      acceptable error is zero.                                       */
  /* -------------------------------------------------------------------- */
  if( dfErrorThreshold != 0.0 )
  {
    hTransformArg = hApproxArg =
                    GDALCreateApproxTransformer( GDALGenImgProjTransform,
                                                 hGenImgProjArg, dfErrorThreshold );
    pfnTransformer = GDALApproxTransform;
  }

  /* -------------------------------------------------------------------- */
  /*      Setup warp options.                                             */
  /* -------------------------------------------------------------------- */
  GDALWarpOptions *psWO = GDALCreateWarpOptions();

  psWO->papszWarpOptions = papszWarpOptions;
  psWO->eWorkingDataType = eWorkingType;
  psWO->eResampleAlg = eResampleAlg;

  psWO->hSrcDS = hSrcDS;
  psWO->hDstDS = hDstDS;

  psWO->pfnTransformer = pfnTransformer;
  psWO->pTransformerArg = hTransformArg;

  if( !bQuiet )
    psWO->pfnProgress = GDALTermProgress;

  if( dfWarpMemoryLimit != 0.0 )
    psWO->dfWarpMemoryLimit = dfWarpMemoryLimit;

  /* -------------------------------------------------------------------- */
  /*      Setup band mapping.                                             */
  /* -------------------------------------------------------------------- */
  psWO->nBandCount = GDALGetRasterCount(hSrcDS);
  psWO->panSrcBands = (int *) CPLMalloc(psWO->nBandCount*sizeof(int));
  psWO->panDstBands = (int *) CPLMalloc(psWO->nBandCount*sizeof(int));

  for( i = 0; i < psWO->nBandCount; i++ )
  {
    psWO->panSrcBands[i] = i+1;
    psWO->panDstBands[i] = i+1;
  }

  /* -------------------------------------------------------------------- */
  /*      Setup NODATA options.                                           */
  /* -------------------------------------------------------------------- */
  if( pszSrcNodata != NULL )
  {
    char **papszTokens = CSLTokenizeString( pszSrcNodata );
    int  nTokenCount = CSLCount(papszTokens);

    psWO->padfSrcNoDataReal = (double *)
                              CPLMalloc(psWO->nBandCount*sizeof(double));
    psWO->padfSrcNoDataImag = (double *)
                              CPLMalloc(psWO->nBandCount*sizeof(double));

    for( i = 0; i < psWO->nBandCount; i++ )
    {
      if( i < nTokenCount )
      {
        CPLStringToComplex( papszTokens[i],
                            psWO->padfSrcNoDataReal + i,
                            psWO->padfSrcNoDataImag + i );
      }
      else
      {
        psWO->padfSrcNoDataReal[i] = psWO->padfSrcNoDataReal[i-1];
        psWO->padfSrcNoDataImag[i] = psWO->padfSrcNoDataImag[i-1];
      }
    }

    CSLDestroy( papszTokens );
  }
  if( pszDstNodata != NULL )
  {
    char **papszTokens = CSLTokenizeString( pszDstNodata );
    int  nTokenCount = CSLCount(papszTokens);

    psWO->padfDstNoDataReal = (double *)
                              CPLMalloc(psWO->nBandCount*sizeof(double));
    psWO->padfDstNoDataImag = (double *)
                              CPLMalloc(psWO->nBandCount*sizeof(double));

    for( i = 0; i < psWO->nBandCount; i++ )
    {
      if( i < nTokenCount )
      {
        CPLStringToComplex( papszTokens[i],
                            psWO->padfDstNoDataReal + i,
                            psWO->padfDstNoDataImag + i );
      }
      else
      {
        psWO->padfDstNoDataReal[i] = psWO->padfDstNoDataReal[i-1];
        psWO->padfDstNoDataImag[i] = psWO->padfDstNoDataImag[i-1];
      }

      if( bCreateOutput )
      {
        CPLErr error =
            GDALSetRasterNoDataValue(
                GDALGetRasterBand( hDstDS, psWO->panDstBands[i] ),
                psWO->padfDstNoDataReal[i] );
        if (error != CE_None)
        {
          printf( "Output format `%s' does not support -dstnodata.\n",
                  pszFormat );
          printf( "Consider \"-of GTiff\" or other format.\n");
        }
      }
    }

    CSLDestroy( papszTokens );
  }

  /* -------------------------------------------------------------------- */
  /*      Initialize and execute the warp.                                */
  /* -------------------------------------------------------------------- */
  GDALWarpOperation oWO;

  CPLErr err = CE_None;

  if( oWO.Initialize( psWO ) == CE_None )
  {
    if( bMulti )
      err = oWO.ChunkAndWarpMulti( 0, 0,
                                   GDALGetRasterXSize( hDstDS ),
                                   GDALGetRasterYSize( hDstDS ) );
    else
      err = oWO.ChunkAndWarpImage( 0, 0,
                                   GDALGetRasterXSize( hDstDS ),
                                   GDALGetRasterYSize( hDstDS ) );
  }

  /* -------------------------------------------------------------------- */
  /*      Cleanup                                                         */
  /* -------------------------------------------------------------------- */
  CSLDestroy( papszWarpOptions );

  if( hApproxArg != NULL )
    GDALDestroyApproxTransformer( hApproxArg );

  if( hGenImgProjArg != NULL )
    GDALDestroyGenImgProjTransformer( hGenImgProjArg );

  /* -------------------------------------------------------------------- */
  /*      Cleanup.                                                        */
  /* -------------------------------------------------------------------- */
  GDALClose( hDstDS );
  GDALClose( hSrcDS );

  GDALDumpOpenDatasets( stderr );

  GDALDestroyDriverManager();

  exit( err == CE_None ? 0 : 1 );
}

/************************************************************************/
/*                        GDALWarpCreateOutput()                        */
/*                                                                      */
/*      Create the output file based on various commandline options,    */
/*      and the input file.                                             */
/************************************************************************/

static GDALDatasetH
GDALWarpCreateOutput( GDALDatasetH hSrcDS, const char *pszFilename,
                      const char *pszFormat, const char *pszSourceSRS,
                      const char *pszTargetSRS, int nOrder,
                      char **papszCreateOptions, GDALDataType eDT,
                      bool queryOnly)

{
  GDALDriverH hDriver;
  GDALDatasetH hDstDS;
  void *hTransformArg;
  double adfDstGeoTransform[6];
  int nPixels=0, nLines=0;

  if( eDT == GDT_Unknown )
    eDT = GDALGetRasterDataType(GDALGetRasterBand(hSrcDS,1));

  /* -------------------------------------------------------------------- */
  /*      Find the output driver.                                         */
  /* -------------------------------------------------------------------- */
  hDriver = GDALGetDriverByName( pszFormat );
  if( hDriver == NULL
      || GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATE, NULL ) == NULL )
  {
    int     iDr;

    printf( "Output driver `%s' not recognised or does not support\n",
            pszFormat );
    printf( "direct output file creation.  The following format drivers are configured\n"
            "and support direct output:\n" );

    for( iDr = 0; iDr < GDALGetDriverCount(); iDr++ )
    {
      GDALDriverH hDriver = GDALGetDriver(iDr);

      if( GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATE, NULL) != NULL )
      {
        printf( "  %s: %s\n",
                GDALGetDriverShortName( hDriver  ),
                GDALGetDriverLongName( hDriver ) );
      }
    }
    printf( "\n" );
    exit( 1 );
  }

  /* -------------------------------------------------------------------- */
  /*      Create a transformation object from the source to               */
  /*      destination coordinate system.                                  */
  /* -------------------------------------------------------------------- */
  hTransformArg =
    GDALCreateGenImgProjTransformer( hSrcDS, pszSourceSRS,
                                     NULL, pszTargetSRS,
                                     TRUE, 1000.0, nOrder );

  if( hTransformArg == NULL )
    return NULL;

  /* -------------------------------------------------------------------- */
  /*      Get approximate output definition.                              */
  /* -------------------------------------------------------------------- */
  if( GDALSuggestedWarpOutput( hSrcDS,
                               GDALGenImgProjTransform, hTransformArg,
                               adfDstGeoTransform, &nPixels, &nLines )
      != CE_None )
    return NULL;

  GDALDestroyGenImgProjTransformer( hTransformArg );

  /* -------------------------------------------------------------------- */
  /*      Did the user override some parameters?                          */
  /* -------------------------------------------------------------------- */
  if (snapup && (GetWGS84ProjString() == pszTargetSRS)) {
    double minX = adfDstGeoTransform[0];
    double maxX = adfDstGeoTransform[0] + adfDstGeoTransform[1] * nPixels;
    double maxY = adfDstGeoTransform[3];
    double minY = adfDstGeoTransform[3] + adfDstGeoTransform[5] * nLines;
    unsigned int level = RasterProductTilespaceFlat
                 .LevelFromDegPixelSize(adfDstGeoTransform[1]);
    if (level != RasterProductTilespaceFlat
        .LevelFromDegPixelSize(-adfDstGeoTransform[5])) {
      fprintf(stderr, "Incompatible x/y warped pixel sizes.");
      exit(1);
    }
    double pixelSize = RasterProductTilespaceFlat.DegPixelSize(level);
    nPixels = (int) ((maxX - minX + (pixelSize/2.0)) / pixelSize);
    nLines  = (int) ((maxY - minY + (pixelSize/2.0)) / pixelSize);
    adfDstGeoTransform[1] = pixelSize;
    adfDstGeoTransform[5] = -pixelSize;
  } else if( dfXRes != 0.0 && dfYRes != 0.0 ) {
    CPLAssert( nPixels == 0 && nLines == 0 );
    if( dfMinX == 0.0 && dfMinY == 0.0 && dfMaxX == 0.0 && dfMaxY == 0.0 )
    {
      dfMinX = adfDstGeoTransform[0];
      dfMaxX = adfDstGeoTransform[0] + adfDstGeoTransform[1] * nPixels;
      dfMaxY = adfDstGeoTransform[3];
      dfMinY = adfDstGeoTransform[3] + adfDstGeoTransform[5] * nLines;
    }

    nPixels = (int) ((dfMaxX - dfMinX + (dfXRes/2.0)) / dfXRes);
    nLines = (int) ((dfMaxY - dfMinY + (dfYRes/2.0)) / dfYRes);
    adfDstGeoTransform[0] = dfMinX;
    adfDstGeoTransform[3] = dfMaxY;
    adfDstGeoTransform[1] = dfXRes;
    adfDstGeoTransform[5] = -dfYRes;
  }

  else if( nForcePixels != 0 && nForceLines != 0 )
  {
    if( dfMinX == 0.0 && dfMinY == 0.0 && dfMaxX == 0.0 && dfMaxY == 0.0 )
    {
      dfMinX = adfDstGeoTransform[0];
      dfMaxX = adfDstGeoTransform[0] + adfDstGeoTransform[1] * nPixels;
      dfMaxY = adfDstGeoTransform[3];
      dfMinY = adfDstGeoTransform[3] + adfDstGeoTransform[5] * nLines;
    }

    dfXRes = (dfMaxX - dfMinX) / nForcePixels;
    dfYRes = (dfMaxY - dfMinY) / nForceLines;

    adfDstGeoTransform[0] = dfMinX;
    adfDstGeoTransform[3] = dfMaxY;
    adfDstGeoTransform[1] = dfXRes;
    adfDstGeoTransform[5] = -dfYRes;

    nPixels = nForcePixels;
    nLines = nForceLines;
  }

  else if( dfMinX != 0.0 || dfMinY != 0.0 || dfMaxX != 0.0 || dfMaxY != 0.0 )
  {
    dfXRes = adfDstGeoTransform[1];
    dfYRes = fabs(adfDstGeoTransform[5]);

    nPixels = (int) ((dfMaxX - dfMinX + (dfXRes/2.0)) / dfXRes);
    nLines = (int) ((dfMaxY - dfMinY + (dfYRes/2.0)) / dfYRes);

    adfDstGeoTransform[0] = dfMinX;
    adfDstGeoTransform[3] = dfMaxY;
  }


  /* -------------------------------------------------------------------- */
  /*      Handle queryOnly                                                */
  /* -------------------------------------------------------------------- */
  if (queryOnly) {
    printf("Result Size: %d,%d\n", nPixels, nLines);

    double *geoxform = adfDstGeoTransform;
    double  beginX = geoxform[0];
    double  endX   = geoxform[0] + geoxform[1]*nPixels
                     + geoxform[2]*nLines;
    double  beginY = geoxform[3];
    double  endY   = geoxform[3] + geoxform[4]*nPixels
                     + geoxform[5]*nLines;

    double North, South, East, West;
    if (geoxform[1] < 0.0) {
      East = beginX;
      West = endX;
    } else {
      East = endX;
      West = beginX;
    }
    if (geoxform[5] < 0.0) {
      North = beginY;
      South = endY;
    } else {
      North = endY;
      South = beginY;
    }
    printf("Result Extents: %.12f,%.12f,%.12f,%.12f\n",
           North, South, East, West);
    exit(0);
  }


  /* -------------------------------------------------------------------- */
  /*      Create the output file.                                         */
  /* -------------------------------------------------------------------- */
  if( !bQuiet )
    printf( "Creating output file that is %dP x %dL.\n", nPixels, nLines );

  hDstDS = GDALCreate( hDriver, pszFilename, nPixels, nLines,
                       GDALGetRasterCount(hSrcDS), eDT,
                       papszCreateOptions );

  if( hDstDS == NULL )
    return NULL;

  /* -------------------------------------------------------------------- */
  /*      Write out the projection definition.                            */
  /* -------------------------------------------------------------------- */
  GDALSetProjection( hDstDS, pszTargetSRS );
  GDALSetGeoTransform( hDstDS, adfDstGeoTransform );

  /* -------------------------------------------------------------------- */
  /*      Copy the color table, if required.                              */
  /* -------------------------------------------------------------------- */
  GDALColorTableH hCT;

  hCT = GDALGetRasterColorTable( GDALGetRasterBand(hSrcDS,1) );
  if( hCT != NULL )
    GDALSetRasterColorTable( GDALGetRasterBand(hDstDS,1), hCT );

  return hDstDS;
}
