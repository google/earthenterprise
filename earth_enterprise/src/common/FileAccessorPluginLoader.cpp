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

#include "FileAccessorPluginLoader.h"
#include "POSIXFileAccessor.h"

const std::string pluginDir = "/opt/google/plugin/fileaccessor/";

void ScanPluginDirectory(std::vector<std::string> &files) {
    boost::filesystem::path p(pluginDir);
    boost::filesystem::directory_iterator start(p);
    boost::filesystem::directory_iterator end;
    std::transform(start, end, std::back_inserter(files), 
        [](const boost::filesystem::directory_entry& entry) {
            return pluginDir + entry.path().leaf().string(); });
}

FileAccessorPluginLoader& FileAccessorPluginLoader::Get() {
    static FileAccessorPluginLoader loader;
    return loader;
}

class POSIXFileAccessorFactory: public FileAccessorFactory {
    friend FileAccessorPluginLoader;
protected:
    virtual AbstractFileAccessor* GetAccessor(const std::string &fileName) {
        if (fileName[0] == '/')
            return new POSIXFileAccessor();
        return nullptr;
    }
};

static POSIXFileAccessorFactory posixFactory;
void FileAccessorPluginLoader::DefaultLoadPluginsImpl (std::vector<PluginFactory> &factories) {
    factories.push_back({(void*)0, &posixFactory});
    std::vector<std::string> files;
    ScanPluginDirectory(files);
    for (auto s : files) {
        FileAccessorPluginLoader::PluginHandle handle = dlopen(s.c_str(), RTLD_NOW);
        std::cout << "Opened the plugin Handle is " << handle << "\n";
        if (handle){
            get_factory_t get_factory = reinterpret_cast<get_factory_t>(dlsym(handle, "get_factory_v1"));
            FileAccessorFactory *pFactory = get_factory();
            if(pFactory){
                factories.push_back({handle,pFactory});
            }
            else{
                dlclose(handle);
            }
        }
        else {
            std::cout << "Couldn't open plugin: " << dlerror() << "\n";
        }
    }
}

FileAccessorPluginLoader::FileAccessorPluginLoader(LoadPluginFunc LoadImpl /*= nullptr*/, UnloadPluginFunc UnloadImpl /*= nullptr*/) {
    loadPluginFunc = LoadImpl ? LoadImpl : DefaultLoadPluginsImpl;
    unloadPluginFunc = UnloadImpl ? UnloadImpl : [](PluginHandle handle){dlclose(handle);};
}

FileAccessorPluginLoader::~FileAccessorPluginLoader() {
    // First, remove the default POSIX 
    //PluginFactory posixDefault({(void*)0, &posixFactory});
    auto it = std::find(factories.begin(), factories.end(), PluginFactory({(void*)0, &posixFactory})/*posixDefault*/);
    assert(it != factories.end());
    if(it != factories.end())
        factories.erase(it);

    // Now close all of the other factories
    for (auto factory : factories)
        unloadPluginFunc(factory.first);
}

void FileAccessorPluginLoader::LoadPlugins() {
    std::lock_guard<std::mutex> lock(loadMutex);
    if (loaded)
        return;

    loadPluginFunc(factories);
    
    loaded = true;
}

std::unique_ptr<AbstractFileAccessor> FileAccessorPluginLoader::GetAccessor(std::string file_path) {
    // Even though LoadPlugins checks `loaded`, check here to avoid incurring
    // the cost of the lock every time someone asks for a file accessor.
    if (!loaded)
        LoadPlugins();

    for(PluginFactory f : factories){
        AbstractFileAccessor *pAccessor = f.second->GetAccessor(file_path);
        if (pAccessor) {
            return std::unique_ptr<AbstractFileAccessor>(pAccessor);
        }
    }

    return nullptr; // Return POSIX accessor by default?
}
