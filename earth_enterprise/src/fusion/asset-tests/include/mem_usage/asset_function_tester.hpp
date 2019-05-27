#ifndef _MEM_USAGE_ASSET_FUNCTION_TESTER_HPP
#define _MEM_USAGE_ASSET_FUNCTION_TESTER_HPP

#include <iomanip>
#include <stdexcept>

#include <stdio.h>
#include <stdlib.h>

#include <common/notify.h>
#include <fusion/autoingest/Asset.h>
#include <fusion/autoingest/AssetVersion.h>
#include <fusion/autoingest/MiscConfig.h>
#include <autoingest/.idl/Systemrc.h>

#include "thread_member_function_tester.hpp"
#include "path.hpp"
#include "utils.hpp"



namespace opengee {

namespace mem_usage {


// Forward declaration:
class AssetFunctionTester;

class FriendlyAsset : public Asset
{
    friend AssetFunctionTester;
};

class FriendlyAssetVersion : public AssetVersion
{
    friend AssetFunctionTester;
};

class AssetFunctionTester : public ThreadMemberFunctionTester
{
    private:

    std::string valid_raster_project_asset_name_prefix = "RasterProjects/Valid/";
    std::string valid_raster_project_asset_name_suffix = ".kiproject";
    std::string invalid_raster_project_asset_name_prefix = "RasterProjects/Invalid/";
    std::string invalid_raster_project_asset_name_suffix = ".kiproject";
    std::string valid_raster_product_asset_name_prefix = "RasterProducts/Valid/";
    std::string valid_raster_product_asset_name_suffix = ".kiasset";
    std::string invalid_raster_product_asset_name_prefix = "RasterProducts/Invalid/";
    std::string invalid_raster_product_asset_name_suffix = ".kiasset";
    khNotifyLevel min_kh_notify_level;

    public:

    static bool asset_name_exists(const std::string &asset_name)
    {
        return path::exists(AssetImpl::XMLFilename(asset_name));
    }

    private:

    static void kh_notify_handler(
        void *self_ptr, khNotifyLevel severity, const char* fmt, va_list ap)
    {
        AssetFunctionTester *self =
            reinterpret_cast<AssetFunctionTester *>(self_ptr);

        if (severity > self->min_kh_notify_level)
        {
            return;
        }

        self->log_emitter << YAML::BeginMap <<
            YAML::Key << "time" <<
            YAML::Value << self->get_log_current_time_string() <<
            YAML::Key << "type" << YAML::Value << "khNotify message" <<
            YAML::Key << "severity" <<
            YAML::Value << khNotifyLevelToString(severity) <<
            YAML::Key << "message" <<
            YAML::Value << vprintf_to_string(fmt, ap) <<
            YAML::EndMap;
    }

    public:

    AssetFunctionTester(std::string test_name, int operation_count=1000000,
        int report_operation_count=1000, int max_thread_count=10,
        bool wrap_kh_notify_logging=true,
        khNotifyLevel min_kh_notify_level=NFY_NOTICE)
    : ThreadMemberFunctionTester(
        test_name, operation_count, report_operation_count, max_thread_count),
        min_kh_notify_level(min_kh_notify_level)
    {
        if (wrap_kh_notify_logging)
        {
            this->wrap_kh_notify_logging();
        }

        // Replace the asset root path read from </etc/opt/google/systemrc>
        // for our tests:
        Systemrc::OverrideAssetRoot(path::real_path(OPENGEE_ASSETS_DIR));
    }

    std::string get_numbered_asset_name(
        const std::string &asset_name_prefix,
        const std::string &asset_name_suffix, int asset_number)
    {
        std::ostringstream asset_name;

        asset_name << asset_name_prefix << "asset-" <<
            std::setfill('0') << std::setw(7) << asset_number <<
            asset_name_suffix;

        return asset_name.str();
    }

    std::string get_named_asset_name(
        const std::string &asset_name_prefix,
        const std::string &asset_name_suffix,
        const std::string &asset_base_name)
    {
        std::ostringstream asset_name;

        asset_name << asset_name_prefix << asset_base_name <<
            asset_name_suffix;

        return asset_name.str();
    }

    std::string get_valid_raster_project_asset_name(int asset_number)
    {
        return get_numbered_asset_name(
            valid_raster_project_asset_name_prefix,
            valid_raster_project_asset_name_suffix, asset_number);
    }

    std::string get_invalid_raster_project_asset_name(
        const std::string &asset_base_name)
    {
        return get_named_asset_name(
            invalid_raster_project_asset_name_prefix,
            invalid_raster_project_asset_name_suffix, asset_base_name);
    }

    std::string acquire_invalid_raster_project_asset_name(
        const std::string &asset_base_name
    )
    {
        std::string res =
            get_invalid_raster_project_asset_name(asset_base_name);

        // If the asset XML doesn't exist, run the script to generate it:
        if (!asset_name_exists(res))
        {
            int return_code =
                system((
                    get_self_dir() +
                    "/generate_test_asset_xmls.py --skip-valid-raster-projects"
                ).c_str());

            if (return_code)
            {
                throw std::runtime_error(
                    "Executing generate_test_asset_xmls.py failed with return code: " +
                    std::to_string(return_code));
            }
        }

        return res;
    }

    std::string get_valid_raster_product_asset_name(int asset_number)
    {
        return get_numbered_asset_name(
            valid_raster_product_asset_name_prefix,
            valid_raster_product_asset_name_suffix, asset_number);
    }

    std::string get_invalid_raster_product_asset_name(
        const std::string &asset_base_name)
    {
        return get_named_asset_name(
            invalid_raster_product_asset_name_prefix,
            invalid_raster_product_asset_name_suffix, asset_base_name);
    }

    void wrap_kh_notify_logging()
    {
        setNotifyHandler(kh_notify_handler, this);
    }

    protected:

    virtual void log_test_start_fields()
    {
        ThreadMemberFunctionTester::log_test_start_fields();

        log_emitter << YAML::Key << "asset cache capacity" <<
            YAML::Value << FriendlyAsset::CacheCapacity() <<
            YAML::Key << "asset version cache capacity" <<
            YAML::Value << FriendlyAssetVersion::CacheCapacity() <<
            YAML::Key << "notify level" <<
            YAML::Value << min_kh_notify_level <<
            YAML::Key << "notify level name" <<
            YAML::Value << khNotifyLevelToString(min_kh_notify_level);
    }

    virtual void log_progress_fields(int complete_operations_count)
    {
        ThreadMemberFunctionTester::log_progress_fields(
            complete_operations_count);

        log_emitter << YAML::Key << "asset cache size" <<
            YAML::Value << Asset::CacheSize() <<
            YAML::Key << "asset version cache size" <<
            YAML::Value << AssetVersion::CacheSize();
    }
};

}

}


#endif // _MEM_USAGE_ASSET_FUNCTION_TESTER_HPP
