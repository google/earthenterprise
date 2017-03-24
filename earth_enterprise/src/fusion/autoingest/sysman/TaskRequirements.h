/*
 * Copyright 2017 Google Inc.
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

#ifndef __TaskRequirements_h
#define __TaskRequirements_h

#include <set>
#include <string>
#include <map>
#include <khFusionURI.h>
#include <autoingest/sysman/.idl/TaskStorage.h>
#include <autoingest/sysman/.idl/TaskRule.h>


class TaskRequirements
{
  typedef std::map<std::string, TaskRule> RuleMap;
  static RuleMap taskRules;
 public:
  static void LoadFromFiles(void);
  static void Init(void) { LoadFromFiles(); }

  typedef TaskRule::Preference Preference;

  class Input {
   public:
    std::string volume;
    std::string host;
    Preference  localToJob;

    Input(const std::string &v, const std::string &h, Preference l);
  };
  class Output {
   public:
    std::string volume;
    std::string host;
    std::string path;
    uint64      size;
    Preference  localToJob;
    std::vector<Preference> differentVolumes; // indexed by input #
    khFusionURI relocateURI;

    Output(const std::string &v,
           const std::string &h,
           const std::string &p,
           uint64 s,
           Preference local,
           uint numInputs);
  };
  class CPU {
   public:
    uint minNumCPU;
    uint maxNumCPU;

    CPU(uint min = 1, uint max = 1) :
        minNumCPU(min), maxNumCPU(max) { }
  };

  // constraints
  std::vector<Input>  inputs;
  std::vector<Output> outputs;
  CPU                 cpu;

  // calculated results
  std::set<std::string> requiredVolumeHosts;
  std::string requiredBuildHost;
  std::string preferredBuildHost;

  // will throw exception describing conflicts
  TaskRequirements(const TaskDef &taskdef, uint taskid,
                   const std::string &verref);
 private:
  // will throw exception describing conflicts
  void EnsureNoConflicts(void);

  void ApplyInputConstraint(uint index, const TaskRule::InputConstraint &);
  void ApplyOutputConstraint(uint index,
                             const TaskRule::OutputConstraint &,
                             uint taskid, const std::string &verref);
  void ApplyUserSuppliedRules(const TaskDef &taskDef, uint taskid,
                              const std::string &verref);
};

#endif /* __TaskRequirements_h */
