#ifndef _MEM_USAGE_RESOURCES_HPP
#define _MEM_USAGE_RESOURCES_HPP

#include <sys/resource.h>

#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>


namespace opengee {

namespace mem_usage {

namespace resources
{

struct rusage get_r_usage(int who = RUSAGE_SELF)
{
    struct rusage usage;

    if (getrusage(who, &usage) != 0)
    {
        throw std::system_error(errno, std::generic_category(),
            "Failed to get resource usage (who = " + std::to_string(who) +
            ")!");
    }

    return usage;
}

struct timeval operator-(const struct timeval &a, const struct timeval &b)
{
    struct timeval result;

    // FIXME: This could overflow:
    result.tv_sec = a.tv_sec - b.tv_sec;
    result.tv_usec = a.tv_usec - b.tv_usec;
    if (result.tv_usec < 0)
    {
        result.tv_sec += result.tv_usec / 1000000;
        // The sign of the modulus of a negative integer is
        // "implementation dependent" in C++. :P
        result.tv_usec %= 1000000;
        if (result.tv_usec < 0)
        {
            result.tv_usec += 1000000;
        }
    }

    return result;
}

std::string to_string(const struct timeval &value)
{
    std::stringstream result;

    result << value.tv_sec << '.' << std::setfill('0') << std::setw(6) <<
        value.tv_usec;

    return result.str();
}

}

}

}

#endif // _MEM_USAGE_RESOURCES_HPP
