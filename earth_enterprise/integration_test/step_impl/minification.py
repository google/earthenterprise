from getgauge.python import step, before_scenario, Messages
import os, random, fnmatch, subprocess, datetime
from subprocess import Popen
import shutil

from xml.etree.ElementTree import parse

# Re-use some of the functionalities
import assets


MISC_XML_PATH = os.path.join(assets.ASSET_ROOT, ".config", "misc.xml")

@step("Turn minification <status>")
def turn_minification(status):
    "Turn off minification in misc.xml."

    newStatus = {"off": "0",
                 "on":  "1"}[status]

    miscTree = parse(MISC_XML_PATH)

    for config in miscTree.iter('MiscConfigStorage'):
        minificationCount = len(list(config.iter('UseMinification')))
        if minificationCount == 0:
            minification = ElementTree.SubElement(config, 'UseMinification')
            minification.text = newStatus
        else:
            for useMin in config.iter('UseMinification'):
                useMin.text = newStatus

    miscTree.write(MISC_XML_PATH)
    

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
    print "firstDepCount = {} secondDepCount = {}".format(firstDepCount, secondDepCount)
    assert(firstDepCount == 119)
    assert(secondDepCount == 101)
    assert(firstDepCount > secondDepCount)
