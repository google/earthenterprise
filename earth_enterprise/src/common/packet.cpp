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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <ctype.h>
#ifndef _WIN32
#include <alloca.h>
#else
#include <malloc.h>
#include <direct.h>
#endif
#include "packet.h"

char * khnull;
const size_t fname_len = 500;

#ifdef offsetof
#undef offsetof
#endif

#if _LP64
#define offsetof(TYPE, MEMBER) (((std::uint64_t) &((TYPE *)(&khnull))->MEMBER) - (std::uint64_t)(&khnull))
#else
#define offsetof(TYPE, MEMBER) (((std::uint32_t) &((TYPE *)(&khnull))->MEMBER) - (std::uint32_t)(&khnull))
#endif


#define PACKET_EXPORT
//#define VECTOR_COMPRESSION

// FlatFile DataPacket variables
#ifdef PACKET_EXPORT
char   etDataPacket::flatbasename[fname_len];
char   etDataPacket::flatdirname[fname_len];
char   etDataPacket::flatdataname[fname_len];
FILE * etDataPacket::flatdirfp = NULL;
FILE * etDataPacket::flatdatafp = NULL;
long   etDataPacket::datafilesize = 0;
int    etDataPacket::flatmode = 0;
#endif //PACKET_EXPORT

etPointerTranslator * etStreetPacket::streetPointerTranslator = NULL;
etPointerTranslator * etStreetPacket::streetStringTranslator = NULL;

etPointerTranslator * etPolyLinePacket::polyLinePointerTranslator = NULL;
etPointerTranslator * etPolyLinePacket::polyLineStringTranslator = NULL;

etPointerTranslator * etAreaPacket::areaPointerTranslator = NULL;
etPointerTranslator * etAreaPacket::areaStringTranslator = NULL;

etPointerTranslator * etSitePacket::sitePointerTranslator = NULL;
etPointerTranslator * etSitePacket::siteStringTranslator = NULL;

etPointerTranslator * etLandmarkPacket::landmarkPointerTranslator = NULL;
etPointerTranslator * etLandmarkPacket::landmarkStringTranslator = NULL;

etPointerTranslator * etPolygonPacket::polygonPointerTranslator = NULL;
etPointerTranslator * etPolygonPacket::polygonStringTranslator = NULL;

etPointerTranslator * etDrawablePacket::packetPointerTranslator = NULL;

//*******************Type definitions from the Client***********************

int etPattern::hash(const char* pattern)
{
  const int M = 26;
  const int primes[] = {
    1  * M, 2 * M, 3 * M, 5 * M, 7 * M,
    11 * M, 13* M, 15* M, 17* M, 19* M,
    23 * M, 29* M, 31* M, 37* M, 43* M, 47* M,
    51 * M, 53* M, 57* M, 59* M,
    61 * M, 67* M, 71* M, 79* M, 83* M, 89* M,
    91 * M, 97* M, 101*M };

  if (!pattern) return 0;

  // computes a pattern based on first, middle, and last character of string
  register unsigned int   h   = 0;
  register char          *c   = (char*)pattern;
  register int            f   = 0;

  while (*c)
  {
    h += *c++ * primes[f++];
    f %= 29;
  }

  return h;
}

//*************************DATA PACKET**********************************
#ifdef PACKET_EXPORT
unsigned int etDataPacket::addDataToDataBuffer(void * buffer, int buffersize)
{
  int pad = buffersize%4;

  if(pad)
    pad = 4-pad;

  while((int)packetHeader.dataBufferSize+buffersize+pad > maxDataBufferSize)
  {
    dataBuffer = (char *)realloc(dataBuffer, maxDataBufferSize*2);
    maxDataBufferSize *= 2;
  }

  memcpy(&dataBuffer[packetHeader.dataBufferSize], buffer, buffersize);
  packetHeader.dataBufferSize += buffersize+pad;

  return packetHeader.dataBufferSize - (buffersize+pad);
}
#endif //PACKET_EXPORT

int etDataPacket::load(char * filename)
{
  FILE * fp;

  struct stat statinfo;
  statinfo.st_size = 0;
  stat(filename, &statinfo);
  packetBufferSize = statinfo.st_size;

  if((fp=fopen(filename, "rb")) == NULL)
  {
    //printf("Cannot open file %s\n", filename);
    return -1;
  }

  packetBuffer = (char *)malloc(packetBufferSize);
  fread(packetBuffer, 1, packetBufferSize, fp);
  fclose(fp);

  return load(packetBuffer, packetBufferSize);
}

//Temporary function - COMPLETE LATER
/*
  void * getMetaBuffer(void * ptr)
  {
  return NULL;
  }
*/

int etDataPacket::load(char* buffer, int bufferSize)
{
  if(bufferSize <=0)
    return -1;

  ((etDataHeader *)buffer)->byteSwap();
  if(((etDataHeader *)buffer)->magicID != KEYHOLEMAGICID)
    return -1;

  memcpy(&packetHeader, buffer, sizeof(etDataHeader));

  if(packetHeader.metaBufferSize > 0)
    //              metaHeader = getMetaBuffer((char *)buffer + sizeof(etDataHeader));
    metaHeader = NULL;

  dataInstances = (char *)buffer + sizeof(etDataHeader) + packetHeader.metaBufferSize;

  dataBuffer = (char *)buffer + packetHeader.dataBufferOffset;

  packetBuffer = (char *)1;

  return 0;
}

#ifdef PACKET_EXPORT

int makePath(char *dir, int len)
{

#if defined(_WIN32)
#if !defined(PATH_MAX)
#define PATH_MAX    MAX_PATH
#endif

#define MKDIR(p, m)     mkdir(p)
#define STACK_ALLOC     _alloca
#else
#define MKDIR(p, m)     mkdir((p), (m))
#define STACK_ALLOC     alloca
#endif

  char* cp;
  char* path;
  size_t pathlen;

  if (dir == NULL || *dir == '\0')
    return 0;

  pathlen = len;
  cp = path = (char*)STACK_ALLOC(pathlen+1);
  memcpy(path, dir, pathlen+1);
  path[len] = '\0';

  while (true) {
    for (cp = cp+1; *cp; cp++) {
      if (*cp == '/') break;
#if defined(_WIN32)
      if (*cp == '\\') break;
#endif
    }
    if (*cp == '\0') break;

    char sep = *cp;
    *cp = '\0';

    if (isalpha((unsigned char)path[0]) && path[1] == ':' &&
        path[2] == '\0') {
      *cp = sep;
      continue;
    }

    MKDIR(path, 0777);
    *cp = sep;
  }
  MKDIR(path, 0777);
  return 1;
}

void writetobuffer(void * buffer, int buffersize, char * &_buf, int &_size)
{
  if(_size - buffersize < 0)
    return;

  memcpy(_buf, buffer, buffersize);
  _buf += buffersize;
  _size -= buffersize;
}

int etDataPacket::save(void * buffer, int buffersize)
{
  char * savebuffer;
  int savebuffersize;

  savebuffer = (char *)buffer;
  savebuffersize = buffersize;

#ifdef FORCE_BASE32
  //Modify dataInstanceSize of Header
  int tmpsize;
  tmpsize = packetHeader.dataInstanceSize;
  packetHeader.dataInstanceSize = dataInstanceSize_BASE32;
#endif

  //Header
  writetobuffer(&packetHeader, sizeof(etDataHeader), savebuffer, savebuffersize);

#ifdef FORCE_BASE32
  //Change dataInstanceSize back to original value
  packetHeader.dataInstanceSize = tmpsize;
#endif

  //Meta Header
  /*    if(metaHeader != NULL)
        {
        metaHeader->write(savebuffer, savebuffersize);
        ((etDataHeader *)buffer)->metaBufferSize = (buffersize - savebuffersize) - sizeof(etDataHeader);
        }
        else
  */
  ((etDataHeader *)buffer)->metaBufferSize = 0;

  //Data Instances
#ifdef FORCE_BASE32
  for(int i=0; i < (int)packetHeader.numInstances; i++)
    writetobuffer(getDataInstanceP(i), dataInstanceSize_BASE32, savebuffer, savebuffersize);
#else
  writetobuffer(dataInstances, packetHeader.dataInstanceSize*packetHeader.numInstances,
                savebuffer, savebuffersize);
#endif

  //Data Buffer
  ((etDataHeader *)buffer)->dataBufferOffset = buffersize - savebuffersize;
  writetobuffer(dataBuffer, packetHeader.dataBufferSize, savebuffer, savebuffersize);
  ((etDataHeader *)buffer)->byteSwap();
  return buffersize - savebuffersize;
}

int etDataPacket::getFlatFileCount()
{
  FILE *fp;
  char fname[fname_len];
  int count;

  snprintf(fname, fname_len, "%s.header", flatbasename);
  char* lastslash = strrchr(fname, '/');
#if defined(_WIN32)
  if (!lastslash)
    lastslash = strrchr(fname, '\\');
#endif
  makePath(fname, strlen(fname) - strlen(lastslash));
  if((fp = fopen(fname, "rb")) == NULL)
  {
    if((fp = fopen(fname, "wb")) == NULL)
      return -1;
    count = 0;
    fprintf(fp, "%d\n", count);
    fclose(fp);

    return 0;
  }

  fscanf(fp, "%d", &count);
  fclose(fp);

  return count;
}

int etDataPacket::setFlatFileCount(int num)
{
  FILE *fp;
  char fname[fname_len];

  snprintf(fname, fname_len, "%s.header", flatbasename);
  char* lastslash = strrchr(fname, '/');
#if defined(_WIN32)
  if (!lastslash)
    lastslash = strrchr(fname, '\\');
#endif
  makePath(fname, strlen(fname) - strlen(lastslash));
  if((fp = fopen(fname, "wb")) == NULL)
    return -1;

  fprintf(fp, "%d\n", num);
  fclose(fp);

  return num;
}

int etDataPacket::openFlat(char * flatfile)
{
  char fname[fname_len];
  int filecount;

  //Set File Names
  snprintf(flatbasename, fname_len, "%s", flatfile);
  snprintf(flatdirname, fname_len, "%sDIR", flatfile);
  snprintf(flatdataname, fname_len, "%sDATA", flatfile);

  //Get Current File Count
  filecount = getFlatFileCount();
  if(filecount < 0)
    return -1;

  //Open Directory File
  snprintf(fname, fname_len, "%s.p%d", flatdirname, filecount);
  char* lastslash = strrchr(fname, '/');
#if defined(_WIN32)
  if (!lastslash)
    lastslash = strrchr(fname, '\\');
#endif
  makePath(fname, strlen(fname) - strlen(lastslash));
  if((flatdirfp = fopen(fname, "ab")) == NULL)
    return -1;

  //Open Data File
  snprintf(fname, fname_len, "%s.p%d", flatdataname, filecount);
  lastslash = strrchr(fname, '/');
#if defined(_WIN32)
  if (!lastslash)
    lastslash = strrchr(fname, '\\');
#endif
  makePath(fname, strlen(fname) - strlen(lastslash));
  if((flatdatafp = fopen(fname, "ab")) == NULL)
    return -1;

  //Get Data File Size
  struct stat statinfo;
  statinfo.st_size = 0;
  stat(fname, &statinfo);
  datafilesize = (long)statinfo.st_size;

  //Check Data file Size
  if(datafilesize > FLAT_MAX_SIZE)
  {
    closeFlat();
    setFlatFileCount(filecount+1);
    openFlat(flatfile);
    return 1;
  }

  flatmode = 1;

  return 1;
}

unsigned long * phtable = NULL;
void parsebcode(char * fname)
{
  int i;
  int bcode[500] = {0};
  int bcodeR[500] = {0};
  int bsize;

  unsigned long num;

  if(phtable == NULL)
  {
    phtable = (unsigned long *)malloc(5000000*sizeof(unsigned long));
    memset(phtable, 0, 5000000*sizeof(unsigned long));
  }

  bsize = 0;

  for(i=0; i < (int)strlen(fname); i++)
  {
    if(fname[i] == 'A')
    {
      bcode[bsize] = 0;
      bsize++;
    }
    else if(fname[i] == 'B')
    {
      bcode[bsize] = 1;
      bsize++;
    }
    else if(fname[i] == 'C')
    {
      bcode[bsize] = 2;
      bsize++;
    }
    else if(fname[i] == 'D')
    {
      bcode[bsize] = 3;
      bsize++;
    }
  }

  for(i=0; i < bsize; i++)
    bcodeR[i] = bcode[bsize-1-i];

  num = 0;
  for(i=0; i < 11; i++)
    num += (unsigned long)powf(4, i)*bcodeR[i];

  phtable[num]++;

  if(phtable[num] > 1)
    printf("t%lu: %lu\n", num, phtable[num]);

}

int etDataPacket::saveFlat(char * fname, char* buffer, int buffersize)
{
  size_t textlen = 100;
  char text[textlen];

  if(flatmode == 0)
    return -1;

  //parsebcode(fname);

  //Enter File into Directory
  snprintf(text, textlen, "%s", fname);
  fwrite(text, textlen, 1, flatdirfp);
  snprintf(text, textlen, "%d", buffersize);
  fwrite(text, textlen, 1, flatdirfp);

  //Save Data
  fwrite(buffer, buffersize, 1, flatdatafp);

  datafilesize += buffersize;

  if(datafilesize > FLAT_MAX_SIZE)
  {
    closeFlat();
    openFlat(flatbasename);
  }

  return 0;
}

int etDataPacket::closeFlat()
{
  fclose(flatdirfp);
  fclose(flatdatafp);

  flatmode = 0;

  return 0;
}

void etDataPacket::initHeader(int dataTypeID, int version, int numInstances, int dataInstanceSize,
                              void * metastruct)
{
  packetHeader.magicID = KEYHOLEMAGICID;
  packetHeader.dataTypeID = dataTypeID;
  packetHeader.version = version;
  packetHeader.numInstances = numInstances;
  packetHeader.dataInstanceSize = dataInstanceSize;
  packetHeader.dataBufferOffset = 0;
  packetHeader.dataBufferSize = 0;
  packetHeader.metaBufferSize = 0;

  metaHeader = metastruct;

  if(numInstances)
  {
    dataInstances = (char*)malloc(numInstances*dataInstanceSize);
    dataBuffer = (char*)malloc(DATABUFFERSIZE);
  }
  else
  {
    dataInstances = NULL;
    dataBuffer = NULL;
  }

  maxDataBufferSize = DATABUFFERSIZE;
}

#endif //PACKET_EXPORT

//**************************DATA TRANSLATOR******************************************

void etDataTranslator::allocTranslationTable(int num)
{
  translationTable = (etDataCypher *)malloc(sizeof(etDataCypher)*num);
  translationTableSize = num;
}

void etDataTranslator::setTranslationTable(int num, int fromOffset, int toOffset, int size,
                                           int pointerFlag)
{
  translationTable[num].fromOffset = fromOffset;
  translationTable[num].toOffset = toOffset;
  translationTable[num].size = size;
  translationTable[num].pointerFlag = pointerFlag;
}

void etDataTranslator::translate(void * fromptr, void * toptr)
{
  int i;

  for(i=0; i < translationTableSize; i++)
    memcpy((char *)toptr+translationTable[i].toOffset, (char *)fromptr+translationTable[i].fromOffset,
           translationTable[i].size);
}

void etDataTranslator::translateBack(void * fromptr, void * toptr)
{
  int i;

  for(i=0; i < translationTableSize; i++)
    memcpy((char *)toptr+translationTable[i].fromOffset, (char *)fromptr+translationTable[i].toOffset,
           translationTable[i].size);
}

void etDataTranslator::translate(void * fromptr, void * toptr, unsigned int pointerOffset)
{
  int i;

  for(i=0; i < translationTableSize; i++)
  {
    memcpy((char *)toptr+translationTable[i].toOffset, (char *)fromptr+translationTable[i].fromOffset,
           translationTable[i].size);
    if(translationTable[i].pointerFlag)
      *((char **)((char *)toptr+translationTable[i].toOffset)) += pointerOffset;
  }
}

//****************************POINTER TRANSLATOR***********************************

void etPointerTranslator::allocPointerList(int num)
{
  pointerList = (etPointerCypher *)malloc(sizeof(etPointerCypher)*num);
  pointerListSize = num;
}

#ifdef PACKET_EXPORT
void etPointerTranslator::translatePointerToOffset(etDataPacket * packet,
                                                   int cleanup) {
  char* ith_data_instance = packet->dataInstances;
  for (unsigned int i = 0; i < packet->packetHeader.numInstances; i++,
       ith_data_instance += packet->packetHeader.dataInstanceSize) {
    for (int j = 0; j < pointerListSize; j++) {
      if (pointerList[j].get == NULL) {
        continue;
      }
      const int size = pointerList[j].get(ith_data_instance);
      void** tmp_ptr = reinterpret_cast<void **>(
        ith_data_instance + pointerList[j].offset);

      unsigned long index = packet->addDataToDataBuffer(*tmp_ptr, size);
      swapb(index);

      if(cleanup) {
        free(*tmp_ptr);
      }

      *tmp_ptr = reinterpret_cast<void *>(index);
    }
  }
}
#endif //PACKET_EXPORT

void etPointerTranslator::translateOffsetToPointer(etDataPacket * packet) {
  char* ith_data_instance = packet->dataInstances;
  for (unsigned int i = 0; i < packet->packetHeader.numInstances; i++,
       ith_data_instance += packet->packetHeader.dataInstanceSize) {
    for (int j = 0; j < pointerListSize; j++) {
      void** tmp_ptr = reinterpret_cast<void **>(
          ith_data_instance + pointerList[j].offset);
      unsigned long index = reinterpret_cast<unsigned long>(*tmp_ptr);
      swapb(index);
      *tmp_ptr = reinterpret_cast<void *>(packet->dataBuffer + index);
    }
  }
}

#ifdef PACKET_EXPORT
void etPointerTranslator::translateStringToOffset(etDataPacket * packet,
                                                  int cleanup) {
  char* ith_data_instance = packet->dataInstances;
  for (unsigned int i = 0; i < packet->packetHeader.numInstances; i++,
       ith_data_instance += packet->packetHeader.dataInstanceSize) {
    for (int j = 0; j < pointerListSize; j++) {
      etString* tmp_ptr = reinterpret_cast<etString*>(
          ith_data_instance + pointerList[j].offset);
      char *string = (tmp_ptr->string - sizeof(etPattern));
      // +1 for the null character in string
      unsigned long index = packet->addDataToDataBuffer(
          string, strlen(tmp_ptr->string) + 1 + sizeof(etPattern));
      if (cleanup) {
        free(string);
      }
      swapb(index);
      tmp_ptr->string = reinterpret_cast<char *>(index);
    }
  }
}
#endif //PACKET_EXPORT

void etPointerTranslator::translateOffsetToString(etDataPacket * packet) {
  char* ith_data_instance = packet->dataInstances;
  for (unsigned int i = 0; i < packet->packetHeader.numInstances; i++,
       ith_data_instance += packet->packetHeader.dataInstanceSize) {
    for (int j = 0; j < pointerListSize; j++) {
      etString* tmp_ptr = reinterpret_cast<etString*>(
          ith_data_instance + pointerList[j].offset);
      unsigned long index = reinterpret_cast<unsigned long>(tmp_ptr->string);
      swapb(index);
      tmp_ptr->string = packet->dataBuffer + index + sizeof(etPattern);
    }
  }
}

//*****************************STREET PACKET**************************

int getStreetLocalPtSize(void *data)
{
#ifdef khEndianDoSwap

  ushort dummy = ((etStreetPacketData *)data)->numPt;
  swapb(dummy);

#ifdef VECTOR_COMPRESSION
  return dummy*sizeof(etVec3us);
#else
  return dummy*sizeof(etVec3d);
#endif

#else

#ifdef VECTOR_COMPRESSION
  return ((etStreetPacketData *)data)->numPt*sizeof(etVec3us);
#else
  return ((etStreetPacketData *)data)->numPt*sizeof(etVec3d);
#endif

#endif
}

etPointerTranslator * etStreetPacket::createStreetPointerTranslator()
{
  etPointerTranslator * tmp;

  tmp = new etPointerTranslator;

  tmp->allocPointerList(1);

  tmp->setPointerList(0, offsetof(etStreetPacketData, localPt), getStreetLocalPtSize);

  return tmp;
}

etPointerTranslator * etStreetPacket::createStreetStringTranslator()
{
  etPointerTranslator * tmp;

  tmp = new etPointerTranslator;

  tmp->allocPointerList(1);

  tmp->setPointerList(0, offsetof(etStreetPacketData, name));

  return tmp;
}

//***************************POLYLINE PACKET**************************

int getPolyLineLocalPtSize(void *data)
{
#ifdef khEndianDoSwap

  ushort dummy = ((etPolyLinePacketData *)data)->numPt;
  swapb(dummy);

#ifdef VECTOR_COMPRESSION
  return dummy*sizeof(etVec3us);
#else
  return dummy*sizeof(etVec3d);
#endif

#else

#ifdef VECTOR_COMPRESSION
  return ((etPolyLinePacketData *)data)->numPt*sizeof(etVec3us);
#else
  return ((etPolyLinePacketData *)data)->numPt*sizeof(etVec3d);
#endif

#endif
}

etPointerTranslator * etPolyLinePacket::createPolyLinePointerTranslator()
{
  etPointerTranslator * tmp;

  tmp = new etPointerTranslator;

  tmp->allocPointerList(1);

  tmp->setPointerList(0, offsetof(etPolyLinePacketData, localPt), getPolyLineLocalPtSize);

  return tmp;
}

etPointerTranslator * etPolyLinePacket::createPolyLineStringTranslator()
{
  etPointerTranslator * tmp;

  tmp = new etPointerTranslator;

  tmp->allocPointerList(1);

  tmp->setPointerList(0, offsetof(etPolyLinePacketData, name));

  return tmp;
}


//****************************AREA PACKET**************************

int getAreaLocalPtSize(void *data)
{
#ifdef khEndianDoSwap

  ushort dummy = ((etAreaPacketData *)data)->numPt;
  swapb(dummy);

#ifdef VECTOR_COMPRESSION
  return dummy*sizeof(etVec3us);
#else
  return dummy*sizeof(etVec3d);
#endif

#else

#ifdef VECTOR_COMPRESSION
  return ((etAreaPacketData *)data)->numPt*sizeof(etVec3us);
#else
  return ((etAreaPacketData *)data)->numPt*sizeof(etVec3d);
#endif

#endif
}

etPointerTranslator * etAreaPacket::createAreaPointerTranslator()
{
  etPointerTranslator * tmp;

  tmp = new etPointerTranslator;

  tmp->allocPointerList(1);

  tmp->setPointerList(0, offsetof(etAreaPacketData, localPt), getAreaLocalPtSize);

  return tmp;
}

etPointerTranslator * etAreaPacket::createAreaStringTranslator()
{
  etPointerTranslator * tmp;

  tmp = new etPointerTranslator;

  tmp->allocPointerList(1);

  tmp->setPointerList(0, offsetof(etAreaPacketData, name));

  return tmp;
}




//***************************SITE PACKET********************************

int getHTMLSize(void *data)
{
  return strlen(((etSitePacketData *)data)->html)+1;
}

#ifdef NEWSITESTUFF
int getAddressSize(void *data)
{
  return strlen(((etSitePacketData *)data)->address)+1;
}

int getPhoneSize(void *data)
{
  return strlen(((etSitePacketData *)data)->phone)+1;
}
#endif

int getSiteLocalPtSize(void *data)
{
#ifdef khEndianDoSwap

  ushort dummy = ((etSitePacketData *)data)->numPt;
  swapb(dummy);

#ifdef VECTOR_COMPRESSION
  return dummy*sizeof(etVec3us);
#else
  return dummy*sizeof(etVec3d);
#endif

#else

#ifdef VECTOR_COMPRESSION
  return ((etSitePacketData *)data)->numPt*sizeof(etVec3us);
#else
  return ((etSitePacketData *)data)->numPt*sizeof(etVec3d);
#endif

#endif
}

etPointerTranslator * etSitePacket::createSitePointerTranslator()
{
  etPointerTranslator * tmp;

  tmp = new etPointerTranslator;

#ifdef NEWSITESTUFF
  tmp->allocPointerList(4);
#else
  tmp->allocPointerList(2);
#endif

  tmp->setPointerList(0, offsetof(etSitePacketData, html), getHTMLSize);
  tmp->setPointerList(1, offsetof(etSitePacketData, localPt), getSiteLocalPtSize);

#ifdef NEWSITESTUFF
  tmp->setPointerList(2, offsetof(etSitePacketData, address), getAddressSize);
  tmp->setPointerList(3, offsetof(etSitePacketData, phone), getPhoneSize);
#endif

  return tmp;
}

etPointerTranslator * etSitePacket::createSiteStringTranslator()
{
  etPointerTranslator * tmp;

  tmp = new etPointerTranslator;

  tmp->allocPointerList(1);

  tmp->setPointerList(0, offsetof(etSitePacketData, name));

  return tmp;
}


//***************************LANDMARK PACKET********************************

int getSiteLocalPtSizeEXT1(void *data)
{
#ifdef khEndianDoSwap

  ushort dummy = ((etLandmarkPacketData *)data)->numPt;
  swapb(dummy);

#ifdef VECTOR_COMPRESSION
  return dummy*sizeof(etVec3us);
#else
  return dummy*sizeof(etVec3d);
#endif

#else

#ifdef VECTOR_COMPRESSION
  return ((etLandmarkPacketData *)data)->numPt*sizeof(etVec3us);
#else
  return ((etLandmarkPacketData *)data)->numPt*sizeof(etVec3d);
#endif

#endif
}

int getDescriptionSizeEXT1(void *data)
{
  return strlen(((etLandmarkPacketData *)data)->description)+1;
}

int getReferenceSizeEXT1(void *data)
{
  return ((etLandmarkPacketData *)data)->referenceSize;
}

int getIconNameSize(void *data)
{
  return strlen(((etLandmarkPacketData *)data)->iconName_deprecated)+1;
}


etPointerTranslator * etLandmarkPacket::createLandmarkPointerTranslator()
{
  etPointerTranslator * tmp;

  tmp = new etPointerTranslator;

  tmp->allocPointerList(4);

  tmp->setPointerList(0, offsetof(etLandmarkPacketData, iconName_deprecated), getIconNameSize);
  tmp->setPointerList(1, offsetof(etLandmarkPacketData, localPt), getSiteLocalPtSizeEXT1);
  tmp->setPointerList(2, offsetof(etLandmarkPacketData, description), getDescriptionSizeEXT1);
  tmp->setPointerList(3, offsetof(etLandmarkPacketData, reference), getReferenceSizeEXT1);

  return tmp;
}

etPointerTranslator * etLandmarkPacket::createLandmarkStringTranslator()
{
  etPointerTranslator * tmp;

  tmp = new etPointerTranslator;

  tmp->allocPointerList(1);

  tmp->setPointerList(0, offsetof(etLandmarkPacketData, name));

  return tmp;
}


//***************************POLYGON PACKET**************************

int getPolygonLocalPtSize(void *data)
{
#ifdef khEndianDoSwap

  ushort dummy = ((etPolygonPacketData *)data)->numPt;
  swapb(dummy);

#ifdef VECTOR_COMPRESSION
  return dummy*sizeof(etVec3us);
#else
  return dummy*sizeof(etVec3d);
#endif

#else

#ifdef VECTOR_COMPRESSION
  return ((etPolygonPacketData *)data)->numPt*sizeof(etVec3us);
#else
  return ((etPolygonPacketData *)data)->numPt*sizeof(etVec3d);
#endif

#endif
}


int getPolygonEdgeFlagsSize(void *data)
{
#ifdef khEndianDoSwap

  ushort dummy = ((etPolygonPacketData *)data)->numEdgeFlags;
  swapb(dummy);

#ifdef VECTOR_COMPRESSION
  return dummy*sizeof(bool);
#else
  return dummy*sizeof(bool);
#endif

#else

#ifdef VECTOR_COMPRESSION
  return ((etPolygonPacketData *)data)->numEdgeFlags*sizeof(bool);
#else
  return ((etPolygonPacketData *)data)->numEdgeFlags*sizeof(bool);
#endif

#endif
}


etPointerTranslator * etPolygonPacket::createPolygonPointerTranslator()
{
  etPointerTranslator * tmp;

  tmp = new etPointerTranslator;

  tmp->allocPointerList(2);

  tmp->setPointerList(0, offsetof(etPolygonPacketData, localPt), getPolygonLocalPtSize);
  tmp->setPointerList(1, offsetof(etPolygonPacketData, edgeFlags), getPolygonEdgeFlagsSize);

  return tmp;
}

etPointerTranslator * etPolygonPacket::createPolygonStringTranslator()
{
  etPointerTranslator * tmp;

  tmp = new etPointerTranslator;

  tmp->allocPointerList(1);

  tmp->setPointerList(0, offsetof(etPolygonPacketData, name));

  return tmp;
}



//***********************DRAWABLE PACKET*******************************

int getDataInstancesSize(void *data)
{
#ifdef khEndianDoSwap
  int num;
  ((etDataPacket*)data)->packetHeader.byteSwap();
  num = ((etDataPacket*)data)->packetHeader.dataInstanceSize*
        ((etDataPacket*)data)->packetHeader.numInstances;
  ((etDataPacket*)data)->packetHeader.byteSwap();
  return num;
#else
  return ((etDataPacket*)data)->packetHeader.dataInstanceSize*
    ((etDataPacket*)data)->packetHeader.numInstances;
#endif
}

int getDataBufferSize(void *data)
{
#ifdef khEndianDoSwap
  int num;
  ((etDataPacket*)data)->packetHeader.byteSwap();
  num = ((etDataPacket*)data)->packetHeader.dataBufferSize;
  ((etDataPacket*)data)->packetHeader.byteSwap();
  return num;
#else
  return ((etDataPacket*)data)->packetHeader.dataBufferSize;
#endif
}

etPointerTranslator * etDrawablePacket::createPacketPointerTranslator()
{
  etPointerTranslator * tmp;

  tmp = new etPointerTranslator;

  tmp->allocPointerList(2);

  tmp->setPointerList(0, offsetof(etDataPacket, dataInstances), getDataInstancesSize);
  tmp->setPointerList(1, offsetof(etDataPacket, dataBuffer),    getDataBufferSize);

  return tmp;
}

//NOTE:  pak1 and pak2 must be in pointer form not offset form
void etDrawablePacket::merge(etDrawablePacket * pak1, etDrawablePacket * pak2)
{
  init(pak1->packetHeader.numInstances+pak2->packetHeader.numInstances);

  for(unsigned int i=0; i < pak1->packetHeader.numInstances; i++)
    memcpy(getPtr(i), pak1->getPtr(i), sizeof(etDataPacket));

  for(unsigned int i=0; i < pak2->packetHeader.numInstances; i++)
    memcpy(getPtr(i+pak1->packetHeader.numInstances), pak2->getPtr(i),
           sizeof(etDataPacket));
}
