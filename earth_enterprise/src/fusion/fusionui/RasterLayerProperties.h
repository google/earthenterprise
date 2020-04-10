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



#ifndef _RasterLayerProperties_h_
#define _RasterLayerProperties_h_

#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/.idl/storage/InsetStackItem.h>

#include "rasterlayerpropertiesbase.h"

typedef unsigned int (*FromPixelFunc)( unsigned int );
typedef unsigned int (*ToPixelFunc)( unsigned int );

class RasterLayerProperties : public RasterLayerPropertiesBase
{
 public:
  RasterLayerProperties( QWidget* parent, const InsetStackItem &, AssetDefs::Type );

  InsetStackItem getConfig() const;

 private:
  FromPixelFunc fromPixel;
  ToPixelFunc toPixel;
    
};


#endif // !_RasterLayerProperties_h_
