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
#include "notify.h"

void ScanPluginDirectory(const std::string &pluginDir, std::vector<std::string> &files) {
    boost::filesystem::path p(pluginDir);
    boost::system::error_code errorCode;
    boost::filesystem::directory_iterator start(p, errorCode);
    if (errorCode.value() != 0){
        notify(NFY_WARN, "Unable to scan file accessor plugin directory, %s. %s.", pluginDir.c_str(), errorCode.message().c_str());
        return;
    }

    boost::filesystem::directory_iterator end;
    std::transform(start, end, std::back_inserter(files), 
        [pluginDir](const boost::filesystem::directory_entry& entry) {
            return pluginDir + entry.path().leaf().c_str(); });
}

FileAccessorPluginLoader& FileAccessorPluginLoader::Get() {
    static FileAccessorPluginLoader loader;
    return loader;
}

void FileAccessorPluginLoader::DefaultLoadPluginsImpl (const std::string &pluginDirectory, std::vector<PluginFactory> &factories) {
    assert(factories.size() == 0);
    
    std::vector<std::string> files;
    ScanPluginDirectory(pluginDirectory, files);
    for (auto pluginFile : files) {
        FileAccessorPluginLoader::PluginHandle handle = dlopen(pluginFile.c_str(), RTLD_NOW);
        if (handle){
            const std::string getFactoryFunctionName = "get_factory_v1";
            get_factory_t get_factory = reinterpret_cast<get_factory_t>(dlsym(handle, getFactoryFunctionName.c_str()));
            FileAccessorFactory *pFactory = (get_factory != nullptr ? get_factory() : nullptr);
            if(pFactory){
                factories.push_back({handle,pFactory});
            }
            else{
                // pluginFile was a valid shared library, but did not provide the required function
                dlclose(handle);
                notify(NFY_WARN, "File accessor plugin %s does not implement the expected function %s", pluginFile.c_str(), getFactoryFunctionName.c_str());
            }
        }
        else {
            // pluginFile was something other than a shared library
            notify(NFY_WARN, "Unable to open file accessor plugin %s", dlerror());
        }
    }
}

FileAccessorPluginLoader::FileAccessorPluginLoader(
        LoadPluginFunc LoadImpl,      // default: nullptr
        UnloadPluginFunc UnloadImpl,  // default: nullptr
        std::string pluginDirectory   // default: "/opt/google/plugin/fileaccessor/"
        ):
        pluginDir(pluginDirectory) {
    loadPluginFunc = LoadImpl ? LoadImpl : DefaultLoadPluginsImpl;
    unloadPluginFunc = UnloadImpl ? UnloadImpl : [](PluginHandle handle){dlclose(handle);};
}

FileAccessorPluginLoader::~FileAccessorPluginLoader() {
    for (auto factory : factories){
        unloadPluginFunc(factory.first);
    }
}

void FileAccessorPluginLoader::LoadPlugins() {
    std::lock_guard<std::mutex> lock(loadMutex);
    if (loaded)
        return;

    loadPluginFunc(pluginDir, factories);

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

    // If there are no plugins loaded or a suitable one was not found, then
    // assume this is a POSIX file path.
    return std::unique_ptr<AbstractFileAccessor>(new POSIXFileAccessor());
}
