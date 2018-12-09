#!/usr/bin/python
#
# TODO insert appropriate copyright info
#
# TODO insert license info

# 
# Tool to automate the necessary database changes after changing 
# the Fusion hostname, to avoid having to delete/re-push/re-publish 
# existing (already published) GEE databases
#
# Reference:
# This tool automates the steps found in "Fusion Issues":
# http://www.opengee.org/geedocs/answer/172780.html
# 
# The mode must be specified by the user:
#    - GEE Server only (no Fusion)
#    - Development System (both Fusion and GEE Server)
# Both have similar steps with minor differences.
#
# Usage:
# - user provides current hostname and the new hostname
#    - and optionally, the Server hostname, if different
# 

import os
import sys
import argparse
import subprocess
from socket import gethostname    # preferred way to get hostname


# TODO fix tabs/spaces, use pylint to check

# GLOBALS:
# Internal Globals:
# Constants:
_FUSION_DAEMON = "/etc/init.d/gefusion"
_SERVER_DAEMON = "/etc/init.d/geserver"


# TODO change non-consts to lowercase
_OLD_HOSTNAME = None    # Current hostname of Fusion machine
_NEW_HOSTNAME = None    # New hostname to change to
_MODE = None        # Server-only or Dev/Combo machine (Fusion + Server)
# TODO replace "MODE" with something like "_HAS_FUSION"?
_DRYRUN = False        # Don't make any real changes, just test workflow/permissions


def exit_early(msg="", errcode=1):
    """Print msg and exit with status code errcode.
    
    Convenience function to exit early. Set msg with useful error message
    if exiting due to an error/exception.
    Change errcode to 0 if exiting normally.

    Args:
        msg: Optional error message (string)
        errcode: Status code to exit with (int)
    """

    if msg:
        print msg
    print "Exiting..."
    sys.exit(errcode)

def handle_input_args():
    """Parse command line arguments and set global variables."""

    global _MODE, _DRYRUN
    parser = argparse.ArgumentParser(description="Update GEE Fusion Hostname andfix all existing published GEE databases")

    parser.add_argument("old_hostname", help="Required - Current hostname of Fusion machine", type=str)
    parser.add_argument("new_hostname", help="Required - New hostname for Fusion machine", type=str)
    parser.add_argument("--dryrun", help="Test only - do not make any actual changes", action="store_true")

    # TODO is mutex group required or do we need to specify that??
    # TODO necessary to do default=False for store_trues?
    mode_grp = parser.add_mutually_exclusive_group()
    mode_grp.add_argument("-s", "--server_only", help="Use -s if this machine is GEE Server ONLY, and not shared with Fusion", action="store_true")
    mode_grp.add_argument("-c", "--combo_machine", help="Use -d if this is a development machine (Fusion AND GEE Server combined)", action="store_true")

    # TODO do we need the Server hostname? maybe good idea for -s mode even if only as a double check...
    args = parser.parse_args()

    # Test and Assign Globals:

    _OLD_HOSTNAME = args.old_hostname

    # TODO test that new hostname is a valid hostname, and not null?
    _NEW_HOSTNAME = args.new_hostname

    if args.server_only:
        _MODE = "SERVER"
    elif args.combo_machine:
        _MODE = "DEV"
    # else, it stays None
    # TODO quit early if mode is not specified


def daemon_start_stop(daemon, command):
    """Send start/stop command to a daemon service.

    Sends the given command to an existing daemon.
    Only "start" or "stop" are allowed, as stdout output is not checked.

    *** Requires root privileges. ***

    Args:
        daemon: the full path to an existing system daemon (string)
            eg. "/etc/init.d/gefusion"
        command: either "start" or "stop" (string)

    Returns:
        Exit code of daemon call. (int)

    Raises:
        ValueError: if given an invalid input for "command"
        OSError: If "daemon" path is invalid or doesn't exist
    """
    # TODO raise error on lack of root privelige?
    # TODO is service ok?

    if command not in ["start", "stop"]:
        raise ValueError("Invalid argument for 'command': only 'start' and 'stop' are allowed.")
    try:
        subprocess.check_call([daemon, command])
    except subprocess.CalledProcessError as e:
        print "Subprocess Error in %s, exit code: %d" % (daemon, e.returncode)
        print e.cmd
        print e.message
        print e.output
        return e.returncode

    return 0


def main():
    handle_input_args()

    # Test for valid inputs:
    # TODO move to separate function?
    if _MODE is None:
        exit_early("Error: Must specify mode (Server/Dev)")    # TODO add "usage" message?

    # TODO check for root permission, if necessary?

    if _MODE == "DEV":
        if (_OLD_HOSTNAME != gethostname()):
            exit_early("Error: Incorrect current hostname - please double check and retry.")
        else:
            print "Old hostname matches... ok!"
    # TODO check if in SERVER mode if hostname matches (need new input arg)


    # **** Stop Daemons ****
    # this requires root
    #daemon_start_stop(_SERVER_DAEMON, "stop")
    #if _MODE == "DEV":
    #    daemon_start_stop(_FUSION_DAEMON, "stop")


if __name__ == "__main__":
    main()
