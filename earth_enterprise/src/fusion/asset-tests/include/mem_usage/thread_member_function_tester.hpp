#ifndef _MEM_USAGE_THREAD_MEMBER_FUNCTION_TESTER_HPP
#define _MEM_USAGE_THREAD_MEMBER_FUNCTION_TESTER_HPP

#include <cctype>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>

#include <date/date.h>

#include "ostream_tee.hpp"
#include "resource_usage_info.hpp"
#include "resources.hpp"
#include "utils.hpp"
#include "yaml-cpp-wrapper.hpp"


namespace opengee {

namespace mem_usage {

using resources::operator-;


class ThreadTestClass {
    public:

    int call_count = 0;

    void test_member_function()
    {
        // This has a race condition, so don't expect this count to be valid.
        // It's used as a minimum function body, so, perhaps, your compiler
        // doesn't optimize out the whole operation.
        ++call_count;
    }
};

class ThreadMemberFunctionTester
{
    private:

    std::string test_name;
    int operation_count;
    int log_progress_operation_count;
    int max_thread_count;
    std::ofstream resource_usage_csv_file;
    std::regex first_number_regex;
    OStreamTee log_stream;
    ResourceUsageInfo resource_usage;

    protected:

    YamlEmitter log_emitter;

    public:

    ThreadTestClass thread_test_instance;

    ThreadMemberFunctionTester(
        std::string test_name, int operation_count=1000000,
        int log_progress_operation_count=1000, int max_thread_count=10
    ) : test_name(test_name),
        operation_count(operation_count),
        log_progress_operation_count(log_progress_operation_count),
        max_thread_count(max_thread_count),
        first_number_regex("[0-9]+"),
        log_stream{{ &std::cout }},
        log_emitter(log_stream)
    {
        log_stream.add_output_path(get_self_path() + ".log.yaml");

        // This may avoid string escaping bugs in minimal string quoting until
        // they're fixed upstream:
        log_emitter.SetStringFormat(YAML::DoubleQuoted);

        resource_usage_csv_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        // The standard G++ exception thrown by ofstream::open is more or less
        // useless, so we add, at least, the file path, and re-throw:
        try
        {
            resource_usage_csv_file.open("resource_usage.csv", std::ofstream::out);
        }
        catch (std::system_error &exc)
        {
            throw std::system_error(exc.code(), "Error opening <" + test_name +
                "/resource_usage.csv>: " + exc.code().message());
        }
        resource_usage_csv_file << "VmPeak,VmSize,VmData,CpuUserTime,CpuSystemTime" << std::endl;
    }

    int get_max_thread_count()
    {
        return max_thread_count;
    }

    void set_max_thread_count(int value)
    {
        max_thread_count = value;
    }

    virtual bool run_test_operation() = 0;

    virtual void log_test_start()
    {
        log_emitter << YAML::BeginDoc;
        log_emitter << YAML::BeginSeq;

        log_emitter << YAML::Flow << YAML::BeginMap;
        log_test_start_fields();
        log_emitter << YAML::EndMap;
        if (!log_emitter.good())
        {
            throw std::runtime_error(
                "YAML emitter error: " + log_emitter.GetLastError());
        }
    }

    virtual void log_progress(int complete_operations_count)
    {
        log_emitter << YAML::Flow << YAML::BeginMap;
        log_progress_fields(complete_operations_count);
        log_emitter << YAML::EndMap;
        if (!log_emitter.good())
        {
            throw std::runtime_error(
                "YAML emitter error: " + log_emitter.GetLastError());
        }
    }

    virtual void log_completion(
        int complete_operations_count,
        const struct rusage &start_resources,
        const struct rusage &end_resources)
    {
        log_progress(complete_operations_count);
        log_emitter << YAML::Flow << YAML::BeginMap;
        log_completion_fields(
            complete_operations_count, start_resources, end_resources);
        log_emitter << YAML::EndMap;

        log_emitter << YAML::EndSeq;
        log_emitter << YAML::EndDoc;
        if (!log_emitter.good())
        {
            throw std::runtime_error(
                "YAML emitter error: " + log_emitter.GetLastError());
        }
    }

    void run_test()
    {
        int half_max_thread_count = std::max(1, max_thread_count/2);
        int c;

        log_test_start();

        struct rusage start_resources = resources::get_r_usage(RUSAGE_SELF);

        for (c = 0; c < operation_count; ++c)
        {
            if (run_test_operation())
            {
                break;
            }
            if (c % log_progress_operation_count == 0)
            {
                read_memory_usage_info();
                read_cpu_usage_info();
                log_progress(c);
                output_resource_usage_info();
            }
            if (c % half_max_thread_count)
            {
                wait_for_max_thread_count(half_max_thread_count);
            }
        }

        struct rusage end_resources = resources::get_r_usage(RUSAGE_SELF);

        log_completion(c, start_resources, end_resources);
        output_resource_usage_info();
    }

    protected:

    virtual void log_test_start_fields()
    {
        log_emitter << YAML::Key << "time" <<
            YAML::Value << get_log_current_time_string() <<
            YAML::Key << "type" << YAML::Value << "start configuration" <<
            YAML::Key << "test name" << YAML::Value << test_name;
    }

    virtual void log_progress_fields(int complete_operations_count)
    {
        log_emitter << YAML::Key << "time" <<
            YAML::Value << get_log_current_time_string() <<
            YAML::Key << "type" << YAML::Value << "progress report" <<
            YAML::Key << "operation" <<
            YAML::Value << complete_operations_count <<
            YAML::Key << "thread count" <<
            YAML::Value << get_thread_count() <<
            YAML::Key << "vm size,kB" <<
            YAML::Value << resource_usage.vm_size_kb;
    }

    virtual void log_completion_fields(
        int complete_operations_count,
        const struct rusage &start_resources,
        const struct rusage &end_resources)
    {
        log_emitter << YAML::Key << "time" << YAML::Value <<
            get_log_current_time_string() <<
            YAML::Key << "type" << YAML::Value << "completion report" <<
            YAML::Key << "user time,s" <<
            YAML::Value << resources::to_string(
                end_resources.ru_utime - start_resources.ru_utime) <<
            YAML::Key << "system time,s" <<
            YAML::Value << resources::to_string(
                end_resources.ru_stime - start_resources.ru_stime);
    }

    std::string get_log_current_time_string()
    {
        return date::format(
                "%FT%TZ",
                date::floor<std::chrono::milliseconds>(
                    std::chrono::system_clock::now()));
    }

    int get_thread_count()
    {
        std::ifstream status_file("/proc/self/status");
        std::string line;
        std::smatch number_match;

        while (std::getline(status_file, line))
        {
            if (line.rfind("Threads:", 0) == 0)
            {
                std::regex_search(line, number_match, first_number_regex);
                return stoi(number_match[0]);
            }
        }

        throw std::runtime_error(
            "Failed to get thread count.  No 'Threads' entry recognized in "
            "</proc/self/status>!");
    }

    private:

    // Waits until there are at most `max_thread_count` threads in the current
    // process.
    //     The implementation uses polling.  You can use it in cases when
    // you're testing detached threads that you can't join.
    //     You can't have less than one thread, and still be running this
    // function.
    void wait_for_max_thread_count(int max_thread_count)
    {
        int sleep_milliseconds = 100;

        if (max_thread_count < 1)
        {
            throw std::range_error(
                "A `max_thread_count` of less than 1 causes an infinite loop.  "
                "You can't have less than 1 thread and be running this "
                "function.");
        }
        while (get_thread_count() > max_thread_count)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(
                sleep_milliseconds));
        }
    }

    void output_resource_usage_info()
    {
        output_memory_usage_info();
        resource_usage_csv_file << ",";
        output_cpu_usage_info();
        resource_usage_csv_file << std::endl;
    }

    void read_memory_usage_info()
    {
        std::ifstream status_file("/proc/self/status");
        std::string line;
        std::smatch number_match;
        std::string vm_peak, vm_size, vm_data;

        while (std::getline(status_file, line))
        {
            if (line.rfind("VmPeak:", 0) == 0)
            {
                std::regex_search(line, number_match, first_number_regex);
                resource_usage.vm_peak_kb = std::stoi(number_match[0]);
            }
            if (line.rfind("VmSize:", 0) == 0)
            {
                std::regex_search(line, number_match, first_number_regex);
                resource_usage.vm_size_kb = std::stoi(number_match[0]);
            }
            if (line.rfind("VmData:", 0) == 0)
            {
                std::regex_search(line, number_match, first_number_regex);
                resource_usage.vm_data_kb = std::stoi(number_match[0]);
            }
        }
    }

    void output_memory_usage_info()
    {
        resource_usage_csv_file <<
            resource_usage.vm_peak_kb << "," <<
            resource_usage.vm_size_kb << "," <<
            resource_usage.vm_data_kb;
    }

    void read_cpu_usage_info()
    {
        resource_usage.rusage = resources::get_r_usage(RUSAGE_SELF);
    }

    void output_cpu_usage_info()
    {
        resource_usage_csv_file <<
            resources::to_string(resource_usage.rusage.ru_utime) << "," <<
            resources::to_string(resource_usage.rusage.ru_stime);
    }
};


}

}

#endif // _MEM_USAGE_THREAD_MEMBER_FUNCTION_TESTER_HPP
