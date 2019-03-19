#ifndef _MEM_USAGE_RESOURCE_USAGE_INFO_HPP
#define _MEM_USAGE_RESOURCE_USAGE_INFO_HPP

#include <sys/resource.h>


namespace opengee {

namespace mem_usage {

struct ResourceUsageInfo
{
    public:

    int vm_peak_kb, vm_size_kb, vm_data_kb;
    struct rusage rusage;
};

}

}

#endif // _MEM_USAGE_RESOURCE_USAGE_INFO_HPP
