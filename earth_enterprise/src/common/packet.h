/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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


// Definitions for Google Earth Fusion data packet formats.  Quadtree
// packet definitions can be found in common/qtpacket/quadtreepacket.h

#ifndef COMMON_PACKET_H__
#define COMMON_PACKET_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <khEndian.h>

#define KEYHOLEMAGICID 32301
#define DATABUFFERSIZE 5000
#define DEFAULT_LEVELSIZE 4


#define TYPE_STREETPACKET 2
#define TYPE_SITEPACKET 3
#define TYPE_DRAWABLEPACKET 4
#define TYPE_POLYLINEPACKET 5
#define TYPE_AREAPACKET 6
#define TYPE_STREETPACKET_UTF8 7
#define TYPE_SITEPACKET_UTF8 8
#define TYPE_LANDMARK 9
#define TYPE_POLYGONPACKET 10

#define RFTYPE_NONE 0
#define RFTYPE_URL 1

#define KEYHOLE_VERSION 1

#define DIRECTORY_DIVISION 5
#define NUMBER_OF_DIVISIONS 3

//#define VECTOR_COMPRESSION

#define NEWSITESTUFF

#define FLAT_MAX_SIZE 30000000

#if _LP64
#define FORCE_BASE32
#endif

enum {
  ET_PACKET_MERGE = 1,
  ET_PACKET_REPLACE = 2
};

char * generateFileName(int level);

#ifdef FORCE_BASE32
typedef std::uint32_t id_size;
#else
typedef unsigned long id_size;
#endif

// Packet header.  TODO: replace this definition with the
// updated one in quadtreepacket.h, update all code in packet.h to use
// khEndian.h buffer definitions.

struct etDataHeader
{
  id_size magicID;
  id_size dataTypeID;
  id_size version;
  id_size numInstances;
  id_size dataInstanceSize;
  id_size dataBufferOffset;
  id_size dataBufferSize;
  id_size metaBufferSize;

  inline void byteSwap()
  {
    swapb(magicID);
    swapb(dataTypeID);
    swapb(version);
    swapb(numInstances);
    swapb(dataInstanceSize);
    swapb(dataBufferOffset);
    swapb(dataBufferSize);
    swapb(metaBufferSize);
  };
  etDataHeader() {}
 private:
  DISALLOW_COPY_AND_ASSIGN(etDataHeader);
};

//*******************Type definitions from the Client***********************//

struct etPattern
{
  int signature;
  unsigned short length;
  unsigned short width;

  static int hash(const char* pattern);

  inline void byteSwap()
  {
    swapb(signature);
    swapb(length);
    swapb(width);
  };
  explicit etPattern(const char * from) :
      signature(etPattern::hash(from)), length(strlen(from)), width(0) {}
 private:
  DISALLOW_COPY_AND_ASSIGN(etPattern);
};

struct etString
{
  char * string;

  etPattern* getPattern() { return (etPattern*)(string - sizeof(etPattern)); }
  int        getLength()  { return getPattern()->length; }

  void set(const char * from)
  {
    etPattern header(from);
    char * tmp = (char *)malloc(header.length+1+sizeof(etPattern));
    memcpy(tmp, &header, sizeof(etPattern));
    string = tmp + sizeof(etPattern);
    strcpy(string, from);
  };

  inline void byteSwap()
  {
    getPattern()->byteSwap();
  };

 public:
  etString():string(NULL) {}
 private:
  DISALLOW_COPY_AND_ASSIGN(etString);
};

struct etVec3us
{
  unsigned short elem[3];

  inline void byteSwap()
  {
    swapb(elem[0]);
    swapb(elem[1]);
    swapb(elem[2]);
  };
 private:
  DISALLOW_COPY_AND_ASSIGN(etVec3us);
};

struct etVec3d
{
  double elem[3];

  inline void byteSwap()
  {
    swapb(elem[0]);
    swapb(elem[1]);
    swapb(elem[2]);
  };
 private:
  DISALLOW_COPY_AND_ASSIGN(etVec3d);
};

//*********************************************************************//

#ifdef FORCE_BASE32
struct etDataPacket_BASE32
{
  etDataHeader packetHeader;
    
  std::uint32_t metaHeader;
    
  std::uint32_t packetBuffer;
  int packetBufferSize;
  
  std::uint32_t dataInstances;
    
  std::uint32_t dataBuffer;
  int maxDataBufferSize;

  //Flat File Concurrent Save
  static char flatbasename[500];
  static char flatdirname[500];
  static char flatdataname[500];
  static std::uint32_t flatdirfp;
  static std::uint32_t flatdatafp;
  static long datafilesize;
  static int flatmode;
 private:
  DISALLOW_COPY_AND_ASSIGN(etDataPacket_BASE32);
};
#endif


//
// This is an abstract base class
//
// All uses should be through the derived classes:
//    etStreetPacket            type = TYPE_STREETPACKET or TYPE_STREETPACKET_UTF8
//    etPolyLinePacket          type = TYPE_POLYLINEPACKET
//    etAreaPacket              type = TYPE_AREAPACKET
//    etSitePacket              type = TYPE_SITEPACKET or TYPE_SITEPACKET_UTF8
//    etLandmarkPacket          type = TYPE_LANDMARK
//    etPolygonPacket           type = TYPE_POLYGONPACKET
//    etDrawablePacket          type = TYPE_DRAWABLEPACKET
//
// The type is stored in packetHeader.dataTypeID

struct etDataPacket
{
  etDataHeader packetHeader;

#ifdef FORCE_BASE32
  std::uint32_t metaHeader_OFFSET;
#else       
  void * metaHeader;
#endif

#ifdef FORCE_BASE32
  std::uint32_t packetBuffer_OFFSET;
#else       
  char* packetBuffer;
#endif
  int packetBufferSize;

#ifdef FORCE_BASE32
  std::uint32_t dataInstances_OFFSET;
  std::uint32_t dataBuffer_OFFSET;
#else       
  char * dataInstances;
  char * dataBuffer;
#endif
  int maxDataBufferSize;

  //Get Data Instances
  void   getDataInstance(int index, void * dst)
  { memcpy(dst, &dataInstances[index*packetHeader.dataInstanceSize], packetHeader.dataInstanceSize); }
  void * getDataInstanceP(int index)
  { return &dataInstances[index*packetHeader.dataInstanceSize]; }

  //Set Data Instances
  void setDataInstance(int index, void * src)
  { memcpy(&dataInstances[index*packetHeader.dataInstanceSize], src, packetHeader.dataInstanceSize); }

  //Add Data to Data Buffer
  unsigned int addDataToDataBuffer(void * buffer, int buffersize);

  //Load and Initialize Data Packet
  int load(char * filename);
  int load(char* buffer, int buffersize);

  //Save Data Packet
  int save(char * filename);
  int save(void * buffer, int buffersize);

  //Calculate Size of Data Packet
  int getsavesize()
  { return sizeof(etDataHeader) +
      packetHeader.dataInstanceSize*packetHeader.numInstances +
      packetHeader.dataBufferSize; }

  //Flat File Concurrent Save
  static char flatbasename[500];
  static char flatdirname[500];
  static char flatdataname[500];
#ifdef FORCE_BASE32
  std::uint32_t flatdirfp_OFFSET;
  std::uint32_t flatdatafp_OFFSET;
#else       
  static FILE * flatdirfp;
  static FILE * flatdatafp;
#endif
  static long datafilesize;
  static int flatmode;

  int getFlatFileCount();
  int setFlatFileCount(int num);

  int openFlat(char * flatfile);
  int saveFlat(char * fname, char* buffer, int buffersize);
  int closeFlat();

  //Initialize Data Packet
  void initHeader(int dataTypeID, int version, int numInstances, int dataInstanceSize,
                  void * metastruct = NULL);

  //Clear Data Packet
  void clear()
  {
    if(packetBuffer)
    {
      if(metaHeader)    { metaHeader = NULL; }
      if(dataInstances) { dataInstances = NULL; }
      if(dataBuffer)    { dataBuffer = NULL; }

      if (packetBuffer != (char*)1)
        free(packetBuffer);
      packetBuffer = NULL;
    }
    else
    {
      if(metaHeader)    { metaHeader = NULL; }
      if(dataInstances) { free(dataInstances); dataInstances = NULL; }
      if(dataBuffer)    { free(dataBuffer); dataBuffer = NULL; }
    }
#ifdef FORCE_BASE32
    dataInstanceSize_BASE32 = 0;
#endif
  }

  //Constructor
  etDataPacket()
  {
    metaHeader = NULL;
    packetBuffer = NULL;
    dataInstances = NULL;
    dataBuffer = NULL;
#ifdef FORCE_BASE32
    dataInstanceSize_BASE32 = 0;
#endif
  }

  ~etDataPacket()
  { clear(); }

  inline void byteSwap()
  {
    packetHeader.byteSwap();
    swapb(packetBufferSize);
    swapb(maxDataBufferSize);
  };

#ifdef FORCE_BASE32
  int dataInstanceSize_BASE32;

  void * metaHeader;
  char* packetBuffer;
  char * dataInstances;
  char * dataBuffer;
  static FILE * flatdirfp;
  static FILE * flatdatafp;

  void movetooffset()
  {
    dataInstances_OFFSET = (std::uint64_t)dataInstances;
    dataBuffer_OFFSET = (std::uint64_t)dataBuffer;
  };

  void movefromoffset()
  {
    std::uint64_t num;
    num = dataInstances_OFFSET;
    dataInstances = (char *)num;
    num = dataBuffer_OFFSET;
    dataBuffer = (char *)num;
  };
  void* padding_;  // TODO: why this padding fixes the memory issue?
#endif
 private:
  DISALLOW_COPY_AND_ASSIGN(etDataPacket);
};

struct etDataCypher
{
  int fromOffset;
  int toOffset;
  int size;

  int pointerFlag;
 private:
  DISALLOW_COPY_AND_ASSIGN(etDataCypher);
};

struct etDataTranslator
{
  etDataCypher * translationTable;
  int translationTableSize;

  void allocTranslationTable(int num);

  void setTranslationTable(int num, int fromOffset, int toOffset, int size, int pointerFlag);

  void translate(void * fromptr, void * toptr);
  void translateBack(void * fromptr, void * toptr);

  void translate(void * fromptr, void * toptr, unsigned int pointerOffset);

  //Destructor
  etDataTranslator() : translationTable(NULL) {}
  ~etDataTranslator() { if(translationTable) free(translationTable); };
 private:
  DISALLOW_COPY_AND_ASSIGN(etDataTranslator);
};

struct etPointerCypher
{
  int offset;
  int (* get)(void *);

  etPointerCypher() { get = NULL; }
 private:
  DISALLOW_COPY_AND_ASSIGN(etPointerCypher);
};

struct etPointerTranslator
{
  etPointerCypher * pointerList;
  int pointerListSize;

  void allocPointerList(int num);

  void setPointerList(int num, int offset, int (* get)(void *) = NULL)
  { pointerList[num].offset = offset; pointerList[num].get = get; }

  void translatePointerToOffset(etDataPacket * packet, int cleanup=0);
  void translateOffsetToPointer(etDataPacket * packet);

  void translateStringToOffset(etDataPacket * packet, int cleanup=1);
  void translateOffsetToString(etDataPacket * packet);

  //Destructor
  etPointerTranslator() : pointerList(NULL) {}
  ~etPointerTranslator() { if(pointerList) free(pointerList); };
 private:
  DISALLOW_COPY_AND_ASSIGN(etPointerTranslator);
};

//***********DATA PACKET GENERATION******************//

#ifdef FORCE_BASE32
struct etStreetPacketData_BASE32
{
  std::uint32_t           name;
  int          texId_deprecated;
  ushort       numPt;
  ushort       bitFlags;
#ifdef VECTOR_COMPRESSION
  std::uint32_t localPt;
#else
  std::uint32_t localPt;
#endif

  int          style;
 private:
  DISALLOW_COPY_AND_ASSIGN(etStreetPacketData_BASE32);
};
#endif

struct etStreetPacketData
{
#ifdef FORCE_BASE32
  std::uint32_t           name_OFFSET;
#else
  etString     name;
#endif
  int          texId_deprecated;
  ushort       numPt;
  ushort       bitFlags;
#ifdef FORCE_BASE32
#ifdef VECTOR_COMPRESSION
  std::uint32_t localPt_OFFSET;
#else
  std::uint32_t localPt_OFFSET;
#endif
#else
#ifdef VECTOR_COMPRESSION
  etVec3us    *localPt;
#else
  etVec3d     *localPt;
#endif
#endif
  int          style;

#ifdef FORCE_BASE32
  etString     name;
#ifdef VECTOR_COMPRESSION
  etVec3us    *localPt;
#else
  etVec3d     *localPt;
#endif
#endif

  inline void byteSwap()
  {
#ifdef khEndianDoSwap
    for(int i=0; i < numPt; i++)
      localPt[i].byteSwap();

    name.byteSwap();
    swapb(texId_deprecated);
    swapb(numPt);
    swapb(bitFlags);
    swapb(style);
#endif
  };

#ifdef FORCE_BASE32
  void movetooffset()
  {
    localPt_OFFSET = (std::uint64_t)localPt;
    name_OFFSET = (std::uint64_t)name.string;
  };

  void movefromoffset()
  {
    std::uint64_t num;
    num = localPt_OFFSET;
#ifdef VECTOR_COMPRESSION
    localPt = (etVec3us *)num;
#else
    localPt = (etVec3d *)num;
#endif
    num = name_OFFSET;
    name.string = (char *)num;
  };
#endif
 private:
  DISALLOW_COPY_AND_ASSIGN(etStreetPacketData);
};

struct etStreetPacket : public etDataPacket
{
  static etPointerTranslator * streetPointerTranslator;
  static etPointerTranslator * streetStringTranslator;

  void initUTF8(int num)
  {
    initHeader(TYPE_STREETPACKET_UTF8/*datatype*/, KEYHOLE_VERSION/*version*/, num,
               sizeof(etStreetPacketData));
#ifdef FORCE_BASE32
    dataInstanceSize_BASE32 = sizeof(etStreetPacketData_BASE32);
#endif
  };

  etStreetPacketData * getPtr(int num)
  {
    return (etStreetPacketData *)getDataInstanceP(num);
  };

  void pointerToOffset()
  {
    if(streetPointerTranslator == NULL)
    {
      streetPointerTranslator = createStreetPointerTranslator();
      streetStringTranslator = createStreetStringTranslator();
    }
    byteSwap();
    streetPointerTranslator->translatePointerToOffset(this);
    streetStringTranslator->translateStringToOffset(this);
#ifdef FORCE_BASE32
    movetooffset();
#endif
  };

  void offsetToPointer()
  {
#ifdef FORCE_BASE32
    if(packetBuffer == (char *)1)
    {
      dataInstanceSize_BASE32 = sizeof(etStreetPacketData_BASE32);
      packetHeader.dataInstanceSize = sizeof(etStreetPacketData);
      char * newpacketBuffer = (char *)malloc(packetHeader.dataInstanceSize*packetHeader.numInstances);
      for(int i=0; i < (int)packetHeader.numInstances; i++)
        memcpy(&newpacketBuffer[i*packetHeader.dataInstanceSize], &dataInstances[i*dataInstanceSize_BASE32],
               dataInstanceSize_BASE32);
      packetBuffer = newpacketBuffer;
      dataInstances = newpacketBuffer;
    }
#endif
    if(streetPointerTranslator == NULL)
    {
      streetPointerTranslator = createStreetPointerTranslator();
      streetStringTranslator = createStreetStringTranslator();
    }
#ifdef FORCE_BASE32
    movefromoffset();
#endif
    streetPointerTranslator->translateOffsetToPointer(this);
    streetStringTranslator->translateOffsetToString(this);
    byteSwap();
  };


  etPointerTranslator * createStreetPointerTranslator();
  etPointerTranslator * createStreetStringTranslator();

  inline void byteSwap()
  {
    for(id_size i=0; i < packetHeader.numInstances; i++)
      getPtr(i)->byteSwap();
  };
#ifdef FORCE_BASE32
  void movetooffset()
  {
    for(id_size i=0; i < packetHeader.numInstances; i++)
      getPtr(i)->movetooffset();
  };

  void movefromoffset()
  {
    for(id_size i=0; i < packetHeader.numInstances; i++)
      getPtr(i)->movefromoffset();
  };
#endif

 public:
  etStreetPacket() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(etStreetPacket);
};

#ifdef FORCE_BASE32
struct etPolyLinePacketData_BASE32
{
  std::uint32_t           name;
  int          texId;
  ushort       numPt;
  ushort       bitFlags;

#ifdef VECTOR_COMPRESSION
  std::uint32_t localPt;
#else
  std::uint32_t localPt;
#endif

  int          style;
 private:
  DISALLOW_COPY_AND_ASSIGN(etPolyLinePacketData_BASE32);
};
#endif

struct etPolyLinePacketData
{
#ifdef FORCE_BASE32
  std::uint32_t           name_OFFSET;
#else
  etString     name;
#endif
  int          texId;
  ushort       numPt;
  ushort       bitFlags;

#ifdef FORCE_BASE32
#ifdef VECTOR_COMPRESSION
  std::uint32_t localPt_OFFSET;
#else
  std::uint32_t localPt_OFFSET;
#endif
#else
#ifdef VECTOR_COMPRESSION
  etVec3us    *localPt;
#else
  etVec3d     *localPt;
#endif
#endif

  int          style;

#ifdef FORCE_BASE32
  etString     name;
#ifdef VECTOR_COMPRESSION
  etVec3us    *localPt;
#else
  etVec3d     *localPt;
#endif
#endif

  inline void byteSwap()
  {
    for(int i=0; i < numPt; i++)
      localPt[i].byteSwap();

    name.byteSwap();
    swapb(texId);
    swapb(numPt);
    swapb(bitFlags);
    swapb(style);
  };

#ifdef FORCE_BASE32
  void movetooffset()
  {
    localPt_OFFSET = (std::uint64_t)localPt;
    name_OFFSET = (std::uint64_t)name.string;
  };

  void movefromoffset()
  {
    std::uint64_t num;
    num = localPt_OFFSET;
#ifdef VECTOR_COMPRESSION
    localPt = (etVec3us *)num;
#else
    localPt = (etVec3d *)num;
#endif
    num = name_OFFSET;
    name.string = (char *)num;
  };
#endif
 private:
  DISALLOW_COPY_AND_ASSIGN(etPolyLinePacketData);
};

struct etPolyLinePacket : public etDataPacket
{
  static etPointerTranslator * polyLinePointerTranslator;
  static etPointerTranslator * polyLineStringTranslator;

  void init(int num)
  {
    initHeader(TYPE_POLYLINEPACKET/*datatype*/, KEYHOLE_VERSION/*version*/, num,
               sizeof(etPolyLinePacketData));
#ifdef FORCE_BASE32
    dataInstanceSize_BASE32 = sizeof(etPolyLinePacketData_BASE32);
#endif
  };

  etPolyLinePacketData * getPtr(int num)
  {
    return (etPolyLinePacketData *)getDataInstanceP(num);
  };

  void pointerToOffset()
  {
    if(polyLinePointerTranslator == NULL)
    {
      polyLinePointerTranslator = createPolyLinePointerTranslator();
      polyLineStringTranslator = createPolyLineStringTranslator();
    }

    byteSwap();
    polyLinePointerTranslator->translatePointerToOffset(this);
    polyLineStringTranslator->translateStringToOffset(this);
#ifdef FORCE_BASE32
    movetooffset();
#endif
  };

  void offsetToPointer()
  {
#ifdef FORCE_BASE32
    if(packetBuffer == (char *)1)
    {
      dataInstanceSize_BASE32 = sizeof(etPolyLinePacketData_BASE32);
      packetHeader.dataInstanceSize = sizeof(etPolyLinePacketData);
      char * newpacketBuffer = (char *)malloc(packetHeader.dataInstanceSize*packetHeader.numInstances);
      for(int i=0; i < (int)packetHeader.numInstances; i++)
        memcpy(&newpacketBuffer[i*packetHeader.dataInstanceSize], &dataInstances[i*dataInstanceSize_BASE32],
               dataInstanceSize_BASE32);
      packetBuffer = newpacketBuffer;
      dataInstances = newpacketBuffer;
    }
#endif
    if(polyLinePointerTranslator == NULL)
    {
      polyLinePointerTranslator = createPolyLinePointerTranslator();
      polyLineStringTranslator = createPolyLineStringTranslator();
    }
#ifdef FORCE_BASE32
    movefromoffset();
#endif
    polyLinePointerTranslator->translateOffsetToPointer(this);
    polyLineStringTranslator->translateOffsetToString(this);
    byteSwap();
  };

  etPointerTranslator * createPolyLinePointerTranslator();
  etPointerTranslator * createPolyLineStringTranslator();

  inline void byteSwap()
  {
    for(id_size i=0; i < packetHeader.numInstances; i++)
      getPtr(i)->byteSwap();
  };
#ifdef FORCE_BASE32
  void movetooffset()
  {
    for(id_size i=0; i < packetHeader.numInstances; i++)
      getPtr(i)->movetooffset();
  };

  void movefromoffset()
  {
    for(id_size i=0; i < packetHeader.numInstances; i++)
      getPtr(i)->movefromoffset();
  };
#endif
  etPolyLinePacket() {}
 private:
  DISALLOW_COPY_AND_ASSIGN(etPolyLinePacket);
};

#ifdef FORCE_BASE32
struct etAreaPacketData_BASE32 {
  std::uint32_t           name;
  int          texId;
  ushort       numPt;
  ushort       bitFlags;

#ifdef VECTOR_COMPRESSION
  std::uint32_t localPt;
#else
  std::uint32_t localPt;
#endif

  int          style;
 private:
  DISALLOW_COPY_AND_ASSIGN(etAreaPacketData_BASE32);
};
#endif

struct etAreaPacketData {
#ifdef FORCE_BASE32
  std::uint32_t           name_OFFSET;
#else
  etString     name;
#endif
  int          texId;
  ushort       numPt;
  ushort       bitFlags;

#ifdef FORCE_BASE32
#ifdef VECTOR_COMPRESSION
  std::uint32_t localPt_OFFSET;
#else
  std::uint32_t localPt_OFFSET;
#endif
#else
#ifdef VECTOR_COMPRESSION
  etVec3us    *localPt;
#else
  etVec3d     *localPt;
#endif
#endif

  int          style;

#ifdef FORCE_BASE32
  etString     name;
#ifdef VECTOR_COMPRESSION
  etVec3us    *localPt;
#else
  etVec3d     *localPt;
#endif
#endif

  inline void byteSwap() {
    for (int i = 0; i < numPt; i++) {
      localPt[i].byteSwap();
    }

    name.byteSwap();
    swapb(texId);
    swapb(numPt);
    swapb(bitFlags);
    swapb(style);
  };

#ifdef FORCE_BASE32
  void movetooffset() {
    localPt_OFFSET = reinterpret_cast<std::uint64_t>(localPt);
    name_OFFSET = reinterpret_cast<std::uint64_t>(name.string);
  };

  void movefromoffset() {
    std::uint64_t num;
    num = localPt_OFFSET;
#ifdef VECTOR_COMPRESSION
    localPt = reinterpret_cast<etVec3us *>(num);
#else
    localPt = reinterpret_cast<etVec3d *>(num);
#endif
    num = name_OFFSET;
    name.string = reinterpret_cast<char *>(num);
  };
#endif
 private:
  DISALLOW_COPY_AND_ASSIGN(etAreaPacketData);
};

struct etAreaPacket : public etDataPacket {
  static etPointerTranslator * areaPointerTranslator;
  static etPointerTranslator * areaStringTranslator;

  void init(int num) {
    initHeader(TYPE_AREAPACKET/*datatype*/, KEYHOLE_VERSION/*version*/, num,
               sizeof(etAreaPacketData));
#ifdef FORCE_BASE32
    dataInstanceSize_BASE32 = sizeof(etAreaPacketData_BASE32);
#endif
  };

  etAreaPacketData * getPtr(int num) {
    return reinterpret_cast<etAreaPacketData *>(getDataInstanceP(num));
  };

  void pointerToOffset() {
    if (areaPointerTranslator == NULL) {
      areaPointerTranslator = createAreaPointerTranslator();
      areaStringTranslator = createAreaStringTranslator();
    }

    byteSwap();
    areaPointerTranslator->translatePointerToOffset(this);
    areaStringTranslator->translateStringToOffset(this);
#ifdef FORCE_BASE32
    movetooffset();
#endif
  };

  void offsetToPointer() {
#ifdef FORCE_BASE32
    if (packetBuffer == reinterpret_cast<char *>(1)) {
      dataInstanceSize_BASE32 = sizeof(etAreaPacketData_BASE32);
      packetHeader.dataInstanceSize = sizeof(etAreaPacketData);
      char * newpacketBuffer = reinterpret_cast<char *>(malloc(
          packetHeader.dataInstanceSize*packetHeader.numInstances));
      for (int i = 0; i < static_cast<int>(packetHeader.numInstances); i++)
        memcpy(&newpacketBuffer[i*packetHeader.dataInstanceSize],
               &dataInstances[i*dataInstanceSize_BASE32],
               dataInstanceSize_BASE32);
      packetBuffer = newpacketBuffer;
      dataInstances = newpacketBuffer;
    }
#endif
    if (areaPointerTranslator == NULL) {
      areaPointerTranslator = createAreaPointerTranslator();
      areaStringTranslator = createAreaStringTranslator();
    }
#ifdef FORCE_BASE32
    movefromoffset();
#endif
    areaPointerTranslator->translateOffsetToPointer(this);
    areaStringTranslator->translateOffsetToString(this);
    byteSwap();
  };


  etPointerTranslator * createAreaPointerTranslator();
  etPointerTranslator * createAreaStringTranslator();

  inline void byteSwap() {
    for (id_size i = 0; i < packetHeader.numInstances; i++) {
      getPtr(i)->byteSwap();
    }
  };
#ifdef FORCE_BASE32
  void movetooffset() {
    for (id_size i = 0; i < packetHeader.numInstances; i++) {
      getPtr(i)->movetooffset();
    }
  };

  void movefromoffset() {
    for (id_size i = 0; i < packetHeader.numInstances; i++) {
      getPtr(i)->movefromoffset();
    }
  };
#endif
 private:
  DISALLOW_COPY_AND_ASSIGN(etAreaPacket);
};

#ifdef FORCE_BASE32
struct etSitePacketData_BASE32 {
  std::uint32_t           name;
  int          texId;
  ushort       numPt;
  ushort       bitFlags;

#ifdef VECTOR_COMPRESSION
  std::uint32_t localPt;
#else
  std::uint32_t localPt;
#endif

  std::uint32_t html;

  int          style;

#ifdef NEWSITESTUFF
  std::uint32_t address;
  std::uint32_t phone;
#endif
 private:
  DISALLOW_COPY_AND_ASSIGN(etSitePacketData_BASE32);
};
#endif

struct etSitePacketData {
#ifdef FORCE_BASE32
  std::uint32_t           name_OFFSET;
#else
  etString     name;
#endif
  int          texId;
  ushort       numPt;
  ushort       bitFlags;

#ifdef FORCE_BASE32
#ifdef VECTOR_COMPRESSION
  std::uint32_t localPt_OFFSET;
#else
  std::uint32_t localPt_OFFSET;
#endif
  std::uint32_t      html_OFFSET;
#else
#ifdef VECTOR_COMPRESSION
  etVec3us    *localPt;
#else
  etVec3d     *localPt;
#endif
  char *      html;
#endif


  int          style;

#ifdef FORCE_BASE32
#ifdef NEWSITESTUFF
  std::uint32_t address_OFFSET;
  std::uint32_t phone_OFFSET;
#endif
#else
#ifdef NEWSITESTUFF
  char * address;
  char * phone;
#endif
#endif

#ifdef FORCE_BASE32
  etString     name;
#ifdef VECTOR_COMPRESSION
  etVec3us    *localPt;
#else
  etVec3d     *localPt;
#endif
  char *      html;
  char * address;
  char * phone;
#endif

  inline void byteSwap() {
    for (int i = 0; i < numPt; i++) {
      localPt[i].byteSwap();
    }

    name.byteSwap();
    swapb(texId);
    swapb(numPt);
    swapb(bitFlags);
    swapb(style);
  };

#ifdef FORCE_BASE32
  void movetooffset() {
    name_OFFSET = reinterpret_cast<std::uint64_t>(name.string);
    localPt_OFFSET = reinterpret_cast<std::uint64_t>(localPt);
    html_OFFSET = reinterpret_cast<std::uint64_t>(html);
    address_OFFSET = reinterpret_cast<std::uint64_t>(address);
    phone_OFFSET = reinterpret_cast<std::uint64_t>(phone);
  };

  void movefromoffset() {
    std::uint64_t num;
    num = localPt_OFFSET;
#ifdef VECTOR_COMPRESSION
    localPt = reinterpret_cast<etVec3us *>(num);
#else
    localPt = reinterpret_cast<etVec3d *>(num);
#endif
    num = name_OFFSET;
    name.string = reinterpret_cast<char *>(num);
    num = html_OFFSET;
    html = reinterpret_cast<char *>(num);
    num = address_OFFSET;
    address = reinterpret_cast<char *>(num);
    num = phone_OFFSET;
    phone = reinterpret_cast<char *>(num);
  };
#endif
 private:
  DISALLOW_COPY_AND_ASSIGN(etSitePacketData);
};

struct etSitePacket : public etDataPacket {
  static etPointerTranslator * sitePointerTranslator;
  static etPointerTranslator * siteStringTranslator;

  void init(int num) {
    initHeader(TYPE_SITEPACKET/*datatype*/, KEYHOLE_VERSION/*version*/, num,
               sizeof(etSitePacketData));
#ifdef FORCE_BASE32
    dataInstanceSize_BASE32 = sizeof(etSitePacketData_BASE32);
#endif
  };

  void initUTF8(int num) {
    initHeader(TYPE_SITEPACKET_UTF8/*datatype*/, KEYHOLE_VERSION/*version*/,
               num, sizeof(etSitePacketData));
#ifdef FORCE_BASE32
    dataInstanceSize_BASE32 = sizeof(etSitePacketData_BASE32);
#endif
  };

  etSitePacketData * getPtr(int num) {
    return reinterpret_cast<etSitePacketData *>(getDataInstanceP(num));
  };

  void pointerToOffset() {
    if (sitePointerTranslator == NULL) {
      sitePointerTranslator = createSitePointerTranslator();
      siteStringTranslator = createSiteStringTranslator();
    }

    byteSwap();
    sitePointerTranslator->translatePointerToOffset(this);
    siteStringTranslator->translateStringToOffset(this);
#ifdef FORCE_BASE32
    movetooffset();
#endif
  };

  void offsetToPointer() {
#ifdef FORCE_BASE32
    if (packetBuffer == reinterpret_cast<char *>(1)) {
      dataInstanceSize_BASE32 = sizeof(etSitePacketData_BASE32);
      packetHeader.dataInstanceSize = sizeof(etSitePacketData);
      char * newpacketBuffer = reinterpret_cast<char *>(malloc(
          packetHeader.dataInstanceSize*packetHeader.numInstances));
      for (int i = 0; i < static_cast<int>(packetHeader.numInstances); i++)
        memcpy(&newpacketBuffer[i*packetHeader.dataInstanceSize],
               &dataInstances[i*dataInstanceSize_BASE32],
               dataInstanceSize_BASE32);
      packetBuffer = newpacketBuffer;
      dataInstances = newpacketBuffer;
    }
#endif
    if (sitePointerTranslator == NULL) {
      sitePointerTranslator = createSitePointerTranslator();
      siteStringTranslator = createSiteStringTranslator();
    }
#ifdef FORCE_BASE32
    movefromoffset();
#endif
    sitePointerTranslator->translateOffsetToPointer(this);
    siteStringTranslator->translateOffsetToString(this);
    byteSwap();
  };


  etPointerTranslator * createSitePointerTranslator();
  etPointerTranslator * createSiteStringTranslator();

  inline void byteSwap() {
    for (id_size i = 0; i < packetHeader.numInstances; i++) {
      getPtr(i)->byteSwap();
    }
  };
#ifdef FORCE_BASE32
  void movetooffset() {
    for (id_size i = 0; i < packetHeader.numInstances; i++) {
      getPtr(i)->movetooffset();
    }
  };

  void movefromoffset() {
    for (id_size i = 0; i < packetHeader.numInstances; i++) {
      getPtr(i)->movefromoffset();
    }
  };
#endif
 private:
  DISALLOW_COPY_AND_ASSIGN(etSitePacket);
};

#ifdef FORCE_BASE32
struct etLandmarkPacketData_BASE32 {
  std::uint32_t           name;
  ushort       numPt;
  ushort           bitFlags;

#ifdef VECTOR_COMPRESSION
  std::uint32_t localPt;
#else
  std::uint32_t localPt;
#endif


  /***********NEW VARIABLES FOR Landmark***********/

  std::uint32_t iconName_deprecated;
  int style;
  std::uint32_t description;
  int referenceType;
  std::uint32_t reference;
  int referenceSize;
 private:
  DISALLOW_COPY_AND_ASSIGN(etLandmarkPacketData_BASE32);
};
#endif

struct etLandmarkPacketData {
#ifdef FORCE_BASE32
  std::uint32_t           name_OFFSET;
#else
  etString     name;
#endif
  ushort       numPt;
  ushort           bitFlags;

#ifdef FORCE_BASE32
#ifdef VECTOR_COMPRESSION
  std::uint32_t localPt_OFFSET;
#else
  std::uint32_t localPt_OFFSET;
#endif
#else
#ifdef VECTOR_COMPRESSION
  etVec3us    *localPt;
#else
  etVec3d     *localPt;
#endif
#endif


  /***********NEW VARIABLES FOR Landmark***********/

#ifdef FORCE_BASE32
  std::uint32_t iconName_deprecated_OFFSET;
#else
  const char * iconName_deprecated;
#endif

  int style;

#ifdef FORCE_BASE32
  std::uint32_t description_OFFSET;
#else
  const char * description;
#endif
  int referenceType;
#ifdef FORCE_BASE32
  std::uint32_t reference_OFFSET;
#else
  unsigned char * reference;
#endif
  int referenceSize;

#ifdef FORCE_BASE32
  etString     name;
#ifdef VECTOR_COMPRESSION
  etVec3us    *localPt;
#else
  etVec3d     *localPt;
#endif
  const char * iconName_deprecated;
  const char * description;
  unsigned char * reference;
#endif


  inline void byteSwap() {
    for (int i = 0; i < numPt; i++) {
      localPt[i].byteSwap();
    }

    name.byteSwap();
    swapb(numPt);
    swapb(style);

    swapb(referenceType);
    swapb(referenceSize);
  };

#ifdef FORCE_BASE32
  void movetooffset() {
    name_OFFSET = reinterpret_cast<std::uint64_t>(name.string);
    localPt_OFFSET = reinterpret_cast<std::uint64_t>(localPt);
    iconName_deprecated_OFFSET = reinterpret_cast<std::uint64_t>(iconName_deprecated);
    description_OFFSET = reinterpret_cast<std::uint64_t>(description);
    reference_OFFSET = reinterpret_cast<std::uint64_t>(reference);
  };

  void movefromoffset() {
    std::uint64_t num;
    num = localPt_OFFSET;
#ifdef VECTOR_COMPRESSION
    localPt = reinterpret_cast<etVec3us *>(num);
#else
    localPt = reinterpret_cast<etVec3d *>(num);
#endif
    num = name_OFFSET;
    name.string = reinterpret_cast<char *>(num);
    num = iconName_deprecated_OFFSET;
    iconName_deprecated = reinterpret_cast<char *>(num);
    num = description_OFFSET;
    description = reinterpret_cast<char *>(num);
    num = reference_OFFSET;
    reference = reinterpret_cast<unsigned char *>(num);
  };
#endif
 private:
  DISALLOW_COPY_AND_ASSIGN(etLandmarkPacketData);
};

struct etLandmarkPacket : public etDataPacket {
  static etPointerTranslator * landmarkPointerTranslator;
  static etPointerTranslator * landmarkStringTranslator;

  void init(int num) {
    initHeader(TYPE_LANDMARK/*datatype*/, KEYHOLE_VERSION/*version*/, num,
               sizeof(etLandmarkPacketData));
#ifdef FORCE_BASE32
    dataInstanceSize_BASE32 = sizeof(etLandmarkPacketData_BASE32);
#endif
  };

  etLandmarkPacketData * getPtr(int num) {
    return reinterpret_cast<etLandmarkPacketData *>(getDataInstanceP(num));
  };

  void pointerToOffset() {
    if (landmarkPointerTranslator == NULL) {
      landmarkPointerTranslator = createLandmarkPointerTranslator();
      landmarkStringTranslator = createLandmarkStringTranslator();
    }

    byteSwap();
    landmarkPointerTranslator->translatePointerToOffset(this);
    landmarkStringTranslator->translateStringToOffset(this);
#ifdef FORCE_BASE32
    movetooffset();
#endif
  };

  void offsetToPointer() {
#ifdef FORCE_BASE32
    if (packetBuffer == reinterpret_cast<char *>(1)) {
      dataInstanceSize_BASE32 = sizeof(etLandmarkPacketData_BASE32);
      packetHeader.dataInstanceSize = sizeof(etLandmarkPacketData);
      char * newpacketBuffer = reinterpret_cast<char *>(malloc(
          packetHeader.dataInstanceSize*packetHeader.numInstances));
      for (int i = 0; i < static_cast<int>(packetHeader.numInstances); i++)
        memcpy(&newpacketBuffer[i*packetHeader.dataInstanceSize],
               &dataInstances[i*dataInstanceSize_BASE32],
               dataInstanceSize_BASE32);
      packetBuffer = newpacketBuffer;
      dataInstances = newpacketBuffer;
    }
#endif
    if (landmarkPointerTranslator == NULL) {
      landmarkPointerTranslator = createLandmarkPointerTranslator();
      landmarkStringTranslator = createLandmarkStringTranslator();
    }
#ifdef FORCE_BASE32
    movefromoffset();
#endif
    landmarkPointerTranslator->translateOffsetToPointer(this);
    landmarkStringTranslator->translateOffsetToString(this);
    byteSwap();
  };


  etPointerTranslator * createLandmarkPointerTranslator();
  etPointerTranslator * createLandmarkStringTranslator();

  inline void byteSwap() {
    for (id_size i = 0; i < packetHeader.numInstances; i++) {
      getPtr(i)->byteSwap();
    }
  };
#ifdef FORCE_BASE32
  void movetooffset() {
    for (id_size i = 0; i < packetHeader.numInstances; i++) {
      getPtr(i)->movetooffset();
    }
  };

  void movefromoffset() {
    for (id_size i = 0; i < packetHeader.numInstances; i++) {
      getPtr(i)->movefromoffset();
    }
  };
#endif
  etLandmarkPacket() {}
 private:
  DISALLOW_COPY_AND_ASSIGN(etLandmarkPacket);
};


#ifdef FORCE_BASE32
struct etPolygonPacketData_BASE32 {
  std::uint32_t           name;
  int          reserved1;
  ushort       numPt;
  ushort       bitFlags;
  ushort           numEdgeFlags;

#ifdef VECTOR_COMPRESSION
  std::uint32_t localPt;
#else
  std::uint32_t localPt;
#endif

#ifdef VECTOR_COMPRESSION
  std::uint32_t edgeFlags;
#else
  std::uint32_t edgeFlags;
#endif

  int          style;
 private:
  DISALLOW_COPY_AND_ASSIGN(etPolygonPacketData_BASE32);
};
#endif

struct etPolygonPacketData {
#ifdef FORCE_BASE32
  std::uint32_t           name_OFFSET;
#else
  etString     name;
#endif
  int          properties;
  ushort       numPt;
  ushort       bitFlags;
  ushort           numEdgeFlags;

#ifdef FORCE_BASE32
#ifdef VECTOR_COMPRESSION
  std::uint32_t localPt_OFFSET;
#else
  std::uint32_t localPt_OFFSET;
#endif
#else
#ifdef VECTOR_COMPRESSION
  etVec3us    *localPt;
#else
  etVec3d     *localPt;
#endif
#endif

#ifdef FORCE_BASE32
#ifdef VECTOR_COMPRESSION
  std::uint32_t edgeFlags_OFFSET;
#else
  std::uint32_t edgeFlags_OFFSET;
#endif
#else
#ifdef VECTOR_COMPRESSION
  bool *edgeFlags;
#else
  bool *edgeFlags;
#endif
#endif

  int          style;

#ifdef FORCE_BASE32
  etString     name;
#ifdef VECTOR_COMPRESSION
  etVec3us    *localPt;
#else
  etVec3d     *localPt;
#endif

#ifdef VECTOR_COMPRESSION
  bool *edgeFlags;
#else
  bool *edgeFlags;
#endif

#endif

  inline void byteSwap() {
    for (int i = 0; i < numPt; i++) {
      localPt[i].byteSwap();
    }

    for (int i = 0; i < numEdgeFlags; i++) {
      swapb(edgeFlags[i]);
    }

    name.byteSwap();
    swapb(properties);
    swapb(numPt);
    swapb(bitFlags);
    swapb(style);
    swapb(numEdgeFlags);
  };

#ifdef FORCE_BASE32
  void movetooffset() {
    localPt_OFFSET = reinterpret_cast<std::uint64_t>(localPt);
    name_OFFSET = reinterpret_cast<std::uint64_t>(name.string);
    edgeFlags_OFFSET = reinterpret_cast<std::uint64_t>(edgeFlags);
  };

  void movefromoffset() {
    std::uint64_t num;
    num = localPt_OFFSET;
#ifdef VECTOR_COMPRESSION
    localPt = (etVec3us *)num;
#else
    localPt = (etVec3d *)num;
#endif
    num = name_OFFSET;
    name.string = (char *)num;

    num = edgeFlags_OFFSET;
#ifdef VECTOR_COMPRESSION
    edgeFlags = (bool *)num;
#else
    edgeFlags = (bool *)num;
#endif
  };
#endif
 private:
  DISALLOW_COPY_AND_ASSIGN(etPolygonPacketData);
};

struct etPolygonPacket : public etDataPacket {
  static etPointerTranslator * polygonPointerTranslator;
  static etPointerTranslator * polygonStringTranslator;

  void init(int num) {
    initHeader(TYPE_POLYGONPACKET/*datatype*/, KEYHOLE_VERSION/*version*/, num,
               sizeof(etPolygonPacketData));
#ifdef FORCE_BASE32
    dataInstanceSize_BASE32 = sizeof(etPolygonPacketData_BASE32);
#endif
  };

  etPolygonPacketData * getPtr(int num) {
    return (etPolygonPacketData *)getDataInstanceP(num);
  };

  void pointerToOffset() {
    if (polygonPointerTranslator == NULL) {
      polygonPointerTranslator = createPolygonPointerTranslator();
      polygonStringTranslator = createPolygonStringTranslator();
    }

    byteSwap();
    polygonPointerTranslator->translatePointerToOffset(this);
    polygonStringTranslator->translateStringToOffset(this);
#ifdef FORCE_BASE32
    movetooffset();
#endif
  };

  void offsetToPointer() {
#ifdef FORCE_BASE32
    if (packetBuffer == reinterpret_cast<char *>(1)) {
      dataInstanceSize_BASE32 = sizeof(etPolygonPacketData_BASE32);
      packetHeader.dataInstanceSize = sizeof(etPolygonPacketData);
      char * newpacketBuffer = reinterpret_cast<char *>(malloc(
          packetHeader.dataInstanceSize*packetHeader.numInstances));
      for (int i = 0; i < static_cast<int>(packetHeader.numInstances); i++) {
        memcpy(&newpacketBuffer[i*packetHeader.dataInstanceSize],
               &dataInstances[i*dataInstanceSize_BASE32],
               dataInstanceSize_BASE32);
      }
      packetBuffer = newpacketBuffer;
      dataInstances = newpacketBuffer;
    }
#endif
    if (polygonPointerTranslator == NULL) {
      polygonPointerTranslator = createPolygonPointerTranslator();
      polygonStringTranslator = createPolygonStringTranslator();
    }
#ifdef FORCE_BASE32
    movefromoffset();
#endif
    polygonPointerTranslator->translateOffsetToPointer(this);
    polygonStringTranslator->translateOffsetToString(this);
    byteSwap();
  };

  etPointerTranslator * createPolygonPointerTranslator();
  etPointerTranslator * createPolygonStringTranslator();

  inline void byteSwap() {
    for (id_size i = 0; i < packetHeader.numInstances; i++) {
      getPtr(i)->byteSwap();
    }
  };
#ifdef FORCE_BASE32
  void movetooffset() {
    for (id_size i = 0; i < packetHeader.numInstances; i++) {
      getPtr(i)->movetooffset();
    }
  };

  void movefromoffset() {
    for (id_size i = 0; i < packetHeader.numInstances; i++) {
      getPtr(i)->movefromoffset();
    }
  };
#endif
  etPolygonPacket() {}
 private:
  DISALLOW_COPY_AND_ASSIGN(etPolygonPacket);
};


struct etDrawablePacket : public etDataPacket {
  static etPointerTranslator * packetPointerTranslator;

  void init(int num) {
    initHeader(TYPE_DRAWABLEPACKET/*datatype*/, KEYHOLE_VERSION/*version*/, num,
               sizeof(etDataPacket));
#ifdef FORCE_BASE32
    dataInstanceSize_BASE32 = sizeof(etDataPacket_BASE32);
#endif
  };

  etDataPacket * getPtr(int num) {
    return reinterpret_cast<etDataPacket *>(getDataInstanceP(num));
  };

  void pointerToOffset() {
    if (!packetPointerTranslator) {
      packetPointerTranslator = createPacketPointerTranslator();
    }

    byteSwap();
    packetPointerTranslator->translatePointerToOffset(this);
#ifdef FORCE_BASE32
    movetooffset();
#endif
  };

  void offsetToPointer() {
#ifdef FORCE_BASE32
    if (packetBuffer == reinterpret_cast<char *>(1)) {
      dataInstanceSize_BASE32 = sizeof(etDataPacket_BASE32);
      packetHeader.dataInstanceSize = sizeof(etDataPacket);
      char * newpacketBuffer = reinterpret_cast<char *>(malloc(
          packetHeader.dataInstanceSize*packetHeader.numInstances));
      for (int i = 0; i < static_cast<int>(packetHeader.numInstances); i++)
        memcpy(&newpacketBuffer[i*packetHeader.dataInstanceSize],
               &dataInstances[i*dataInstanceSize_BASE32],
               dataInstanceSize_BASE32);
      packetBuffer = newpacketBuffer;
      dataInstances = newpacketBuffer;
    }
#endif
    if (!packetPointerTranslator)
      packetPointerTranslator = createPacketPointerTranslator();
#ifdef FORCE_BASE32
    movefromoffset();
#endif
    packetPointerTranslator->translateOffsetToPointer(this);
    byteSwap();
  };

  void merge(etDrawablePacket * pak1, etDrawablePacket * pak2);

  etPointerTranslator * createPacketPointerTranslator();

  inline void byteSwap() {
    for (id_size i = 0; i < packetHeader.numInstances; i++) {
      getPtr(i)->byteSwap();
    }
  };
#ifdef FORCE_BASE32
  void movetooffset() {
    for (id_size i = 0; i < packetHeader.numInstances; i++) {
      getPtr(i)->movetooffset();
    }
  };

  void movefromoffset() {
    for (id_size i = 0; i < packetHeader.numInstances; i++) {
      getPtr(i)->movefromoffset();
    }
  };
#endif
  etDrawablePacket() {}
 private:
  DISALLOW_COPY_AND_ASSIGN(etDrawablePacket);
};

#endif  // COMMON_PACKET_H__
