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

#include "MemoryMonitor.h"

MemoryMonitor& MemoryMonitor::Instance() {
    static MemoryMonitor _instance;
    static bool doOnce = true;
    if (doOnce) {
        notify(NFY_WARN, "Created Memory Monitor Instance");
        doOnce = false;
        _instance.CalculateMemoryUsage();
    }
    return _instance;
}

void MemoryMonitor::CalculateMemoryUsage(void) {
    std::vector<ulong> memoryVars(4, 0);
    uint memoryUsed;
    std::string line;
    std::ifstream memFile;
    std::vector <std::string> tokens;
    memFile.open("/proc/meminfo");
    while (getline(memFile,line)) {
        std::stringstream stringStream(line);
        std::string intermediate;
        while(getline(stringStream, intermediate, ' ')) {
            if (intermediate.find_first_not_of(' ') != std::string::npos){
                tokens.push_back(intermediate);
            }
        }
        if (tokens[0].compare("MemTotal:") == 0) {
            memoryVars[0] = stoul(tokens[1]);
        }
        else if (tokens[0].compare("MemFree:") == 0) {
            memoryVars[1] = stoul(tokens[1]);
        }
        else if (tokens[0].compare("Buffers:") == 0) {
            memoryVars[2] = stoul(tokens[1]);
        }
        else if (tokens[0].compare("Cached:") == 0) {
            memoryVars[3] = stoul(tokens[1]);
            tokens.clear();
            break;
        }
        tokens.clear();
    }
    memFile.close();
    if (memoryVars[0] == 0) {
        memoryUsed = 0;
    }
    else {
        memoryUsed = (((float) (memoryVars[0] - memoryVars[1] - memoryVars[2] - memoryVars[3]))/memoryVars[0])*100;
    }
    used = memoryUsed;
    notify(NFY_WARN, "Memory Calcualted: %u%%", memoryUsed);
}