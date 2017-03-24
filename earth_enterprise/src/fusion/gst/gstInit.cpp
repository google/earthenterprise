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


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <gdal_priv.h>

#include <gstTypes.h>
#include <gstMemory.h>

#include <gstFormatManager.h>
#include <gstIconManager.h>
#include <gstAssetManager.h>
#include <gstFormatRules.h>
#include <gstSourceManager.h>
#include <gstEarthStream.h>
#include <maprender/SGLHelps.h>

#include <gstMemoryPool.h>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/.idl/storage/IconReference.h>
#include <geFilePool.h>

gstAssetManager *theAssetManager = NULL;

pthread_mutex_t MemoryMutex;

int CacheItemCount = 0;

static bool init_once = false;
geFilePool* GlobalFilePool = NULL;

void gstInit(geFilePool* file_pool) {
  if (init_once == true)
    notify(NFY_FATAL, "Can only initialize the gst library once!");

  GlobalFilePool = file_pool;

  pthread_mutex_init( &MemoryMutex, NULL );

  // register GDAL Formats
  GDALAllRegister();

  std::string iconpath = IconReference::CustomIconPath();
  if (!khDirExists(iconpath)) {
    khMakeDir(iconpath);     // will make recursively as needed
    // the icon dir needs wide open permissions for now
    khChmod(iconpath, 0777);
  }

  gstIconManager::init(iconpath.c_str());

  // initialize random number seed
  srand48(time(NULL));

  // setup asset manager
  theAssetManager = new gstAssetManager(AssetDefs::AssetRoot().c_str());

  gstFormatRules::init();

  gstSourceManager::init(250000);

  gstEarthStream::Init();

  // load optional fonts for map rendering
  maprender::FontInfo::LoadFontConfig();

  init_once = true;
}
