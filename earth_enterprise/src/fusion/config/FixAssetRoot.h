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



#ifndef FUSION_CONFIG_FIXASSETROOT_H__
#define FUSION_CONFIG_FIXASSETROOT_H__

#include <string>

class geUserId;


extern void FixSpecialPerms(const std::string &assetroot, bool secure);

// will throw if user doesn't confirm
extern void PromptUserAndFixOwnership(const std::string &assetroot, bool noprompt);


// used by configure
// tries its best to create each of the special dirs with the
// correct owner and permissions
// returns true if it had to chown any of the dirs
extern bool MakeSpecialDirs(const std::string &assetroot,
                            const geUserId &fusion_user,
                            bool secure);

#endif // FUSION_CONFIG_FIXASSETROOT_H__
