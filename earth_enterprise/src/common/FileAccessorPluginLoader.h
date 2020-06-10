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

#ifndef COMMON_FILEACCESSORPLUGINLOADER_H_
#define COMMON_FILEACCESSORPLUGINLOADER_H_

#include <string>
#include <vector>
#include <dlfcn.h>
#include <mutex>
#include <boost/filesystem.hpp>

class FileAccessorInterface {
public:
    virtual int GetSomething() = 0;
};

class FileAccessorFactoryInterface {
// NOTE: Is this an unnecessary layer? Should it be replaced by a function
// in the plugin module? One good thing is that it prevents you from having to
// make repeated dlsym calls.
public:
    virtual FileAccessorInterface* GetAccessor(const std::string &fileName) = 0;
};

void ScanPluginDirectory(std::vector<std::string> &files) {
    boost::filesystem::path p("/opt/google/plugin/fileaccessor");
    boost::filesystem::directory_iterator start(p);
    boost::filesystem::directory_iterator end;
    std::transform(start, end, std::back_inserter(files), 
        [](const boost::filesystem::directory_entry& entry) {
            return entry.path().leaf().string(); });
}

typedef FileAccessorFactoryInterface* (*get_factory_t)();

class FileAccessorPluginLoader {
public:
    using PluginHandle = void*;
    using PluginFactory = std::pair<PluginHandle,FileAccessorFactoryInterface*>;
    using LoadPluginFunc = std::function<void(std::vector<PluginFactory> &)>;
    using UnloadPluginFunc = std::function<void(PluginHandle)>;

    FileAccessorPluginLoader(LoadPluginFunc LoadImpl = nullptr, UnloadPluginFunc UnloadImpl = nullptr) {
        loadPluginFunc = LoadImpl ? LoadImpl : DefaultLoadPluginsImpl;
        unloadPluginFunc = UnloadImpl ? UnloadImpl : [](PluginHandle handle){dlclose(handle);};
    }

    ~FileAccessorPluginLoader() {
        for (auto factory : factories)
            unloadPluginFunc(factory.first);
    }

    void LoadPlugins() {
        std::lock_guard<std::mutex> lock(loadMutex);
        if (loaded)
            return;

        loadPluginFunc(factories);
        
        loaded = true;
    }

    FileAccessorInterface* GetAccessor(std::string file_path) {
        // Even though LoadPlugins checks `loaded`, check here to avoid incurring
        // the cost of the lock every time someone asks for a file accessor.
        if (!loaded)
            LoadPlugins();

        for(PluginFactory f : factories){
            FileAccessorInterface *pAccessor = f.second->GetAccessor(file_path);
            if (pAccessor) {
                return pAccessor;
            }
        }

        return nullptr; // Return POSIX accessor by default?
    }

    static void DefaultLoadPluginsImpl (std::vector<PluginFactory> &factories) {
        std::vector<std::string> files;
        ScanPluginDirectory(files);
        for (auto s : files) {
            FileAccessorPluginLoader::PluginHandle handle = dlopen(s.c_str(), RTLD_LAZY);
            if (handle){
                get_factory_t get_factory = reinterpret_cast<get_factory_t>(dlsym(handle, "get_factory_v1"));
                FileAccessorFactoryInterface *pFactory = get_factory();
                if(pFactory){
                    factories.push_back({handle,pFactory});
                }
                else
                    dlclose(handle);
            }
        }
    }

private:
    bool loaded = false;
    std::vector<PluginFactory> factories;
    std::mutex loadMutex;
    LoadPluginFunc loadPluginFunc;
    UnloadPluginFunc unloadPluginFunc;
};



#endif // COMMON_FILEACCESSORPLUGINLOADER_H_