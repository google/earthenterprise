// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include "khGetopt.h"

#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <khstl.h>
#include "khSimpleException.h"
#include "khFileUtils.h"


// class - static
std::string khGetopt::ProgName = std::string();
std::string khGetopt::FullProgName = std::string();

// ****************************************************************************
// ***  validators
// ****************************************************************************
void
khGetopt::FileExists(const std::string &fname)
{
  if (!khIsURI(fname)) {
    if (!khExists(fname)) {
      throw khSimpleErrnoException(""); // will add str version of errno
    }
  }
}

void
khGetopt::DirExists(const std::string &dname)
{
  if (!khDirExists(dname)) {
    if (errno == 0) {
      // no error - so stat worked, just not a dir
      throw khSimpleException("Not a directory");
    } else {
      throw khSimpleErrnoException(""); // will add str version of errno
    }
  }
}

namespace {
// This class is to make sure that optind gets initialized for making getopt
// reentrant.
class InitializeOptindGuard {
 public:
  InitializeOptindGuard() {}

  ~InitializeOptindGuard() {
    optind = 0;
  }
};
}  // end namespace


bool
khGetopt::processAll(int argc, char *argv[], int &argn) throw()
{
  SetProgName(argv[0]);

  // generate the option structures for getopt_long_only
  struct option options[opts.size()+1];
  for (unsigned int i = 0; i < opts.size(); ++i) {
    options[i].name    = opts[i]->name.c_str();
    options[i].has_arg = opts[i]->hasarg;
    options[i].flag    = 0;
    options[i].val     = 0; // set all of them to 0, we'll use longindex
  }
  unsigned int numopt = opts.size();
  options[numopt].name    = 0;
  options[numopt].has_arg = 0;
  options[numopt].flag    = 0;
  options[numopt].val     = 0;


  int longindex = 0;;

  InitializeOptindGuard optind_guard;

  while (1) {
    int ret = getopt_long_only(argc, argv, posixlyCorrect ? "+" : "",
                               options, &longindex);
    switch (ret) {
      case -1:
        argn = optind;
        return CheckConstraints();
      case 0:
        if ((longindex >= 0) && (longindex < (int)numopt)) {
          try {
            HandleArg(longindex, optarg);
          } catch (const std::exception &e) {
            fprintf(stderr, "Error processing option %s = %s:\n\t%s\n",
                    options[longindex].name, optarg, e.what());
            return false;
          } catch (...) {
            fprintf(stderr,
                    "Unknown error processing option %s = %s\n",
                    options[longindex].name, optarg);
            return false;
          }
        } else {
          fprintf(stderr, "Internal error: Unexpected longindex returned from getopt %d\n", longindex);
        }
        break;
      case '?':
        // error message was already printed
        return false;
      default:
        fprintf(stderr, "Internal error: Unexpected return value from getopt %d\n", ret);
        return false;
    }
  }

  return false; // shouldn't get here
}


bool
khGetopt::processOptionsOnly(int argc, char *argv[], const char *&nextarg)
{
  SetProgName(argv[0]);

  // we've already ran across a '--' and are done looking for options
  // just keep handing out the rest of the arguments until they are gone
  if (someDoneWithArgs) {
    nextarg = (++optind < argc) ? argv[optind] : 0;
    return true;
  }


  // generate the option structures for getopt_long_only
  struct option options[opts.size()+1];
  for (unsigned int i = 0; i < opts.size(); ++i) {
    options[i].name    = opts[i]->name.c_str();
    options[i].has_arg = opts[i]->hasarg;
    options[i].flag    = 0;
    options[i].val     = 0;
  }
  unsigned int numopt = opts.size();
  options[numopt].name    = 0;
  options[numopt].has_arg = 0;
  options[numopt].flag    = 0;
  options[numopt].val     = 0;


  int longindex = 0;;
  while (1) {
    // "-" -> tells get opt that I want to hear about arguments
    // intermixed with options
    int ret = getopt_long_only(argc, argv, "-", options, &longindex);
    switch (ret) {
      case -1:
        // argument processing is finished
        someDoneWithArgs = true;
        nextarg = (optind < argc) ? argv[optind] : 0;
        return true;
      case 1:
        // we've run across an arg in the middle of the options
        nextarg = optarg;
        return true;
      case 0:
        if ((longindex >= 0) && (longindex < (int)numopt)) {
          try {
            HandleArg(longindex, optarg);
          } catch (const std::exception &e) {
            fprintf(stderr, "Error processing option %s = %s: %s\n",
                    options[longindex].name, optarg, e.what());
            return false;
          } catch (...) {
            fprintf(stderr,
                    "Unknown error processing option %s = %s\n",
                    options[longindex].name, optarg);
            return false;
          }
        } else {
          fprintf(stderr, "Internal error: Unexpected longindex returned from getopt %d\n", longindex);
        }
        break;
      case '?':
        // error message was already printed
        return false;
      default:
        fprintf(stderr, "Internal error: Unexpected return value from getopt %d\n", ret);
        return false;
    }
  }

  return false; // shouldn't get here
}


void khGetopt::setExclusive(const std::set<std::string> &set) {
  constraints_.push_back(Constraint(set, false /* required */));
}

void khGetopt::setExclusiveRequired(const std::set<std::string> &set) {
  constraints_.push_back(Constraint(set, true /* required */));
}

void khGetopt::setRequired(const std::set<std::string> &set) {
  for (std::set<std::string>::const_iterator name = set.begin();
       name != set.end(); ++name) {
    constraints_.push_back(Constraint(makeset(*name),
                                      true /* required */));
  }
}

khGetopt::khGetopt(void) :
    someDoneWithArgs(false),
    posixlyCorrect(false),
    constraints_(),
    used_options_(),
    used_help_(false)
{
}

khGetopt::~khGetopt(void) {
  for (std::vector<OptBase*>::iterator i = opts.begin();
       i != opts.end(); ++i) {
    delete *i;
  }
}


void khGetopt::HandleArg(int index, const char *arg) {
  OptBase *opt = opts[index];

  opt->handleArg(arg);

  used_options_.insert(opt->name);

  if ((dynamic_cast<HelpOption*>(opt) != 0) ||
      (opt->name == "help") ||
      (opt->name == "?")) {
    used_help_ = true;
  }
}

bool khGetopt::CheckConstraints(void) {
  if (used_help_) {
    return true;
  }

  bool result = true;
  for (std::vector<Constraint>::const_iterator constraint =
         constraints_.begin(); constraint != constraints_.end();
       ++constraint) {
    unsigned int count = 0;
    for (std::set<std::string>::const_iterator name = constraint->set_.begin();
         name != constraint->set_.end(); ++name) {
      if (used_options_.find(*name) != used_options_.end()) {
        ++count;
      }
    }

    if (count == 0) {
      if (constraint->required_) {
        if (constraint->set_.size() == 1) {
          fprintf(stderr, "Error: --%s must be specified\n",
                  constraint->set_.begin()->c_str());
          result = false;
        } else {
          fprintf(stderr,
                  "Error: One of the following options must be specified:\n");
          for (std::set<std::string>::const_iterator name =
                 constraint->set_.begin(); name != constraint->set_.end();
               ++name) {
            fprintf(stderr, "\t--%s\n", name->c_str());
          }
          result = false;
        }
      }
    } else if (count > 1) {
      fprintf(stderr,
              "Error: The following options are exclusive:\n");
      for (std::set<std::string>::const_iterator name =
             constraint->set_.begin(); name != constraint->set_.end();
           ++name) {
        fprintf(stderr, "\t--%s\n", name->c_str());
      }
      result = false;
    }
  }
  return result;
}



void khGetopt::SetProgName(const char *arg0) {
  if (ProgName.empty()) {
    ProgName = arg0;
    FullProgName = khFindFileInPath(arg0);
  }
}

std::string khGetopt::GetProgramName(void) {
  if (ProgName.empty()) {
    SetProgName(program_invocation_name);
  }
  return ProgName;
}

std::string khGetopt::GetFullProgramName(void) {
  if (ProgName.empty()) {
    SetProgName(program_invocation_name);
  }
  return FullProgName;
}
