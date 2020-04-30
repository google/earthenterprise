/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __khSpawn_h
#define __khSpawn_h

#include <sys/types.h>
#include <cstdint>
#include <vector>
#include <string>
#include <sstream>
#include <khstrconv.h>


// ****************************************************************************
// ***  For building commandlines in a way that you don't have to worry
// ***  about shell escaping. Each item pushed onto the CmdLine object
// ***  will become one entry in argv[] of the called program.
// ***
// ***  --- example ---
// ***  std::string inname = "/tmp/foo bar.tif";  // <- note the spaces
// ***  std::string outname = "/tmp/foo bar.kip";
// ***  CmdLine cmdline;
// ***  cmdline << "gerasterimport"
// ***          << "--imagery" << inname
// ***          << "-o" << outname;
// ***  if (!cmdline.System()) {
// ***     // handle error
// ***  }
// ****************************************************************************

class CmdLine : public std::vector<std::string> {
 public:

  template <class T>
  CmdLine& operator<<(const T& t) {
    push_back(ToString(t));
    return *this;
  }

  template <class T>
  CmdLine& operator<<(const std::vector<T>& t) {
    for (typename std::vector<T>::const_iterator i = t.begin();
         i != t.end(); ++i) {
      push_back(ToString(*i));
    }
    return *this;
  }

  std::string AsString(void) const;

  // execvp's the cmdline -- shouldn't return
  void Execvp(void) const;

  enum SpawnUser { SpawnAsEffectiveUser, SpawnAsRealUser };

  // forks/execs the cmdline - return child pid or -1 on error
  pid_t Spawn(SpawnUser = SpawnAsEffectiveUser) const;

  // spawns then waits for child to finish.
  // returns true if child wa successful (e.g exit status of 0)
  bool System(SpawnUser = SpawnAsEffectiveUser) const;

};

inline std::string ToString(const CmdLine &cmdline) {
  return cmdline.AsString();
}


// helper function if you do your own waiting
// If rusage parameter is non-null resource usage by process and its children
// information is returned.
extern bool khWaitForPid(pid_t waitfor, bool &success, bool &coredump,
                         int &signum, struct rusage* rusage);




extern std::string gePidFilename(const std::string &procname);
extern std::string geLogFilename(const std::string &procname);

// should be run as root so we can call khPidActive
// This can throw if it cannot read an existing pidfile
extern bool geCheckPidFile(const std::string &procname,
                           const std::string &pidfile = std::string());

// is this pid alive?
// only works for pid that I can signal: I'm root or matching uids
extern bool khPidActive(pid_t pid);
extern bool khKillPid(pid_t pid, bool sigkill = false);
extern std::string khHostname(void);
extern pid_t khPid(void);


#endif /* __khSpawn_h */
