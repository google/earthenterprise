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

#ifndef __khGetopt_h
#define __khGetopt_h

#include <vector>
#include <map>
#include <set>
#include "khstrconv.h"
#include "khSimpleException.h"
#include <khstl.h>

// convenience macro
#define GetNextArg() ((argn < argc) ? argv[argn++] : 0)
#define GetNextArgAsString() ((argn < argc) ? std::string(argv[argn++]) : std::string())



// ****************************************************************************
// ***  Example usage
// ***
// ***  NOTE: This example is NOT exclusive. but it should give you an idea
// ***  of the sorts of things you can do with khGetopt
// ***
// ***  If you want to use one of the public routines that's not shown in this
// ***  example, just grep for it in the codebase to see how other code is
// ***  using it.

/*

bool help;
int argn;
unsigned int num_cpus = 2;
std::string outdir;
std::vector<std::string> wanted;
enum Mode { Retro, Normal, Experimental };
Mode mode = Normal;
std::string type;
std::string imagery_source;
std::string terrain_source;
double scale = 1.0;
bool debug = false;
std::map<std::string, std::string> meta;


khGetopt options;

// --help and -?
options.helpOpt(help);

// --outdir <dir>
// where <dir> must be specified and already exist as a directory
options.opt("outdir", outdir, &khGetopt::DirExists);
options.setRequired("outdir");

// [--want <string>]...
// (--want can be specified multiple times)
options.vecOpt("want", wanted);

// [--numcpus <num>]
// where 1 <= num <= 4
options.opt("numcpus", num_cpus, &khGetopt::RangeValidator<unsigned int, 1, 4>);

// [{--scale <num> | --feet}]
// Set the scale (e.g. translation ratio to meters). Optional, but exclusive.
options.opt("scale", scale);
options.setOpt("feet", scale, 0.3048);
options.setExclusive("scale", "feet");

// [--debug]
// can be specified w/o an argument
options.flagOpt("debug", debug);


// [--mode <mode>]
// Where mode is one of the valid enum options for Mode
options.choiceOpt("mode", mode,
                  makemap(std::string("Retro"),        Retro,
                          std::string("Normal"),       Normal,
                          std::string("Experimental"), Experimental));

// [--type <type>]
// Where <type> is limited to certain values.
// This is like the choceOpt above, but here the values can convert
// themselves to strings, so we don't need to provide them
options.choiceOpt("type", type,
                  makevec<std::string>(kLayer, kProjectSubtype, kDatabaseSubtype));

// {--imagery <source> | --terrain <source>}
// must have --imagery or --terrain, but not both
options.opt("imagery", imagery_source);
options.opt("terrain", terrain_source);
options.setExclusiveRequired("imagery", "terrain");

// [--meta name=value]...
// multiple name value pairs to populate a std::map
options.mapOpt("meta", meta);


// do the actual processing of all the options
if (!options.processAll(argc, argv, argn) || help) {
  usage(progname);
}

// handle the non-option command line args
{
  const char *arg;
  while ((arg = GetNextArg())) {
    // do something with arg
  }
}



*/
// ****************************************************************************





class khGetopt {
  static std::string ProgName;
  static std::string FullProgName;
  static void SetProgName(const char *arg0);

  class OptBase {
   public:
    std::string name;
    bool hasarg;
    virtual void handleArg(const char *arg) = 0;
    virtual ~OptBase(void) { }

    OptBase(const std::string &name_, bool hasarg_)
        : name(name_), hasarg(hasarg_) { }
  };
  class FlagOption : public OptBase {
    bool &flag;
    bool set;
   public:
    FlagOption(const std::string &name, bool &f, bool toset = true)
        : OptBase(name, false), flag(f), set(toset) { }
    virtual void handleArg(const char *) {
      flag = set;
    }
  };
  class HelpOption : public FlagOption {
   public:
    HelpOption(const std::string &name, bool &f) :
        FlagOption(name, f) { }
  };

  template <class T>
  class SetOption : public OptBase {
    T &t;
    T set;
   public:
    SetOption(const std::string &name, T &t_, const T &toset)
        : OptBase(name, false), t(t_), set(toset) { }
    virtual void handleArg(const char *) {
      t = set;
    }
  };

  template <class T>
  class Option : public OptBase {
    T &t;
    void (*validator)(const T&);
   public:
    Option(const std::string &name, T &t_,
           void (*validate)(const T&) = 0)
        : OptBase(name, true), t(t_), validator(validate) { }
    virtual void handleArg(const char *arg) {
      T tmp;
      FromString(arg, tmp);
      if (validator)
        (*validator)(tmp);
      t = tmp;
    }
  };

  template <class T>
  class AdjustOption : public OptBase {
    T &t;
    void (*adjuster)(T&);
   public:
    AdjustOption(const std::string &name, T &t_,
                 void (*adjust)(T&) = 0)
        : OptBase(name, true), t(t_), adjuster(adjust) { }
    virtual void handleArg(const char *arg) {
      T tmp;
      FromString(arg, tmp);
      if (adjuster)
        (*adjuster)(tmp);
      t = tmp;
    }
  };

  template <class T>
  class ChoiceOption : public OptBase {
    T &t;
    typedef std::map<std::string,T> MapType;
    MapType choices;
   public:
    ChoiceOption(const std::string &name, T &t_, const MapType &choices_) :
        OptBase(name, true), t(t_), choices(choices_) { }
    virtual void handleArg(const char *arg) {
      typename MapType::const_iterator found = choices.find(arg);
      if (found != choices.end()) {
        t = found->second;
      } else {
        std::string choicestr;
        for (typename MapType::const_iterator k = choices.begin();
             k != choices.end(); ++k) {
          choicestr += k->first + "\n";
        }
        throw khSimpleException("Invalid value\n"
                                "Must be one of the following:\n")
            << choicestr;
      }
    }
  };

  template <class T>
  class NoMapChoiceOption : public OptBase {
    T &t;
    std::vector<T> choices;
   public:
    NoMapChoiceOption(const std::string &name, T &t_,
                      const std::vector<T> &choices_) :
        OptBase(name, true), t(t_), choices(choices_) { }
    virtual void handleArg(const char *arg) {
      std::string argstr(arg);
      for (unsigned int i = 0; i < choices.size(); ++i) {
        if (argstr == ToString(choices[i])) {
          t = choices[i];
          return;
        }
      }
      std::string choicestr;
      for (unsigned int i = 0; i < choices.size(); ++i) {
        choicestr += ToString(choices[i]) + "\n";
      }
      throw khSimpleException("Invalid value\n"
                              "Must be one of the following:\n")
          << choicestr;
    }
  };

  template <class T>
  class VectorOption : public OptBase {
    std::vector<T> &t;
    void (*validator)(const T&);
   public:
    VectorOption(const std::string &name, std::vector<T> &t_,
                 void (*validate)(const T&) = 0)
        : OptBase(name, true), t(t_), validator(validate) { }
    virtual void handleArg(const char *arg) {
      T tmp;
      FromString(arg, tmp);
      if (validator)
        (*validator)(tmp);
      t.push_back(tmp);
    }
  };

  template <class T, class U>
  class MapOption : public OptBase {
    std::map<T,U> &t;
    void (*keyvalidator)(const T&);
    void (*valvalidator)(const U&);
   public:
    MapOption(const std::string &name,
              std::map<T,U> &t_,
              void (*keyvalidate)(const T&) = 0,
              void (*valvalidate)(const U&) = 0)
        : OptBase(name, true), t(t_),
          keyvalidator(keyvalidate),
          valvalidator(valvalidate)
    { }
    virtual void handleArg(const char *rawarg) {
      std::string arg = rawarg;
      std::string::size_type sep = arg.find("=");
      if ((sep > 0) && (sep+1 < arg.size())) {
        T key;
        FromString(arg.substr(0, sep), key);
        U val;
        FromString(arg.substr(sep+1, std::string::npos), val);
        if (keyvalidator)
          (*keyvalidator)(key);
        if (valvalidator)
          (*valvalidator)(val);
        t.erase(key);   // last one takes priority
        t.insert(std::make_pair(key, val));
        return;
      }
      throw khSimpleException("Poorly formed map option");
    }
  };

  std::vector<OptBase*> opts;
  bool someDoneWithArgs;
  bool posixlyCorrect;
 public:
  // will return the program name from argv[0]
  // if one of the process routines hasn't been called yet, it will fallback
  // to program_invocation_name from glibc
  static std::string GetProgramName(void);
  static std::string GetFullProgramName(void);


  // do as POSIX says and stop processing options after first non option
  // is found. Note: this doesn't apply if calling processSome, which
  // requires processing contrary to POSIX specifications
  void setPosixlyCorrect(bool set = true) { posixlyCorrect = set; }
  void stopAtFirstNonOption(void) { posixlyCorrect = true; }


  // call these to define the arguments
  void flagOpt(const std::string &name, bool &storage) {
    opts.push_back(new FlagOption(name, storage, true));
    opts.push_back(new FlagOption("no" + name, storage, false));
  }
  void helpOpt(bool &storage) {
    opts.push_back(new HelpOption("help", storage));
    opts.push_back(new HelpOption("?", storage));
  }
  void opt(const std::string &name, bool &storage) {
    opts.push_back(new FlagOption(name, storage, true));
    opts.push_back(new FlagOption("no" + name, storage, false));
  }
  template <class T>
  void opt(const std::string &name, T &storage,
           void (*validate)(const T &) = 0) {
    opts.push_back(new Option<T>(name, storage, validate));
  }
  template <class T>
  void adjustOpt(const std::string &name, T &storage,
                 void (*adjust)(T &) = 0) {
    opts.push_back(new AdjustOption<T>(name, storage, adjust));
  }
  template <class T>
  void setOpt(const std::string &name, T &storage, const T& toset) {
    opts.push_back(new SetOption<T>(name, storage, toset));
  }
  template <class T>
  void choiceOpt(const std::string &name, T &storage,
                 const std::map<std::string,T> &choices) {
    opts.push_back(new ChoiceOption<T>(name, storage, choices));
  }
  template <class T>
  void choiceOpt(const std::string &name, T &storage,
                 const std::vector<T> &choices) {
    opts.push_back(new NoMapChoiceOption<T>(name, storage, choices));
  }
  template <class T>
  void vecOpt(const std::string &name, std::vector<T> &storage,
              void (*validate)(const T &) = 0) {
    opts.push_back(new VectorOption<T>(name, storage, validate));
  }
  template <class T, class U>
  void mapOpt(const std::string &name, std::map<T,U> &storage,
              void (*keyvalidate)(const T &) = 0,
              void (*valvalidate)(const U &) = 0) {
    opts.push_back(new MapOption<T,U>(name, storage,
                                      keyvalidate, valvalidate));
  }


  // setExclusive
  void setExclusive(const std::set<std::string> &set);
  inline void setExclusive(const std::string &n1,
                           const std::string &n2) {
    setExclusive(makeset(n1, n2));
  }
  inline void setExclusive(const std::string &n1,
                           const std::string &n2,
                           const std::string &n3) {
    setExclusive(makeset(n1, n2, n3));
  }
  inline void setExclusive(const std::string &n1,
                           const std::string &n2,
                           const std::string &n3,
                           const std::string &n4) {
    setExclusive(makeset(n1, n2, n3, n4));
  }


  // setExclusiveRequired
  void setExclusiveRequired(const std::set<std::string> &set);
  inline void setExclusiveRequired(const std::string &n1,
                                   const std::string &n2) {
    setExclusiveRequired(makeset(n1, n2));
  }
  inline void setExclusiveRequired(const std::string &n1,
                                   const std::string &n2,
                                   const std::string &n3) {
    setExclusiveRequired(makeset(n1, n2, n3));
  }
  inline void setExclusiveRequired(const std::string &n1,
                                   const std::string &n2,
                                   const std::string &n3,
                                   const std::string &n4) {
    setExclusiveRequired(makeset(n1, n2, n3, n4));
  }

  // setRequired
  void setRequired(const std::set<std::string> &set);
  inline void setRequired(const std::string &name) {
    setRequired(makeset(name));
  }
  inline void setRequired(const std::string &n1,
                          const std::string &n2) {
    setRequired(makeset(n1, n2));
  }
  inline void setRequired(const std::string &n1,
                          const std::string &n2,
                          const std::string &n3) {
    setRequired(makeset(n1, n2, n3));
  }
  inline void setRequired(const std::string &n1,
                          const std::string &n2,
                          const std::string &n3,
                          const std::string &n4) {
    setRequired(makeset(n1, n2, n3, n4));
  }

  // verifies that required and exclusive constrains have been met
  // an emits an error if they have not.
  // NOTE: this is called automatically by processAll()
  bool CheckConstraints(void);


  // ***** validators *****
  static void FileExists(const std::string &fname);
  static void DirExists(const std::string &dname);
  template <class T, T min, T max>
  static void RangeValidator(const T &arg) {
    if (arg < min || arg > max) {
      throw khSimpleException("Out of range >= ")
          << min << " and <= " << max;
    }
  }

  template <class T, T min, T max>
  static void IsEvenNumberInRange(const T &arg) {
    if (arg < min || arg > max) {
      throw khSimpleException("Out of range >= ")
          << min << " and <= " << max;
    }
    else if (arg % 2 != 0){
      throw khSimpleException("Not an even number");
    }
  }
  
  // Process all options, permuting non-options to the end
  // Will emit a warning and return false on failure,
  // otherwise argn will be updated to point to the first non-opt argment.
  // Will stop processing if it encounters '--'
  //
  // NOTE: This routine uses getopt_long_only internally and is therefore
  // not MT-safe
  bool processAll(int argc, char *argv[], int &argn) throw();


  // Process all options up to the next non-opt argument.
  // Will emit a warning and return false on failure,
  // On success it will return true and update nextarg to point to the
  // next non-option argument in argv. Once all opts & args have been
  // processed, the routine will return true and next arg will be set to 0
  // It is expected that you will call this routine from w/in a loop to
  // process all options and arguments
  //
  //    while (1) {
  //      const char *nextarg;
  //      if (!options.processOptionsOnly(argc, argv, nextarg)) {
  //         usage(argv[0]);
  //      } else if (nextarg) {
  //         // deal with interspersed arg
  //      } else {
  //         break;
  //      }
  //    }
  //    if (!options.CheckConstraints()) {
  //       usage(argv[0]);
  //    }
  //
  // NOTE: This routine uses getopt_long_only internally and is therefore
  // not MT-safe
  bool processOptionsOnly(int argc, char *argv[], const char *&nextarg);


  khGetopt(void);
  ~khGetopt(void);

 private:
  class Constraint {
   public:
    Constraint(const std::set<std::string> &set, bool required) :
        set_(set), required_(required)
    { }

    std::set<std::string> set_;
    bool required_;
  };
  std::vector<Constraint> constraints_;
  std::set<std::string> used_options_;
  bool used_help_;

  void HandleArg(int index, const char *arg);


  // private and unimplemented, ownership of pointers
  // in the vector would be an issue
  khGetopt(const khGetopt&);
  khGetopt& operator=(const khGetopt&);
};

#endif /* __khGetopt_h */
