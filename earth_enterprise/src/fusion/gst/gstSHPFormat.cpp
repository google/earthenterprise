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
#include <alloca.h>

#include <gstSHPFormat.h>
#include <gstTypes.h>
#include <notify.h>
#include <gstDBFTable.h>
#include <gstFileUtils.h>
#include <khFileUtils.h>
#include <gstRecord.h>

gstSHPFormat::gstSHPFormat(const char* n)
    : gstFormat(n), _table(NULL), _fpSHP(NULL), _shxRecords(NULL) {
  // figure out this machine's endianess
  int i = 1;
  _bigEndian = (*((unsigned char *) &i) == 1) ? false : true;
}

gstSHPFormat::~gstSHPFormat() {
  if (_table)
    _table->unref();
  delete [] _shxRecords;
  closeFile();
}

gstStatus gstSHPFormat::OpenFile() {
  if (!(_fpSHP = fopen(name(), "r")) || !(_hdrSHP = readFileHeader(_fpSHP))) {
    notify(NFY_WARN, "Error opening %s : %s", FormatName(), name());
    return GST_OPEN_FAIL;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Look for the .shx file.  This is a mandatory file
  // so if we can't find it, bail now.
  std::string shxfile = khReplaceExtension( name(), ".shx" );

  // maybe this file came from a PC?
  if (!khExists(shxfile))
    shxfile = khReplaceExtension(name(), ".SHX");

  FILE* fpSHX;
  SHP_FileHeader_i *hdrSHX;
  if ( khExists( shxfile ) && ( fpSHX = fopen( shxfile.c_str(), "r" ) )
       && ( hdrSHX = readFileHeader( fpSHX ) ) ) {

    _numRecords = ((hdrSHX->FileLength * 2) - 100) / sizeof(SHX_RecHeader_i);
    notify(NFY_INFO, "ESRI SHP file has %d records", _numRecords);

    _shxRecords = new SHX_RecHeader_i[_numRecords];

    if ( fread( _shxRecords, sizeof( SHX_RecHeader_i ), _numRecords, fpSHX ) != _numRecords ) {
      notify( NFY_WARN, "Unable to read shx file: %s", shxfile.c_str() );
      return GST_OPEN_FAIL;
    }

    // pre-multiply offset and length
    // they are counts of 16-bit words
    // additionally, shift offset to skip past record header (8 bytes)
    for ( unsigned int r = 0; r < _numRecords; r++ ) {
      SHX_RecHeader_i *irec = &_shxRecords[ r ];
      if ( !_bigEndian ) {
        gstByteSwap( irec->Offset );
        gstByteSwap( irec->RecSize );
      }
      irec->Offset = irec->Offset * 2 + 8;
      irec->RecSize = irec->RecSize * 2;
    }

    fclose( fpSHX );

  } else {
    notify(NFY_WARN, "Unable to open mandatory shx file" );
    return GST_OPEN_FAIL;
  }

  //
  //////////////////////////////////////////////////////////////////////////////


  //////////////////////////////////////////////////////////////////////////////
  // Look for a corresponding .dbf file and parse it.
  // This is done first since the .shp file will have
  // a 1:1 correspondence with the .dbf records if they
  // exist.  And if they do, they should be associated
  // with the new geode built for the .shp file.

  gstFileInfo dbfinfo(name());
  dbfinfo.ext("dbf");

  // maybe this file came from a PC?
  if (dbfinfo.status() != GST_OKAY)
    dbfinfo.ext("DBF");

  if (dbfinfo.status() == GST_OKAY) {
    _table = new gstDBFTable(dbfinfo.name());
    if (_table->open() == GST_OKAY) {
      notify(NFY_DEBUG, "gstDBFTable record has %d fields", _table->numColumns());
    } else {
      notify(NFY_WARN, "Found a dbf file, but unable to open it: %s", dbfinfo.name());
      delete _table;
      _table = NULL;
      return GST_OPEN_FAIL;
    }
  } else {
    notify(NFY_WARN, "Unable to open mandatory dbf file: %s", dbfinfo.name());
    return GST_OPEN_FAIL;
  }
  //
  //////////////////////////////////////////////////////////////////////////////


  //
  // establish a member function pointer that can be used
  // to read in the record contents of a particular shape type
  //
  switch (_hdrSHP->ShapeType) {
    case SHP_Point:
      notify(NFY_DEBUG, "ESRI Shape file contains points.");
      _shapeParseFunc = &gstSHPFormat::parsePoint;
      break;
    case SHP_PolyLine:
      notify(NFY_DEBUG, "ESRI Shape file contains polylines.");
      _shapeParseFunc = &gstSHPFormat::parsePolyLine;
      break;
    case SHP_Polygon:
    case SHP_PolygonZ:
      notify(NFY_DEBUG, "ESRI Shape file contains polygons.");
      _shapeParseFunc = &gstSHPFormat::parsePolygon;
      break;
    default:
      notify(NFY_WARN, "Unsupported shape type: %d", _hdrSHP->ShapeType);
      return GST_OPEN_FAIL;
  };

  addLayer( _numRecords, _table ? _table->getHeader() : gstHeaderImpl::Create() );

#if 0
  // do not use header bbox.  calculate it when needed to pick up any srs
  gstLayerDef &ldef = addLayer( _numRecords, _table ? _table->getHeader() : gstHeaderImpl::Create() );
  gstBBox bbox( _hdrSHP->BBox_Xmin, _hdrSHP->BBox_Xmax,
                _hdrSHP->BBox_Ymin, _hdrSHP->BBox_Ymax );
  vertexTransform( bbox.w, bbox.s );
  vertexTransform( bbox.e, bbox.n );

  ldef.bbox( bbox );
#endif

  return GST_OKAY;
}

gstStatus gstSHPFormat::CloseFile() {
  if (_fpSHP != NULL) {
    fclose(_fpSHP);
    _fpSHP = NULL;
  }

  delete _table;
  _table = NULL;

  return GST_OKAY;
}

gstGeode* gstSHPFormat::GetFeature(std::uint32_t layer, std::uint32_t fidx) {
  if ( layer != 0 || fidx >= _numRecords )
    return NULL;

  // create any geometry from the binary object
  unsigned char buff[_shxRecords[fidx].RecSize];

  if ( fseek( _fpSHP, _shxRecords[fidx].Offset, SEEK_SET ) != 0 ) {
    notify( NFY_WARN, "Unable to load feature %d (seek fail)\n", fidx );
    return NULL;
  }

  //SHP_RecHeader_i *recHdr = readRecHeader(_fpSHP);

  int bytesRead = fread( buff, 1, _shxRecords[fidx].RecSize, _fpSHP );
  if ( bytesRead != _shxRecords[fidx].RecSize ) {
    notify( NFY_WARN, "Unable to load feature %d (read fail, %d/%d)\n",
            fidx, bytesRead, _shxRecords[fidx].RecSize );
    return NULL;
  }

  //
  // validate type
  // first 4 bytes defines the type
  std::int32_t type = *( (std::int32_t*)buff );
  if ( _bigEndian )
    gstByteSwap( type );

  // skip NULL shapes
  if ( type == SHP_Null_Shape )
    return NULL;

  static int maxmsg = 10;
  if ( type != _hdrSHP->ShapeType ) {
    if ( maxmsg == 1 ) {
      notify( NFY_WARN, "Mismatch shape type!  File is %d, record is %d, skipping.  (suppressing further messages)",
              _hdrSHP->ShapeType, type );
      --maxmsg;
    } else if ( maxmsg ) {
      notify( NFY_WARN, "Mismatch shape type!  File is %d, record is %d, skipping.",
              _hdrSHP->ShapeType, type );
      --maxmsg;
    }
    return NULL;
  }

  gstGeode *geode = ( this->*_shapeParseFunc )( buff + 4 );

  return geode;
}

gstRecordHandle gstSHPFormat::GetAttribute(std::uint32_t layer, std::uint32_t id) {
  if (!_table) {
    return gstRecordHandle();
  } else {
    return _table->Row(id);
  }
}

//////////////////////////////////////////////////////////////////////
//
// Point
//
//////////////////////////////////////////////////////////////////////

gstGeode *gstSHPFormat::parsePoint(unsigned char *buff)
{
  SHP_Point_Rec *newPoint = (SHP_Point_Rec*)buff;

  //memcpy((void*)&newPoint, buff, sizeof(SHP_Point_Rec));

  if ( _bigEndian ) {
    gstByteSwap(newPoint->x);
    gstByteSwap(newPoint->y);
  }

  gstGeode *geode = new gstGeode(gstPoint);

#if 0
  double x = newPoint->x;
  double y = newPoint->y;

  vertexTransform(x, y);

  geode->addVertex(x, y);
#else
  geode->addVertex( gstVertex(newPoint->x, newPoint->y) );

#endif

  return geode;
}

//////////////////////////////////////////////////////////////////////
//
// PolyLine
//
//////////////////////////////////////////////////////////////////////

gstGeode *gstSHPFormat::parsePolyLine(unsigned char *buff)
{
  SHP_PolyLine_Rec newPolyLine;

  // this doesn't handle the pointers properly
  memcpy((void*)&newPolyLine, buff, sizeof(SHP_PolyLine_Rec));

  if ( _bigEndian ) {
    gstByteSwap(newPolyLine.Box[0]);
    gstByteSwap(newPolyLine.Box[1]);
    gstByteSwap(newPolyLine.Box[2]);
    gstByteSwap(newPolyLine.Box[3]);
    gstByteSwap(newPolyLine.NumParts);
    gstByteSwap(newPolyLine.NumPoints);
  }

  // now fix the pointers
  newPolyLine.Parts = (std::int32_t*)(buff + 40);
  newPolyLine.Points = (SHP_Point_Rec *)(buff + 40 +
                                         (newPolyLine.NumParts * sizeof(std::int32_t*)));

  if ( _bigEndian ) {
    int ii;
    for (ii = 0; ii < newPolyLine.NumParts; ii++)
      gstByteSwap(newPolyLine.Parts[ii]);

    for (ii = 0; ii < newPolyLine.NumPoints; ii++) {
      gstByteSwap(newPolyLine.Points[ii].x);
      gstByteSwap(newPolyLine.Points[ii].y);
    }
  }

  gstGeode *geode = new gstGeode(gstPolyLine);

  int pointsUsed = 0;
  for (int ii = 1; ii <= newPolyLine.NumParts; ii++) {

    if (ii != 1)
      geode->addSubPart();

    while (pointsUsed < newPolyLine.NumPoints &&
           (ii == newPolyLine.NumParts || pointsUsed < newPolyLine.Parts[ii])) {
      double x = newPolyLine.Points[pointsUsed].x;
      double y = newPolyLine.Points[pointsUsed].y;
      //vertexTransform(x, y);
      geode->appendVertex( gstVertex(x, y) );
      pointsUsed++;
    }
  }

  return geode;

}   // End gstSHPFormat::parsePolyLine()


//////////////////////////////////////////////////////////////////////
//
// Polygon
//
//////////////////////////////////////////////////////////////////////

gstGeode *gstSHPFormat::parsePolygon(unsigned char *buff)
{
  SHP_Polygon_Rec newPoly;

  // this doesn't handle the pointers properly
  memcpy((void*)&newPoly, buff, sizeof(SHP_Polygon_Rec));

  if ( _bigEndian ) {
    gstByteSwap(newPoly.Box[0]);
    gstByteSwap(newPoly.Box[1]);
    gstByteSwap(newPoly.Box[2]);
    gstByteSwap(newPoly.Box[3]);
    gstByteSwap(newPoly.NumParts);
    gstByteSwap(newPoly.NumPoints);
  }

  // now fix the pointers
  newPoly.Parts = (std::int32_t*)(buff + 40);
  newPoly.Points = (SHP_Point_Rec *)(buff + 40 + (newPoly.NumParts * sizeof(std::int32_t*)));

  if ( _bigEndian ) {
    int ii;
    for (ii = 0; ii < newPoly.NumParts; ii++)
      gstByteSwap(newPoly.Parts[ii]);

    for (ii = 0; ii < newPoly.NumPoints; ii++) {
      gstByteSwap(newPoly.Points[ii].x);
      gstByteSwap(newPoly.Points[ii].y);
    }
  }

  gstGeode *geode = new gstGeode(gstPolygon);
  int pointsUsed = 0;
  for (int ii = 1; ii <= newPoly.NumParts; ii++) {

    if (ii != 1)
      geode->addSubPart();

    while (pointsUsed < newPoly.NumPoints &&
           (ii == newPoly.NumParts || pointsUsed < newPoly.Parts[ii])) {
      double x = newPoly.Points[pointsUsed].x;
      double y = newPoly.Points[pointsUsed].y;
      //vertexTransform(x, y);
      geode->appendVertex( gstVertex(x, y) );
      pointsUsed++;
    }
  }

  return geode;

}   // End gstSHPFormat::parsePolygon()

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

SHP_FileHeader_i *gstSHPFormat::readFileHeader(FILE *fp)
{
  unsigned char buff[sizeof(SHP_FileHeader_i)];

  if (fread(&buff, 100, 1, fp) != 1) {
    notify(NFY_WARN, "Unable to read file header for %s", FormatName());
    return NULL;
  }

  SHP_FileHeader_i *irec = new SHP_FileHeader_i;

  // first copy the ints
  memcpy((void*)irec, buff, sizeof(unsigned int) * 9);
  // then copy the doubles
  memcpy((void*)&irec->BBox_Xmin, buff + sizeof(unsigned int) * 9, sizeof(double) * 8);

  // for big endian machines, swap these
  if ( _bigEndian ) {
    gstByteSwap(irec->Version);
    gstByteSwap(irec->ShapeType);
    gstByteSwap(irec->BBox_Xmin);
    gstByteSwap(irec->BBox_Ymin);
    gstByteSwap(irec->BBox_Xmax);
    gstByteSwap(irec->BBox_Ymax);
    gstByteSwap(irec->BBox_Zmin);
    gstByteSwap(irec->BBox_Zmax);
    gstByteSwap(irec->BBox_Mmin);
    gstByteSwap(irec->BBox_Mmax);
  } else {
    gstByteSwap(irec->FileCode);
    gstByteSwap(irec->FileLength);
  }

  notify( NFY_DEBUG, "File code: %d", irec->FileCode );
  notify( NFY_DEBUG, "File Length: %d", irec->FileLength );
  notify( NFY_DEBUG, "Version=%d", irec->Version );
  notify( NFY_DEBUG, "Shape Type=%d", irec->ShapeType );

  return irec;
}


SHP_RecHeader_i *gstSHPFormat::readRecHeader(FILE *fp)
{
  static unsigned char buff[sizeof(SHP_RecHeader_i)];
  static SHP_RecHeader_i irec;

  if (fread(&buff, 8, 1, fp) != 1)
    notify(NFY_WARN, "Unable to read record header.");

  memcpy((void*)&irec, buff, sizeof(SHP_RecHeader_i));

  if ( !_bigEndian ) {
    gstByteSwap(irec.RecordNumber);
    gstByteSwap(irec.ContentLength);
  }

  return &irec;
}

