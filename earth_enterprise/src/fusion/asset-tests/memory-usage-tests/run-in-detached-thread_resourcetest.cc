#include <iostream>

#include <mem_usage/thread_member_function_tester.hpp>

#include <khThread.h>
#include <khFunctor.h>



class RunInDetachedThread_Tester : public opengee::mem_usage::ThreadMemberFunctionTester
{
    public:

    RunInDetachedThread_Tester(std::string test_name="RunInDetachedThread")
    : opengee::mem_usage::ThreadMemberFunctionTester(test_name)
    {}

    virtual bool run_test_operation()
    {
        RunInDetachedThread
            (khFunctor<void>(std::mem_fun
                (&opengee::mem_usage::ThreadTestClass::test_member_function),
                &thread_test_instance));

        // Keep processing operations:
        return false;
    }
};

int main(int argc, char** argv)
{
    try
    {
        RunInDetachedThread_Tester tester;

        tester.run_test();
    }
    catch (std::system_error &exc)
    {
        std::cerr << exc.code().message() << std::endl;
        throw;
    }

    return 0;
}