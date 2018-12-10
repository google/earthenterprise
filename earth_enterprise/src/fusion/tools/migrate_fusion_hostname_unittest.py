#!/usr/bin/python

# TODO docstring?

import unittest
import subprocess
from socket import gethostname
import migrate_fusion_hostname as mfh   # Module to Test

class TestMigrateFusionHostname(unittest.TestCase):

    # TODO add class variable for current hostname?
    # this complicates things for testing...

    _PYTHON_RUNTIME = "python"
    _TEST_PROGRAM = "migrate_fusion_hostname.py"
    _NEW_HOSTNAME = "newtesthost"
    _CURRENT_HOSTNAME = None   # get hostname of THIS machine at runtime
    _BASE_ARG_LIST = None   # Prefix for all full-program test
                            # function calls (must include --dryrun)
                            # for safety

    """Setup/Teardown"""
    @classmethod
    def setUpClass(cls):
        """Runs ONLY once at beginning."""

        cls._CURRENT_HOSTNAME = gethostname()

    @classmethod
    def tearDownClass(cls):
        """Runs ONLY once at end."""
        pass

    def setUp(self):
        """Runs once before every test."""
        
        # Reset basic arguments with --dryrun 
        # for safety before each test
        self._BASE_ARG_LIST = [self._PYTHON_RUNTIME,
                               self._TEST_PROGRAM,
                               "--dryrun",
                               ]

    def tearDown(self):
        """Runs once after every test."""
        pass


    #### Full Program Tests ####
    
    # All of these should be run with the --dryrun flag

    def test_full_program_dryrun_server_mode(self):
        """Run tests of full program in DRYRUN mode.

        Dryrun mode avoids any permanent changes to the host computer.

        Successful test runs should exit with code 0.
        """

        # TODO test bad commandline args, etc.
        
        my_args = ["--server_only",
                   self._CURRENT_HOSTNAME,
                   self._NEW_HOSTNAME,
                  ]
        full_args = self._BASE_ARG_LIST + my_args
        self.assertEquals(0, subprocess.check_call(full_args))

    #def test_full_program_dryrun_dev_mode(self):
    # TODO implement this

    def test_full_program_missing_mode_arg(self):
        my_args = [ # omitting the --server_only arg on purpose
                   self._CURRENT_HOSTNAME,
                   self._NEW_HOSTNAME,
                  ]
        full_args = self._BASE_ARG_LIST + my_args

        with self.assertRaises(subprocess.CalledProcessError):
            subprocess.check_call(full_args)
            # TODO check actual exit code? would have to catch, parse,
            # and then re-throw exception...




    ### Individual Method Tests ####

    # TODO doesn't work with dryrun mode...
    #def test_daemon_start_stop_bad_daemon(self):
    #    """Test Bad Input for daemon name."""
    #    with self.assertRaises(OSError):
    #        mfh.daemon_start_stop("/etc/init.d/fake_daemon_asdf", "start")

    def test_daemon_start_stop_bad_daemon(self):
        """Test Bad Input for command."""
        with self.assertRaises(ValueError):
            mfh.daemon_start_stop("/etc/init.d/gefusion", None)
            mfh.daemon_start_stop("/etc/init.d/gefusion", 1)
            mfh.daemon_start_stop("/etc/init.d/gefusion", "")
            mfh.daemon_start_stop("/etc/init.d/gefusion", "asdf")


            

if __name__ == "__main__":
    unittest.main()
