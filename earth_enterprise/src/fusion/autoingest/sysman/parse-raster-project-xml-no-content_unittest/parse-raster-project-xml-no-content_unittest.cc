// Copyright 2021 The Open GEE Contributors
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

    static FriendlyRasterProjectAssetVersionImplD * Load(std::string boundref)
    {
        AssetSerializerLocalXML<AssetVersionImpl> serializer;
        // We don't wrap this in a shared_ptr to avoid having two independent
        // shared_ptrs that don't know about each other pointing at the same
        // object. Instead, we just return the raw pointer and depend on it
        // to not be deleted based on the way the test is written.
        FriendlyRasterProjectAssetVersionImplD * result =
            reinterpret_cast<FriendlyRasterProjectAssetVersionImplD*>(
                serializer.Load(boundref).operator->());

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

        auto asset_version =
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

TEST_F(ParseRasterProjectXmlNoContent_Test, DelayedBuildChildrenWorksWithEmptyXmlFields)
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
