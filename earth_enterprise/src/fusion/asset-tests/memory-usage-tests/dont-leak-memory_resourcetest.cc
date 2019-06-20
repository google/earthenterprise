#include <iostream>

#include <mem_usage/thread_member_function_tester.hpp>



// This test case doesn't leaks memory on each iteration.  It's used to verify
// that the memory usage tester doesn't show spurious memory leaks.
//     It can also be used to estimate the base amount of memory, and CPU time
// used by the test infrastructure.
class DontLeakMemory_Tester : public opengee::mem_usage::ThreadMemberFunctionTester
{
    int operation_count = 0;

    public:

    DontLeakMemory_Tester(std::string test_name="DontLeakMemory")
    : opengee::mem_usage::ThreadMemberFunctionTester(test_name)
    {}

    virtual bool run_test_operation()
    {
        // Dummy operation:
        ++operation_count;

        // Keep processing operations:
        return false;
    }
};

int main(int argc, char** argv)
{
    try
    {
        DontLeakMemory_Tester tester;

        tester.run_test();
    }
    catch (std::system_error &exc)
    {
        std::cerr << exc.code().message() << std::endl;
        throw;
    }

    return 0;
}