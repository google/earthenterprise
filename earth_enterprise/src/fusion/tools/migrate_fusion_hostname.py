#!/usr/bin/python
#
# TODO insert appropriate copyright info
#
# TODO insert appropriate license info

"""Submitted as part of interview/Github Challenge for
   Thermopylae Google Earth Enterprise Open Source Developer

    by Douglas J. Brantner, December 2018
    https://github.com/float13/earthenterprise

    See also:
        migrate_fusion_hostname_unittest.py

    Forked from:
    https://github.com/google/earthenterprise
"""

"""migrate_fusion_hostname.py

Tool to automate the necessary database changes after changing
the Fusion hostname, to avoid having to delete/re-push/re-publish
existing (already published) GEE databases

Reference:
This tool automates the steps found in "Fusion Issues":
http://www.opengee.org/geedocs/answer/172780.html

The mode must be specified by the user:
   - GEE Server only (no Fusion)
   - Development System (both Fusion and GEE Server)
Both have similar steps with minor differences.

Usage:
- user provides current hostname and the new hostname
   - and optionally, the Server hostname, if different
"""

import os
import sys
import shutil
import argparse
import subprocess
from socket import gethostname    # preferred way to get hostname


# Main To-Do List (see also inline TODO's!):
# TODO remove globals and pass input args dict/namespace as argument
    # to any function that needs them.
    # (this will require a significant restructuring both here
    # and in unit tests which rely on manipulating the globals)
# TODO add user confirmation before deletions, etc. (in progress)
# TODO exit early on function fails/check return values
# TODO write logfile of errors; use logging module?
# TODO make logfile useful for resuming starting after error?
# TODO run pylint

#### GLOBALS: ####

# CONSTANTS:
# All paths should be absolute paths starting with "/" for safety.
_FUSION_DAEMON = "/etc/init.d/gefusion"
_SERVER_DAEMON = "/etc/init.d/geserver"
_PUBLISHED_STREAM_FOLDER = "/gevol/published_dbs/stream_space"
_PUBLISHED_SEARCH_FOLDER = "/gevol/published_dbs/search_space"
# Constants for
# sudo -u gepguser /opt/google/bin/geresetpgdb
_GOOGLE_SCRIPT_FOLDER = "/opt/google/bin"
_RESET_DB_SCRIPT = "geresetpgdb"
_RESET_DB_USER = "gepguser"


# COMMAND LINE ARGUMENT "CONSTANTS"
# (these should only be assigned once after argparse)

# TODO change non-consts to lowercase
_OLD_HOSTNAME = None    # Current hostname of Fusion machine
_NEW_HOSTNAME = None    # New hostname to change to
# TODO replace "MODE" with something like "_HAS_FUSION"?
_MODE = None    # Server-only or Dev/Combo machine (Fusion + Server)
_VALID_MODES = ["SERVER", "DEV"]    # valid values for _MODE
    # SERVER: This is a GEE-Server-only machine
    # DEV: This machine has both Fusion and Server
_OVERRIDE_USER_CONFIRM = False  # skip all user prompts and always
                                # continue with 'yes'
                                # (this should be used carefully!)
_DRYRUN = True  # Test only - Skip all permanent/destructive changes
# TODO change 'dryrun' to 'live mode'? So that program always runs
# as a dryrun by default, and a '--live' flag needs to be passed
# in order to make any real changes to system?

#### End of Globals ####


#### Function Definitions ####

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

    Args:
        None

    Returns:
        TODO

    Raises:
        TODO
    """

    # Shut down the geserver daemon at /etc/init.d/geserver stop.
    daemon_start_stop(_SERVER_DAEMON, "stop")

    # TODO confirm daemons stopped?

    # TODO Update the hostname and IP address to the correct
    # entries for the machine.

    # TODO Edit /opt/google/gehttpd/htdocs/intl/en/tips/tip*.html
    # and update any hardcoded URLs to the new machine URL.
    # TODO What is the format for the * part?
    # does it include hostname? or just a unique id?

    # Remove the contents of
    # /gevol/published_dbs/stream_space
    # and
    # /gevol/published_dbs/search_space.
    if prompt_user_confirm("Deleting published files at %s"
            % _PUBLISHED_STREAM_FOLDER):
        delete_folder_contents(_PUBLISHED_STREAM_FOLDER)
    else:
        exit_early("Files must be deleted before continuing.")

    if prompt_user_confirm("Deleting published files at %s"
            % _PUBLISHED_SEARCH_FOLDER):
        delete_folder_contents(_PUBLISHED_SEARCH_FOLDER)
    else:
        exit_early("Files must be deleted before continuing.")
    # TODO use return codes?

    # TODO Change directory to /tmp
    # and execute
    # sudo -u gepguser /opt/google/bin/geresetpgdb.
    # TODO START HERE. CHECK ABOVE STUFF TOO!

    # Start the GEE Server: /etc/init.d/geserver start.
    daemon_start_stop(_SERVER_DAEMON, "start")

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

    Args:
        None

    Returns:
        TODO

    Raises:
        TODO
    """

    # Shut down both the gefusion and geserver daemons:
    daemon_start_stop(_FUSION_DAEMON, "stop")
    daemon_start_stop(_SERVER_DAEMON, "stop")

    # TODO confirm daemones stopped?

    # TODO Update the hostname and IP address to the correct entries
    # for the machines.

    # TODO If needed, edit the /etc/hosts file to update the
    # IP address
    # and hostname entries for the production machine.

    # TODO Execute /opt/google/bin/geconfigureassetroot --fixmasterhost.

    # TODO Back up the following files to /tmp or another archive folder:
        # TODO /gevol/assets/.config/volumes.xml
        # TODO /gevol/assets/.config/PacketLevel.taskrule
        # TODO /gevol/assets/.userdata/serverAssociations.xml
        # TODO /gevol/assets/.userdata/snippets.xml
    # TODO Replace assets with the name of your asset root.
    # For example, /gevol/tutorial.

    # TODO Edit the /gevol/assets/.userdata/snippets.xml file
    # and
    # TODO update the hardcoded URLs to match the new hostname URL
    # of the production machine.

    # TODO Edit /opt/google/gehttpd/htdocs/intl/en/tips/tip*.html
    # and update any hardcoded URLs to the new machine URL.

    # TODO Remove the contents of /gevol/published_dbs/stream_space
    # TODO and /gevol/published_dbs/search_space.

    # TODO Change directory to /tmp and execute
    # sudo -u gepguser /opt/google/bin/geresetpgdb.

    # Start the geserver and gefusion services:
    daemon_start_stop(_FUSION_DAEMON, "start")
    daemon_start_stop(_SERVER_DAEMON, "start")

    # TODO You can now change the server associations and publish
    # the databases.

    # TODO return something?



def handle_input_args():
    """Parse command line arguments and set global variables."""

    global _MODE
    global _DRYRUN
    global _OLD_HOSTNAME
    global _NEW_HOSTNAME
    global _OVERRIDE_USER_CONFIRM

    parser = argparse.ArgumentParser(
        description="Update GEE Fusion Hostname and fix all existing published GEE databases")

    parser.add_argument(
        "old_hostname",
        help="Required - Current hostname of Fusion machine",
        type=str)
    parser.add_argument(
        "new_hostname",
        help="Required - New hostname for Fusion machine",
        type=str)
    parser.add_argument(
        "--dryrun",
        help="Test only - do not make any actual changes",
        action="store_true")
    parser.add_argument(
        "--override",
        help="Override (skip) ALL user prompts (eg. before deletions). Use this with care!",
        action="store_true")

    # TODO change -s/-d to a single positional argument so help message is more useful
    mode_grp = parser.add_mutually_exclusive_group(required=True)
    mode_grp.add_argument(
        "-s",
        "--server_only",
        help="Use -s if this machine is GEE Server ONLY, and not shared with Fusion (either -s or -d is required)",
        action="store_true")
    mode_grp.add_argument(
        "-d",
        "--dev_machine",
        help="Use -d if this is a Development Machine with both Fusion AND GEE Server together (either -s or -d is required)",
        action="store_true")

    # TODO do we need the Server hostname? maybe good idea for -s mode
    # even if only as a double check...

    args = parser.parse_args()

    # Test and Assign Globals:
    if not args.dryrun:
        _DRYRUN = False

    if args.override:
        _OVERRIDE_USER_CONFIRM = True

    _OLD_HOSTNAME = args.old_hostname

    # TODO test that new hostname is a valid hostname, and not null?
    _NEW_HOSTNAME = args.new_hostname

    # TODO this check might be redundant w/ argparse mutex group
    if args.server_only:
        _MODE = "SERVER"
    elif args.dev_machine:
        _MODE = "DEV"
    else:
        exit_early("Error: Must specify mode (Server/Dev)")
        # TODO add "usage" message?



def daemon_start_stop(daemon, command):
    """Send start/stop command to a daemon service.

    Sends the given command to an existing daemon. Only "start" or
    "stop" are allowed, as stdout output is not checked.

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
        raise ValueError("Invalid arg for 'command': only 'start' and 'stop' are allowed.")


    if _DRYRUN:
        print "dryrun: skipping %s %s" % (daemon, command)
        return 0

    return subproc_check_call_wrapper([daemon, command])


def subproc_check_call_wrapper(arglist):
    """Wrapper for subprocess.check_call, to catch exceptions.

    Calls subprocess.check_call(arglist) and handles/prints
    exception information if necessary.

    Args:
        arglist: list of string arguments passed to
            subprocess.check_call()
            See subprocess documentation for correct formatting
            of the 'args' input variable

    Returns:
        Return code of called function (int)

    Raises:
        -Should not raise any exceptions if used properly.
        -Handles subprocess.CalledProcessError if return code
            of called function is non-zero.
    """

    ret_code = -1

    try:
        ret_code = subprocess.check_call(arglist)
    except subprocess.CalledProcessError as err:
        print "Subprocess Error in %s, exit code: %d" % (arglist[0], err.returncode)
        print err.cmd
        print err.message
        print err.output
        return err.returncode

    return ret_code


def delete_folder_contents(path):
    """Delete entire contents of a folder, but not the actual folder.

    Deletes all files/subfolders contained in folder specified
    by 'path', but leaves the containing folder in place.

    TODO likely requires root? Might fail on permissions issues
    but we assume this program is run as root.

    Globals:
        _DRYRUN: will not actually delete anything if _DRYRUN is True

    Args:
        path: absolute path to folder to delete (string)
            Must begin with "/" to ensure full absolute path.
            e.g. "/gevol/published_dbs/stream_space"

    Returns:
        TODO

    Raises:
        ValueError: If path does not start with "/"
        OSError: on any os/shutil deletion errors

    """

    if path[0] != "/":
        raise(ValueError, "path must be absolute and start with '/'")

    # TODO add try/catch, or let exceptions propagate?
    for f_name in os.listdir(path):
        full_path = os.path.join(path, f_name)
        if os.path.isfile(full_path):
            if not _DRYRUN:
                os.unlink(full_path)
            else:
                print "Dryrun: Skipping file deletion %s" % full_path
        elif os.path.isdir(full_path):
            if not _DRYRUN:
                shutil.rmtree(full_path)
                # TODO add ignore_errors?
                # TODO add onerror?
            else:
                print "Dryrun: Skipping directory deletion %s" % full_path




#### Helper Functions ####

def prompt_user_confirm(explanation, prompt="Continue?"):
    """Prompt user for confirmation before continuing.

    Reads from stdin for user confirmation, with options
    "y/n/yes/no". Only "y" or "yes" result in a True return value.

    User input is NOT case sensitive.

    If global _OVERRIDE_USER_CONFIRM is True, user prompts are
    skipped and this function will always return True. This should
    be used carefully.

    Args:
        explanation: Required - Message to explain to end user
            what they are being asked to confirm (string)

            Example: "13 files will be deleted."

            'explanation' should be short but informative; try to
            limit to 70 chars, or insert newlines manually; otherwise
            terminal will wrap message.

        prompt: Message to print as part of raw_input prompt (string)

            Follows 'explanation' after a newline;
            Should be as short as possible (eg. 1 word)

    Returns:
        True or False, depending on user response.

    Raises:
        TODO
    """

    if _OVERRIDE_USER_CONFIRM:
        return True

    print explanation   # TODO print explanation before override?
    answer = raw_input(prompt + " (y/n/yes/no): ").strip()
    return answer.upper() in ["Y", "YES"]


def exit_early(msg="", errcode=1):
    """Print msg and exit with status code errcode.

    Convenience function to end program with message and error code.
    Message is optional but should be specified for user help/
    troubleshooting.

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
    """Main Function/Control Flow"""

    handle_input_args()

    if os.geteuid() != 0:
        exit_early("Error: Requires Root Permission (try sudo)")

    if _MODE == "DEV":
        if _OLD_HOSTNAME != gethostname():
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
