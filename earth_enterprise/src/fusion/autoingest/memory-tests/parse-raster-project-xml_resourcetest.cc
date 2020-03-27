#include <iomanip>
#include <iostream>
#include <sstream>

#include <mem_usage/raster_project_function_tester.hpp>
#include <autoingest/plugins/RasterProjectAsset.h>
#include <autoingest/AssetSerializer.h>

class ParseAssetXml_Tester : public opengee::mem_usage::RasterProjectFunctionTester
{
    public:

    // A rather useless count.  Used instead of special directives to
    // prevent the compiler from optimizing out parsing the asset XML.
    int asset_ref_character_count = 0;

    ParseAssetXml_Tester(std::string test_name="ParseRasterProjectXml")
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

        AssetSerializerLocalXML<AssetImpl> serializer;
        std::shared_ptr<AssetImpl> asset = serializer.Load(asset_name);

        // Use the asset in an operation before throwing it away:
        asset_ref_character_count += asset->GetRef().toString().length();

        // Keep processing operations:
        return false;
    }
};

int main(int argc, char** argv)
{
    try
    {
        ParseAssetXml_Tester tester;

        tester.run_test();
    }
    catch (std::system_error &exc)
    {
        std::cerr << exc.code().message() << std::endl;
        throw;
    }

    return 0;
}
