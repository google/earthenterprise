#include <iostream>

#include <mem_usage/thread_member_function_tester.hpp>



// This test case leaks a constant amount of memory on each iteration.  It's
// used to verify that the memory usage tester shows memory leaks.
class LeakMemory_Tester : public opengee::mem_usage::ThreadMemberFunctionTester
{
    int leaked_bytes = 0;

    public:

    LeakMemory_Tester(std::string test_name="LeakMemory")
    : opengee::mem_usage::ThreadMemberFunctionTester(test_name)
    {}

    virtual bool run_test_operation()
    {
        char *leaked_obj = new char[1024];

        if (leaked_obj)
        {
            leaked_bytes += 1024;
        }

        // Keep processing operations:
        return false;
    }
};

int main(int argc, char** argv)
{
    try
    {
        LeakMemory_Tester tester;

        tester.run_test();
    }
    catch (std::system_error &exc)
    {
        std::cerr << exc.code().message() << std::endl;
        throw;
    }

    return 0;
}