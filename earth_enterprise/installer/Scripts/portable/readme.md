#
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

Portable Build Process
======================
This is a unified codebase for generating both the Mac and Windows installers.

Overview
--------
The process for build is as follows:
1.  The pulse build server (fusionbuilder) kicks off pulse_build.py.

    This takes an argument of "mac" or "win" and will gather up the files from
    the source needed for a build machine to run the build. This file gets 
    hosted on the GE Server web server on fusionbuilder at 
    http://fusionbuilder/builds/[build_id].tar.gz.
    
    pulse_build.py then calls the corresponding build server at
    http://[...]/build.cgi?id=[build_id]
    
    pulse_build.py then sleeps and checks over a period of five minutes looking
    for the finished build to be hosted by build machine. If the build finishes
    it copies the completed build back up to /usr/local/google/gepackager/dist.
    Otherwise it errors and copies up the log file from the build machine if
    there is one.

2.  build.cgi runs in a web server on each build machine. It accepts the
    request and given build_id and downloads the archive from pulse and kicks 
    off the build on the local machine. Once the build is done (or error) it 
    copies the completed artifact and logs to 
    http://[...]/builds/[build_id]/...tar.gz (.exe for Windows).

3.  pulse_build.py picks up the artifact as described above.

NOTE: The generated archives on the pulse machine and the completed artifacts
are NOT cleaned up automatically. This needs to be done periodically by hand.

Setting Up the Build Web Server
-------------------------------
The build machine needs an Apache (or similar) web server setup to allow CGI
execution on the document root. To do this in Apache:
1.  Edit the config to include a line for AddHandler cgi-script .cgi
2.  Edit the config sections that reference the directory root and add ExecCGI
    to Options.
3.  Create a folder called builds in the directory root and make sure it is
    read and write for the web server.
4.  Copy build.cgi, build.py and build_helper.py to the directory root and make 
    sure they have execute permissions.
5.  Modify firewall issues on your machine as needed.

Notes for Windows
*   You will need to install 32bit Python using the installer from 
    http://python.org.
*   For Apache you will need to add the following lines to the config as well.
    ScriptInterpreterSource registry
    PassEnv PYTHONPATH
    SetEnv PYTHONUNBUFFERED 1
*   You will need to add a registry entry for Windows to recognize .cgi as a 
    Python script. Open regedit and look at
    HKEY_LOCAL_MACHINE\SOFTWARE\Classes\.py and create a matching entry called
    .cgi in the same location.
*   You also need to install some kind of command line unzip program (recommend
    7-zip).
