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
# TODO replace "MODE" with something like "_HAS_FUSION"?
_MODE = None        # Server-only or Dev/Combo machine (Fusion + Server)
_VALID_MODES = ["SERVER", "DEV"]    # valid values for _MODE
    # SERVER: This is a GEE-Server-only machine
    # DEV: This machine has both Fusion and Server
_DRYRUN = True  # Don't make any real changes, just test workflow/permissions


def change_hostname_server_only():
    """Change the hostname for a GEE Server-only machine.

    *** Requires root permissions. ***

    Automates the instructions for changing the GEE Fusion 
    hostname found at:
    http://www.opengee.org/geedocs/answer/172780.html

    Note: This is for GEE Server-only machines.
        For a Development machine (Server + Fusion), use
        change_hostname_dev_machine()

    If global _DRYRUN is true, will test each step but 
    not execute any permanent changes.

    Args: None

    Raises:
        TODO

    Returns:
        TODO
    """

    # Shut down the geserver daemon at /etc/init.d/geserver stop.
    daemon_start_stop(_SERVER_DAEMON, "stop")
    # TODO confirm daemones stopped?

    # TODO Update the hostname and IP address to the correct 
    # entries for the machine.

    # TODO Edit /opt/google/gehttpd/htdocs/intl/en/tips/tip*.html
    # and update any hardcoded URLs to the new machine URL.
    # TODO What is the format for the * part?

    # TODO Remove the contents of /gevol/published_dbs/stream_space
    # and /gevol/published_dbs/search_space.

    # TODO Change directory to /tmp and execute 
    # sudo -u gepguser /opt/google/bin/geresetpgdb.

    # Start the GEE Server: /etc/init.d/geserver start.
    daemon_start_stop("_SERVER_DAEMON", "start")

    # TODO return something?


def change_hostname_dev_machine():
    """Change the hostname for a Development machine
        (contains both Fusion and GEE Server)

    *** Requires root permissions. ***

    Automates the instructions for changing the GEE Fusion 
    hostname found at:
    http://www.opengee.org/geedocs/answer/172780.html

    Note: This is for machines with BOTH GEE Fusion and GEE Server
        For GEE Server-only machines, use
        change_hostname_server_only()

    If global _DRYRUN is true, will test each step but 
    not execute any permanent changes.

    Args: None

    Raises:
        TODO

    Returns:
        TODO
    """

    
    # Shut down both the gefusion and geserver daemons:
    daemon_start_stop(_FUSION_DAEMON, "stop")
    daemon_start_stop(_SERVER_DAEMON, "stop")

    # TODO confirm daemones stopped?

    # TODO Update the hostname and IP address to the correct entries for the machines.

    # TODO If needed, edit the /etc/hosts file to update the IP address and hostname entries for the production machine.

    # TODO Execute /opt/google/bin/geconfigureassetroot --fixmasterhost.

    # TODO Back up the following files to /tmp or another archive folder:
        # TODO /gevol/assets/.config/volumes.xml
        # TODO /gevol/assets/.config/PacketLevel.taskrule
        # TODO /gevol/assets/.userdata/serverAssociations.xml
        # TODO /gevol/assets/.userdata/snippets.xml
    # TODO Replace assets with the name of your asset root. For example, /gevol/tutorial.

    # TODO Edit the /gevol/assets/.userdata/snippets.xml file and update the hardcoded URLs to match the new hostname URL of the production machine.

    # TODO Edit /opt/google/gehttpd/htdocs/intl/en/tips/tip*.html and update any hardcoded URLs to the new machine URL.

    # TODO Remove the contents of /gevol/published_dbs/stream_space and /gevol/published_dbs/search_space.

    # TODO Change directory to /tmp and execute sudo -u gepguser /opt/google/bin/geresetpgdb.

    # Start the geserver and gefusion services:
    daemon_start_stop(_FUSION_DAEMON, "start")
    daemon_start_stop(_SERVER_DAEMON, "start")

    # TODO You can now change the server associations and publish the databases.

    # TODO return something?



def handle_input_args():
    """Parse command line arguments and set global variables."""

    global _MODE, _DRYRUN, _OLD_HOSTNAME, _NEW_HOSTNAME

    parser = argparse.ArgumentParser(description="Update GEE Fusion Hostname andfix all existing published GEE databases")

    parser.add_argument("old_hostname", help="Required - Current hostname of Fusion machine", type=str)
    parser.add_argument("new_hostname", help="Required - New hostname for Fusion machine", type=str)
    parser.add_argument("--dryrun", help="Test only - do not make any actual changes", action="store_true")
    # TODO set defaults?

    # TODO is mutex group required or do we need to specify that??
    # TODO necessary to do default=False for store_trues?
    mode_grp = parser.add_mutually_exclusive_group()
    mode_grp.add_argument("-s", "--server_only", help="Use -s if this machine is GEE Server ONLY, and not shared with Fusion", action="store_true")
    mode_grp.add_argument("-c", "--dev_machine", help="Use -d if this is a Development Machine (Fusion AND GEE Server combined)", action="store_true")

    # TODO do we need the Server hostname? maybe good idea for -s mode even if only as a double check...
    args = parser.parse_args()

    if not args.dryrun:
        _DRYRUN = False

    # Test and Assign Globals:
    _OLD_HOSTNAME = args.old_hostname

    # TODO test that new hostname is a valid hostname, and not null?
    _NEW_HOSTNAME = args.new_hostname

    if args.server_only:
        _MODE = "SERVER"
    elif args.dev_machine:
        _MODE = "DEV"
    else:
        exit_early("Error: Must specify mode (Server/Dev)")    
        # TODO add "usage" message?




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

    if _DRYRUN:
        print "dryrun: skipping %s %s" % (daemon, command)
        return 0

    try:
        subprocess.check_call([daemon, command])
    except subprocess.CalledProcessError as e:
        print "Subprocess Error in %s, exit code: %d" % (daemon, e.returncode)
        print e.cmd
        print e.message
        print e.output
        return e.returncode

    return 0


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


def main():
    handle_input_args()

    # TODO check for root permission, if necessary?

    if _MODE == "DEV":
        if (_OLD_HOSTNAME != gethostname()):
            exit_early("Error: Incorrect current hostname - please double check and retry.")
        else:
            print "Old hostname matches... ok!"

        # TODO any more checks?

        change_hostname_dev_machine()

    elif _MODE == "SERVER":
        # TODO check if in SERVER mode if hostname matches (need new input arg)

        change_hostname_server_only()

    else:
        # TODO raise exception instead?
        exit_early("Error: Invalid mode option.")

    # TODO print further instructions on changing server association?
    # To change the server associations and publish the databases...

    # TODO print extra helpful info (how to test, etc.)
    # TODO print link to issue/help


if __name__ == "__main__":
    main()
