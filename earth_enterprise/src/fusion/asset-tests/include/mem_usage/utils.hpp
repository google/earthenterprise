#ifndef _MEM_USAGE_UTILS_HPP
#define _MEM_USAGE_UTILS_HPP

#include <stdarg.h>
#include <stdio.h>
#include <string>

#include "path.hpp"

#ifndef MEM_USAGE_STRINGIFY
#   define MEM_USAGE_STRINGIFY(value)   #value
#endif // MEM_USAGE_STRINGIFY

#ifndef MEM_USAGE_STRINGIFY_VALUE
#   define MEM_USAGE_STRINGIFY_VALUE(value)   MEM_USAGE_STRINGIFY(value)
#endif // MEM_USAGE_STRINGIFY_VALUE


namespace opengee {

namespace mem_usage {

std::string get_self_path()
{
    return path::read_link("/proc/self/exe");
}

std::string get_self_dir()
{
    std::string self_path = get_self_path();

    return self_path.substr(0, self_path.rfind("/"));
}

std::string vprintf_to_string(
    const char *format, va_list format_parameters)
{
    va_list format_parameters_copy;

    // Get the size of the output string:
    va_copy(format_parameters_copy, format_parameters);

    int cstr_size = vsnprintf(NULL, 0, format, format_parameters_copy);

    va_end(format_parameters_copy);

    if (cstr_size < 0)
    {
        throw std::runtime_error(
            "Failed to format a khNotify message.  Format: " +
            std::string(format));
    }

    ++cstr_size;  // Add space for a terminating null character.

    // Allocate an output C string:
    char *output_buffer = static_cast<char*>(malloc(cstr_size));
    if (output_buffer == NULL)
    {
        throw std::runtime_error(
            "Failed to allocate memory for a khNotify message.  Format: " +
            std::string(format));
    }

    // Fill-in the formatted C string:
    int string_size =
        vsnprintf(output_buffer, cstr_size, format, format_parameters);

    if (string_size < 0)
    {
        free(output_buffer);
        throw std::runtime_error(
            "Failed to format a khNotify message.  Format: " + std::string(format));
    }

    // Return the result:
    std::string result(output_buffer);

    free(output_buffer);

    return result;
}

}

}

#endif // _MEM_USAGE_UTILS_HPP
