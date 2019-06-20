#include <iomanip>
#include <iostream>
#include <sstream>

#include <mem_usage/access_private_variables.hpp>
#include <mem_usage/raster_project_function_tester.hpp>
#include <mem_usage/utils.hpp>

#include <autoingest/sysman/khAssetManager.h>
#include <autoingest/MiscConfig.h>
#include <autoingest/sysman/AssetD.h>
#include <notify.h>
#include <khThread.h>



// Get access to khAssetManager's private member `mutex`:
struct khAssetManager_mutex {
    typedef khNoDestroyMutex(khAssetManager::*type);
};

// Explicit instantiation; the only place where it is legal to pass
// the address of a private member.  Generates the static ::instance
// that in turn initializes stowed<Tag>::value.
template class mem_usage::access_private_variables::stow_private<
    khAssetManager_mutex, &khAssetManager::mutex>;


class BuildRasterProject_Tester : public opengee::mem_usage::RasterProjectFunctionTester
{
    public:

    // A rather useless count.  Used instead of special directives to
    // prevent the compiler from optimizing out parsing the asset XML.
    int asset_ref_character_count = 0;

    BuildRasterProject_Tester(std::string test_name="BuildRasterProject",
        int operation_count=1000000, int report_operation_count=1000,
        int max_thread_count=100)
    : opengee::mem_usage::RasterProjectFunctionTester(
        test_name, operation_count, report_operation_count, max_thread_count)
    {
        MiscConfig::Instance().AssetCacheSize = 128000;
        MiscConfig::Instance().VersionCacheSize = 128000;
        setNotifyLevel(NFY_NOTICE);
    }

    void build_asset(const std::string &asset_name)
    {
        // Lock `theAssetManager.mutex`.  Otherwise, this will fail on a debug
        // build:
        //
        //     assert(!mutex.TryLock());
        //
        // On a non-debug build you're in danger of getting a segmentation
        // fault because of unsynchronized threads.
        //     `theAssetManager.mutex` is private, so we're a bit hacky to get
        // access to it:
        khLockGuard lock(
            theAssetManager.*mem_usage::access_private_variables::stowed<
                khAssetManager_mutex>::value);

        // Construct an asset reference:
        AssetD asset(asset_name);
        bool needed = false;
        asset->Update(needed);

        // Cause the reference to be resolved by loading the XML:
        asset_ref_character_count += asset->GetRef().toString().length();
    }

    virtual bool run_test_operation()
    {
        std::string asset_name = get_next_asset_name();

        if (!asset_name_exists(asset_name))
        {
            // Break operation processing:
            return true;
        }

        build_asset(asset_name);

        // Keep processing operations:
        return false;
    }
};

int main(int argc, char** argv)
{
    try
    {
        BuildRasterProject_Tester tester;

        tester.run_test();
    }
    catch (std::system_error &exc)
    {
        std::cerr << exc.code().message() << std::endl;
        throw;
    }

    return 0;
}
