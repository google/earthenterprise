#!/usr/bin/python

"""Submitted as part of interview/Github Challenge for
   Thermopylae Google Earth Enterprise Open Source Developer

    by Douglas J. Brantner, December 2018
    https://github.com/float13/earthenterprise

    See also:
        migrate_fusion_hostname.py

    Forked from:
    https://github.com/google/earthenterprise
"""

"""Unit tests for migrate_fusion_hostname.py

    WARNING: Do NOT run these tests on a production machine.
        All full-program unit tests are run with -dryrun flag
        to avoid destructive behavior, but just to be safe,
        ONLY RUN THIS ON A TEST MACHINE!

        Some individual tests may change _DRYRUN to False,
        but this should be used very carefully and only on
        very granular cases, using disposable temporary files, etc.

        Both setUp and tearDown methods reset
            _DRYRUN = True
        for safety.

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
        python migrate_fusion_hostname_unittest.py
            or optionally with sudo to run all tests.
    """

import unittest
import os
import subprocess
import tempfile
from socket import gethostname
import migrate_fusion_hostname as mfh   # Module to Test

# Global Variables
# TODO use argparse to set these?
_ALLOW_DAEMON_STARTSTOP = True      # this only applies to module tests
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

# TODO find a way to run full/destructive tests on a disposable VM



# Helper Functions:

def check_root():
    """Check for root permission.

    Returns:
        True if this unittest module is run as root,
        otherwise returns False.
    """
    return os.geteuid() == 0

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


    #### Full Program Dry-Run Tests ####
    # All of these should be run with the --dryrun flag
    # to avoid deleting files or stopping services.

    # These are also run with the --override flag to avoid
    # stopping for user input. (only safe with --dryrun flag!)

    # These tests all use the class variable '_base_arg_list'
    # which includes the --dryrun flag implicitly.


    @unittest.skipUnless(check_root(), "Requires root.")
    def test_full_program_dryrun_server_mode(self):
        """Run tests of full program in DRYRUN mode.

        Dryrun mode avoids any permanent changes to the host computer.

        Successful test runs should exit with code 0.
        """

        # TODO test bad commandline args, etc.

        my_args = ["--override",
                   "--server_only",
                   self._CURRENT_HOSTNAME,
                   self._NEW_HOSTNAME,
                  ]
        full_args = self._base_arg_list + my_args
        self.assertEquals(0, subprocess.check_call(full_args))


    #def test_full_program_dryrun_dev_mode(self):
        # TODO implement this


    def test_full_program_missing_mode_arg(self):
        """Test argparse for missing arg.
            (omitting the --server_only arg on purpose)
            also omitting --override, this should fail before
                it hits a user prompt.
        """
        my_args = [self._CURRENT_HOSTNAME,
                   self._NEW_HOSTNAME,
                  ]
        full_args = self._base_arg_list + my_args

        with self.assertRaises(subprocess.CalledProcessError):
            subprocess.check_call(full_args)
            # TODO check actual exit code? would have to catch, parse,
            # and then re-throw exception...




    ### Individual Method Tests ####

    def test_change_hostname_dryrun(self):
        """Check that hostname doesn't change in dryrun mode
        and that return value is 0.
        """
        self.assertEqual(0, mfh.change_hostname("fakehostname"))
        self.assertEqual(self._CURRENT_HOSTNAME, gethostname())

    def test_daemon_start_stop_bad_daemon(self):
        """Test Bad Input for daemon name."""
        mfh._DRYRUN = False
        with self.assertRaises(OSError):
            mfh.daemon_start_stop("/etc/init.d/fake_daemon_asdf", "start")


    def test_daemon_start_stop_bad_command(self):
        """Test Bad Input for command.

        Should not require root since it should fail much earlier,
        likewise should not require daemon to exist because it should
        fail before the subprocess is called.
        """
        with self.assertRaises(ValueError):
            mfh.daemon_start_stop(mfh._SERVER_DAEMON, None)
            mfh.daemon_start_stop(mfh._SERVER_DAEMON, 1)
            mfh.daemon_start_stop(mfh._SERVER_DAEMON, "")
            mfh.daemon_start_stop(mfh._SERVER_DAEMON, "asdf")


    @unittest.skipUnless(check_root(), "Requires root.")
    @unittest.skipUnless(os.path.isfile(mfh._SERVER_DAEMON), "Missing GEE Server Daemon")
    @unittest.skipUnless(_ALLOW_DAEMON_STARTSTOP, "Must enable _ALLOW_DAEMON_STARTSTOP")
    def test_daemon_start_stop_live_test(self):
        """Actually stop and start a service.

        WARNING this will affect actual services running

        Requires _ALLOW_DAEMON_STARTSTOP to be True
        """
        # TODO check if server is running first
        # TODO make sure we return it to its original state? maybe in tearDownClass() ?
        self.assertEqual(
            mfh.daemon_start_stop(mfh._SERVER_DAEMON, "stop"),
            0)

        # TODO check that server stopped

        self.assertEqual(
            mfh.daemon_start_stop(mfh._SERVER_DAEMON, "start"),
            0)


    # TODO tests for update_htdocs_urls()


    def test_delete_folder_contents_startsWithSlash(self):
        """Omitting leading slash should throw ValueError."""
        with self.assertRaises(ValueError):
            mfh.delete_folder_contents("tmp/asdffakedir123")


    def test_delete_folder_contents_missingDir(self):
        """Correct leading slash but missing dir should throw OSError."""
        with self.assertRaises(OSError):
            mfh.delete_folder_contents("/tmp/asdffakedir123")


    def test_delete_folder_contents_actual_delete(self):
        """Create some fake tmp files/folders and delete them.

            WARNING: Creates and deletes some temp files.

            You may wish to change the default temp directory:
            (see Python documentation for 'tempfile' module)
        """

        tmpdir = tempfile.mkdtemp(prefix="mfh_test_delete_folder")
        fake_folders = ["testdir1", "testdir2", "testdir3"]
        fake_files = ["file1.txt", "file2.txt", "file3.txt"]

        # Create temporary file tree
        # Create files in top-level directory
        for f_name in fake_files:
            with open(os.path.join(tmpdir, f_name), 'w') as my_file:
                my_file.write("testwords1234567890")
        # Create subfolders and files within them
        for dir_name in fake_folders:
            my_dir = os.path.join(tmpdir, dir_name)
            os.mkdir(my_dir)
            for f_name in fake_files:
                with open(os.path.join(my_dir, f_name), 'w') as my_file:
                    my_file.write("testwords1234567890")

        # TODO try/catch file creation errors?

        # verify fake files exist
        n_files = len(os.listdir(tmpdir))
        self.assertGreater(n_files, 0)

        # make sure files are NOT deleted in _DRYRUN mode:
        mfh.delete_folder_contents(tmpdir)
        self.assertEqual(len(os.listdir(tmpdir)), n_files)

        # switch to live mode
        mfh._DRYRUN = False

        # delete the files for real this time
        mfh.delete_folder_contents(tmpdir)

        # now, tmpdir should still exist but it should be empty
        self.assertTrue(os.path.isdir(tmpdir))
        self.assertEqual(len(os.listdir(tmpdir)), 0)

        # clean up:
        os.rmdir(tmpdir)

    def test_run_geresetpgdb_dryrun(self):
        """Check return value in dryrun mode."""
        self.assertEqual(0, mfh.run_geresetpgdb())

        # TODO other tests for run_geresetpgdb? Anything else requires
        # running actual script (not safe)

    # TODO for prompt_user_confirm(...) testing actual user input
    # requires unittest.mocks 
    # which is not part of Python 2.7 standard library
    # These should be tested but would introduce extra dependencies.
    def test_prompt_user_confirm_override(self):
        """Test that override returns true."""

        mfh._OVERRIDE_USER_CONFIRM = True
        self.assertTrue(mfh.prompt_user_confirm("fake user prompt"))

if __name__ == "__main__":
    unittest.main(verbosity=2)
