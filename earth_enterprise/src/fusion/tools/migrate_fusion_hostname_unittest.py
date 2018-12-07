#!/usr/bin/python

# TODO docstring?
# TODO fix tabs/spaces

import unittest
#import subprocess
import migrate_fusion_hostname as mfh

class TestMigrateFusionHostname(unittest.TestCase):

	"""Setup/Teardown"""
	@classmethod
	def setUpClass(cls):
		"""Runs ONLY once at beginning."""
		pass

	@classmethod
	def tearDownClass(cls):
		"""Runs ONLY once at end."""
		pass

	def setUp(self):
		"""Runs once before every test."""
		pass

	def tearDown(self):
		"""Runs once after every test."""


	def testFullProgram(self):
		# TODO big tests of full program as subprocess calls
		# TODO test bad input, etc.
		# *** run all of these ase DRYRUNS!
		pass


# TODO split into smaller functions (1 per test?)
	def testDaemonStartStop(self):
		# Test Bad Input for daemon:
		with self.assertRaises(OSError):
			mfh.daemon_start_stop("/etc/init.d/fake_daemon_asdf", "start")

		# Test Bad Input for command:
		with self.assertRaises(ValueError):
			mfh.daemon_start_stop("/etc/init.d/gefusion", None)
			mfh.daemon_start_stop("/etc/init.d/gefusion", 1)
			mfh.daemon_start_stop("/etc/init.d/gefusion", "")
			mfh.daemon_start_stop("/etc/init.d/gefusion", "asdf")


			

if __name__ == "__main__":
	unittest.main()
