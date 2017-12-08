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



#include "khSystemManager.h"
#include <notify.h>
#include <khException.h>
#include <fusionversion.h>

int main(int argc, char *argv[])
{
  try {
    // tell the notify sub system that I want time prefixed messages
    NotifyPrefix = "[time]";

    notify(NFY_NOTICE, "--------------- Started ---------------");
    notify(NFY_NOTICE, "Fusion Version: %s", GEE_VERSION);

    theSystemManager.Run();

    // don't do anything of significance after theSystemManager.Run();
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unhandled exception");
  }

  return 0;
}
