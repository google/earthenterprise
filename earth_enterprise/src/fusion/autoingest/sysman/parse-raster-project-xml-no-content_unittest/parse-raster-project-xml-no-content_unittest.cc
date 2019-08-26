#include <iomanip>
#include <iostream>
#include <sstream>

#include <gtest/gtest.h>

#include <mem_usage/asset_function_tester.hpp>
#include <mem_usage/utils.hpp>

#include <autoingest/sysman/AssetD.h>

#include <AssetThrowPolicy.h>
#include <autoingest/sysman/plugins/RasterProjectAssetD.h>


// Get access to protected members of class RasterProjectAssetVersionImplD:
class FriendlyRasterProjectAssetVersionImplD : public RasterProjectAssetVersionImplD
{
    friend class ParseRasterProjectXmlNoContent_Test;

    public:

    static std::shared_ptr<FriendlyRasterProjectAssetVersionImplD> Load(std::string boundref)
    {
        AssetSerializerLocalXML<AssetVersionImpl> serializer;
        std::shared_ptr<AssetVersionImplD> unfriendly_result =
            std::dynamic_pointer_cast<AssetVersionImplD>(serializer.Load(boundref));
        std::shared_ptr<FriendlyRasterProjectAssetVersionImplD> result = 
            *reinterpret_cast<std::shared_ptr<FriendlyRasterProjectAssetVersionImplD>*>(&unfriendly_result);

        return result;
    }
};

class ParseRasterProjectXmlNoContent_Test :
    public opengee::mem_usage::AssetFunctionTester, public ::testing::Test
{
    public:

    // A rather useless count.  Used instead of special directives to
    // prevent the compiler from optimizing out parsing the asset XML.
    int asset_ref_character_count = 0;

    ParseRasterProjectXmlNoContent_Test(
        std::string test_name="ParseRasterProjectXmlNoContent",
        int operation_count=1, int report_operation_count=1000,
        int max_thread_count=100)
    : opengee::mem_usage::AssetFunctionTester(
        test_name, operation_count, report_operation_count, max_thread_count)
    {}

    void run_delayed_build_children_test()
    {
        // This names an asset with XMLs, but no content directories, such as
        // mask, mosaic, product:
        std::string asset_name = acquire_invalid_raster_project_asset_name(
            "asset-01-input-with-no-content-files");

        assert(asset_name_exists(asset_name));

        std::shared_ptr<FriendlyRasterProjectAssetVersionImplD> asset_version =
            FriendlyRasterProjectAssetVersionImplD::Load(asset_name);

        // Currently, this causes a segmentation fault:
        asset_version->DelayedBuildChildren();
    }

    void run_asset_update_test()
    {
        // This names an asset with XMLs, but no content directories, such as
        // mask, mosaic, product:
        std::string asset_name = get_invalid_raster_project_asset_name(
            "asset-01-input-with-no-content-files");

        // Construct an asset reference:
        AssetD asset(asset_name);
        bool needed = false;
        asset->Update(needed);

        // Currently, this causes a segmentation fault:
        // Cause the reference to be resolved by loading the XML:
        asset_ref_character_count += asset->GetRef().toString().length();
    }

    virtual bool run_test_operation()
    {
        // Stop processing operations:
        return true;
    }
};

TEST_F(ParseRasterProjectXmlNoContent_Test, DalayedBuildChildrenWorksWithEmptyXmlFields)
{
    run_delayed_build_children_test();

    // Assert that a segmentation fauld didn't occur.
}

TEST_F(ParseRasterProjectXmlNoContent_Test, AssetUpdateWorksWithEmptyXmlFields)
{
    run_asset_update_test();

    // Assert that a segmentation fauld didn't occur.
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
