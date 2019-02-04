#ifndef _MEM_USAGE_RASTER_PROJECT_FUNCTION_TESTER_HPP
#define _MEM_USAGE_RASTER_PROJECT_FUNCTION_TESTER_HPP

#include <string>

#include "asset_function_tester.hpp"



namespace opengee {

namespace mem_usage {

class RasterProjectFunctionTester : public AssetFunctionTester
{
    private:

    int asset_number = 0;

    public:

    RasterProjectFunctionTester(std::string test_name, int operation_count=1000000,
        int report_operation_count=1000, int max_thread_count=10,
        bool wrap_kh_notify_logging=true,
        khNotifyLevel min_kh_notify_level=NFY_NOTICE)
    : AssetFunctionTester(
        test_name, operation_count, report_operation_count, max_thread_count,
        wrap_kh_notify_logging, min_kh_notify_level)
    {}

    std::string get_last_asset_name()
    {
        return get_valid_raster_project_asset_name(asset_number);
    }

    std::string get_next_asset_name()
    {
        std::string result = get_last_asset_name();

        ++asset_number;

        return result;
    }
};

}

}


#endif // _MEM_USAGE_RASTER_PROJECT_FUNCTION_TESTER_HPP
