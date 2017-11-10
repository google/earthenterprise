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


/******************************************************************************
File:        fusionversion.h
******************************************************************************/
#ifndef __fusionversion_h
#define __fusionversion_h

#include <string>

// defined in gee_version.h generated by SCons
extern const char *const GEE_VERSION;

enum FusionProductType {
  FusionPro,
  FusionLT,
  FusionInternal
};

extern FusionProductType GetFusionProductType(void);
extern void SetFusionProductType(FusionProductType type);
extern std::string GetFusionProductName(void);
extern std::string GetFusionProductShortName(void);

#endif /* __fusionversion_h */
