/*
 * Copyright 2019 The Open GEE Contributors
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

#include "MiscConfig.h"
#include <fstream>
#include <iostream>
#include <sstream>

class MemoryMonitor {
private:
    ulong memTotal;
    ulong memFree;
    ulong buffers;
    ulong cached;
    uint used;

public:
    void CalculateMemoryUsage(){
        std::string line;
        std::ifstream memFile;
        std::vector <std::string> tokens;
        memFile.open("/proc/meminfo");
        while (getline(memFile,line)) {
            std::stringstream stringStream(line);
            std::string intermediate;
            while(getline(stringStream, intermediate, ' ')) 
            { 
            if (intermediate.find_first_not_of(' ') != std::string::npos){
                tokens.push_back(intermediate);
            }
            }
            if (tokens[0].compare("MemTotal:") == 0) {
            memTotal = stoul(tokens[1]);
            }
            else if (tokens[0].compare("MemFree:") == 0) {
            memFree = stoul(tokens[1]);
            }
            else if (tokens[0].compare("Buffers:") == 0) {
            buffers = stoul(tokens[1]);
            }
            else if (tokens[0].compare("Cached:") == 0) {
            cached = stoul(tokens[1]);
            tokens.clear();
            break;
            }
            tokens.clear();
        }
        memFile.close();
        used = (((float) (memTotal - memFree - buffers - cached))/memTotal)*100;
    }

    uint GetUsage(){
        return used;
    }

    /*if (MiscConfig::Instance().LimitMemoryUtilization) {
      if (used > MiscConfig::Instance().MaxMemoryUtilization){
        notify(NFY_WARN, "Usage:%u", used);
      }
      else{
        notify(NFY_NOTICE, "Usage:%u", used);
      }
    }*/
}