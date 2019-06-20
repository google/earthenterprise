#include <iomanip>
#include <iostream>
#include <sstream>

#include <mem_usage/raster_project_function_tester.hpp>

#include <autoingest/AssetVersion.h>



class LoadRasterProjectAssetVersionFromXml_Tester : public opengee::mem_usage::RasterProjectFunctionTester
{
    public:

    // A rather useless count.  Used instead of special directives to
    // prevent the compiler from optimizing out parsing the asset XML.
    int asset_ref_character_count = 0;

    LoadRasterProjectAssetVersionFromXml_Tester(
        std::string test_name="LoadRasterProjectAssetVersionFromXml")
    : opengee::mem_usage::RasterProjectFunctionTester(test_name)
    {}

    virtual bool run_test_operation()
    {
        std::string asset_name = get_next_asset_name();

        if (!asset_name_exists(asset_name))
        {
            // Break operation processing:
            return true;
        }

        // Construct an asset reference:
        AssetVersion asset(asset_name);

        // Cause the reference to be resolved by loading the XML:
        asset_ref_character_count += asset->GetRef().toString().length();

        // Keep processing operations:
        return false;
    }

    protected:

    virtual void log_completion_fields(
        int complete_operations_count,
        const struct rusage &start_resources,
        const struct rusage &end_resources)
    {
        opengee::mem_usage::RasterProjectFunctionTester::log_completion_fields(
            complete_operations_count, start_resources, end_resources);
        log_emitter << YAML::Key << "total AssetRef character count" <<
            YAML::Value << asset_ref_character_count;
    }
};

int main(int argc, char** argv)
{
    try
    {
        LoadRasterProjectAssetVersionFromXml_Tester tester;

        tester.run_test();
    }
    catch (std::system_error &exc)
    {
        std::cerr << exc.code().message() << std::endl;
        throw;
    }

    return 0;
}
