#ifndef _MEM_USAGE_PATH_HPP
#define _MEM_USAGE_PATH_HPP

#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <system_error>


namespace opengee {

namespace mem_usage {

namespace path {

std::string read_link(const std::string &path)
{
    // PATH_MAX is bad, but we'll live with it for spiking:
    char link_path_cstr[PATH_MAX + 1];

    ssize_t link_path_length =
        readlink(path.c_str(), link_path_cstr, sizeof(link_path_cstr) - 1);
    if (link_path_length > 0)
    {
        link_path_cstr[link_path_length] = '\0';
        return std::string(link_path_cstr);
    }
    else
    {
        throw std::system_error(
            errno, std::generic_category(), "Failed to readlink: " + path);
    }
}

std::string real_path(const std::string &path)
{
    char real_path_buffer[PATH_MAX + 1];
    char *real_path_cstr = realpath(path.c_str(), real_path_buffer);

    if (real_path_cstr == nullptr)
    {
        throw std::system_error(
            errno, std::generic_category(), "Failed to get realpath: " + path);
    }

    return std::string(real_path_cstr);
}

bool exists(const std::string &path)
{
    struct stat path_status;

    if (stat(path.c_str(), &path_status))
    {
        if (errno == ENOENT)
        {
            return false;
        }
        throw std::system_error(errno, std::generic_category(),
            "Failed test if path exists: " + path);
    }
    else
    {
        return true;
    }
}

}

}

}

#endif // _MEM_USAGE_PATH_HPP
