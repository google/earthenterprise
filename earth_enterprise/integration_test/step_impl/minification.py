from getgauge.python import step, before_scenario, Messages
import os, random, fnmatch, subprocess, datetime
from subprocess import Popen
import shutil

# Re-use some of the functionalities
import assets


MISC_XML_PATH = os.path.join(assets.ASSET_ROOT, ".config", "misc.xml")


@step("Turn minification <status>")
def turn_minification(status):
    "Turn off minification in misc.xml."

    if status == "off":
        sedFilter = "s/<UseMinification>1<\/UseMinification>/<UseMinification>0<\/UseMinification>/"

    elif status == "on":
        sedFilter = "s/<UseMinification>0<\/UseMinification>/<UseMinification>1<\/UseMinification>/"

    else:
        raise Exception('Can only turn minification "on" or "off"')

    commandLine = ["sed", "-i", "-e", sedFilter, MISC_XML_PATH]
    assets.call(commandLine, "Failed to set minification status.")
    

def run_and_wait(command):
    "Run command and wait for it to finish."
    pHandle = subprocess.Popen(command, shell=True)
    assert (pHandle.wait() == 0)


@step("Restart fusion")
def restart_fusion():
    "Restart Fusion."
    run_and_wait("sudo service gefusion restart")

    # Checking status here gives fusion enough time to start.
    # Without this call, the next step to use fusion after restarting will get the error:
    # "Fusion Fatal: socket connect '127.0.1.1:13031': Connection refused"
    run_and_wait("sudo service gefusion status")
    

@step("Verify project <first> has more dependencies than project <second>")
def verify_minification(first, second):
    firstDepCount = len(subprocess.check_output(["/opt/google/bin/gequery", "--dependencies", os.path.join(assets.IMAGERY_PROJECT_PATH, first)]).splitlines())
    secondDepCount = len(subprocess.check_output(["/opt/google/bin/gequery", "--dependencies", os.path.join(assets.IMAGERY_PROJECT_PATH, second)]).splitlines())
    assert(firstDepCount > secondDepCount)
