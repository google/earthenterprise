#!/usr/bin/python

"""Submitted as part of interview/Github Challenge for
   Thermopylae Google Earth Enterprise Open Source Developer

    by Douglas J. Brantner, December 2018
    https://github.com/float13/earthenterprise

    See also:
        migrate_fusion_hostname.py
        migrate_fusion_hostname_unittest.py

    Forked from:
    https://github.com/google/earthenterprise
"""

"""DESTRUCTIVE UNIT TESTS for migrate_fusion_hostname.py

    WARNING: Do NOT run these tests on a production machine.

    These tests WILL MAKE PERMANENT CHANGES, DELETE FILES, ETC.
    For non-destructive safe tests, see:
        migrate_fusion_hostname_unittest.py
    and note the use of the --dryrun flag.

    WARNING: Writes and deletes some temporary files. Uses default
        temp file directory as per Python tempfile specification:
        https://docs.python.org/2/library/tempfile.html#tempfile.mkstemp

        See the above link for changing default temp file directory.

    Usage:
        Most unit tests can be run as default user.

        However, there are also some tests which require root
        permission and must be run with sudo; otherwise
        they are skipped.

        Run as:
        python migrate_fusion_hostname_destructive_tests.py
            or optionally with sudo to run all tests.

        (Using Python 2.7)
    """

import unittest
import os
import subprocess
import tempfile
from socket import gethostname
import migrate_fusion_hostname as mfh   # Module to Test

# Global Variables
# TODO use argparse to set these?
# TODO delete this:
#_ALLOW_DAEMON_STARTSTOP = True      # this only applies to module tests
                                    # to allow starting/stopping daemons
                                    # and should only be used for
                                    # @unittest.skip... decorators
                                    # DO NOT rely on this for safety.
                                    # Only the -dryrun flag is safe.

# TODO implement _ALLOW_DRYRUN_OVERRIDE checks!
# _ALLOW_DRYRUN_OVERRIDE = True     # Enable direct manipulation of
                                    # mfh._DRYRUN flag for tests
                                    # This should be reset in setUp and/or
                                    # tearDown for safety.

# TODO run full tests with --override flag?




# Helper Functions:

def check_root():
    """Check for root permission.

    Returns:
        True if this unittest module is run as root,
        otherwise returns False.
    """
    return os.geteuid() == 0

# Unit Tests (WARNING - Will cause permanent changes to system!)

class TestMigrateFusionHostname(unittest.TestCase):
    """Unit tests for migrate_fusion_hostname.py

    These tests should NOT be run on a production machine
    and are all run in -dryrun mode for safety.

    Perhaps a separate test class could be written for live tests,
    with user confirmation (eg. command line flag).
    """

    # TODO add class variable for current hostname?
    # this complicates things for testing...

    _PYTHON_RUNTIME = "python"
    _TEST_PROGRAM = "migrate_fusion_hostname.py"
    _NEW_HOSTNAME = "newtesthost"
    # TODO might not need _CURRENT_HOSTNAME - just get as part of each test?
    _CURRENT_HOSTNAME = None   # get hostname of THIS machine at runtime
    _base_arg_list = None   # Prefix for all full-program test
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

        # Reset internal _DRYRUN variable just to be safe
        # NOTE: some tests may change this (eg. file deletion)
        # (for individual module/function tests)
        mfh._DRYRUN = True

        # Reset _OVERRIDE_USER_CONFIRM flag
        mfh._OVERRIDE_USER_CONFIRM = False

        # Reset basic arguments with --dryrun
        # for safety before each test
        # (for full-program tests called w/ subprocess)
        self._base_arg_list = [self._PYTHON_RUNTIME,
                               self._TEST_PROGRAM,
                               "--dryrun",
                              ]

    def tearDown(self):
        """Runs once after every test."""

        mfh._DRYRUN = True  # double check just for safety


    #### Individual Method Tests ####

    # TODO move test_daemon_start_stop_live_test here from _unittest?

    def test_change_hostname(self):
        """Test & confirm hostname change, then revert to original."""

        old_hostname = gethostname()
        new_hostname = "testhost123"

        mfh._DRYRUN = False
        self.assertEqual(mfh.change_hostname(new_hostname), 0)
        self.assertEqual(gethostname(), new_hostname)

        # change it back & test again
        self.assertEqual(mfh.change_hostname(old_hostname), 0)
        self.assertEqual(gethostname(), old_hostname)





if __name__ == "__main__":
    unittest.main(verbosity=2)
