#!/usr/bin/python

# TODO docstring?
# TODO fix tabs/spaces

import unittest
import migrate_fusion_hostname

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


	def testDaemonStartStop(self):
		with self.assertRaises(subprocess.CalledProcessError):

			# TODO test bad start/stop input (custom exception?)

			daemon_start_stop("/etc/init.d/fake_daemon_asdf", "start"))
			
