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


#include "TaskRequirements.h"

#include <khVolumeManager.h>
#include <khException.h>
#include <khFileUtils.h>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/Misc.h>
#include <khgdal/.idl/khVirtualRaster.h>
#include <iomanip>
#include <autoingest/geAssetRoot.h>
#include <common/performancelogger.h>


// ****************************************************************************
// ***  TaskRequirements::Input
// ****************************************************************************
TaskRequirements::Input::Input(const std::string &v,
                               const std::string &h,
                               Preference local)
    : volume(v), host(h), localToJob(local) {
}


// ****************************************************************************
// ***  TaskRequirements::Output
// ****************************************************************************
TaskRequirements::Output::Output(const std::string &v,
                                 const std::string &h,
                                 const std::string &p,
                                 std::uint64_t s,
                                 Preference local,
                                 unsigned int numInputs) :
    volume(v), host(h), path(p), size(s), localToJob(local),
    differentVolumes(numInputs, TaskRule::Prefer),
    relocateURI()
{
}


// ****************************************************************************
// ***  TaskRequirements
// ****************************************************************************
TaskRequirements::TaskRequirements(const TaskDef &taskdef, unsigned int taskid,
                                   const std::string &verref)
{
  // pre-fill my inputs with volume & host info from taskdef's inputs
  for (std::vector<TaskDef::Input>::const_iterator i =
         taskdef.inputs.begin();
       i != taskdef.inputs.end(); ++i) {
    Preference local = TaskRule::DontCare;

    std::string ipath = i->path;
    std::string volume;
    std::string host;

    // check if this input is a .khm file. If it is we don't want to use
    // it's volume, but rather the volume of its tiles. This is because
    // the .khm is teeny and we don't care if we ever have to pull it
    // across the network. But the tile in the actual image files, that
    // we may want locally.
    if (khHasExtension(ipath, ".khm")) {
      khMosaic mosaic;
      if (mosaic.Load(ipath)) {
        if (mosaic.tiles.size()) {
          ipath = mosaic.tiles[0].file;
        } else {
          // just leave ipath pointing to the mosaic
        }
      } else {
        throw khException(kh::tr("Unable to load '%1'").arg(ipath.c_str()).toUtf8().constData());
      }
    }

    if (khHasExtension(ipath, ".khvr")) {
      khVirtualRaster virtraster;
      if (virtraster.Load(ipath)) {
        if (virtraster.inputTiles.size()) {
          ipath = virtraster.inputTiles[0].file;
        } else {
          // just leave ipath pointing to the mosaic
        }
      } else {
        throw khException(kh::tr("Unable to load '%1'").arg(ipath.c_str()).toUtf8().constData());
      }
    }

    if (khIsURI(ipath)) {
      // leave volume empty
    } else {
      khFusionURI uri = theVolumeManager.DeduceURIFromPath(ipath);
      if (uri.Valid()) {
        volume = uri.Volume();
      } else {
        throw khException(kh::tr("Unable to determine volume for '%1'")
                          .arg(ToQString(ipath)));
      }
    }

    if (volume.size()) {
      host = theVolumeManager.VolumeHost(volume);
      if (host.empty()) {
        throw khException
          (kh::tr("Unable to determine host for volume '%1'")
           .arg(ToQString(volume)));
      }
    }
    inputs.push_back(Input(volume, host, local));
  }

  // pre-fill my outputs w/ info from taskdef's outputs
  for (std::vector<TaskDef::Output>::const_iterator o =
         taskdef.outputs.begin();
       o != taskdef.outputs.end(); ++o) {

    std::string volume = geAssetRoot::VolumeName;

    std::string host = theVolumeManager.VolumeHost(volume);
    if (host.empty()) {
      throw khException
        (kh::tr("Unable to determine host for volume '%1'")
         .arg(ToQString(volume)));
    }

    outputs.push_back(Output(volume, host, o->path, o->sizeEstimate,
                             TaskRule::DontCare, taskdef.inputs.size()));
  }



  // let the user config modify the requirements
  ApplyUserSuppliedRules(taskdef, taskid, verref);


  // determine the required set of hosts for my volumes
  for (std::vector<Input>::const_iterator i = inputs.begin();
       i != inputs.end(); ++i) {
    if (i->host.size()) {
      requiredVolumeHosts.insert(i->host);
    }
  }
  for (std::vector<Output>::const_iterator o = outputs.begin();
       o != outputs.end(); ++o) {
    if (o->volume != "*anytmp*") {
      requiredVolumeHosts.insert(o->host);
    }
  }


  // make sure there are no conflicts
  // this just finds the conflicts that will NEVER be able to be resolved
  // (e.g. output[1] & input[3] are on the same volume, but a rule says
  //  that they shouldn't be)
  // (e.g. input[1] and input[3] are on different hosts but both say they
  //  must be run locally)
  EnsureNoConflicts();
}


// ****************************************************************************
// ***  EnsureNoConflicts
// ****************************************************************************
void
TaskRequirements::EnsureNoConflicts(void)
{
  // check for required host collisions
  // while we're at it, save the required host and preferred host
  for (unsigned int i = 0; i < inputs.size(); ++i) {
    Input &input(inputs[i]);
    if (input.host.size()) {
      if (input.localToJob == TaskRule::Must) {
        if (requiredBuildHost != input.host) {
          if (requiredBuildHost.empty()) {
            requiredBuildHost = input.host;
          } else {
            throw khException
              (kh::tr("Input #%1 requires host '%2', build something else requires host '%3'")
               .arg(ToQString(i), ToQString(input.host))
               .arg(ToQString(requiredBuildHost)));
          }
        }
      } else if ((input.localToJob == TaskRule::Prefer) &&
                 preferredBuildHost.empty()) {
        preferredBuildHost = input.host;
      }
    }
  }

  for (unsigned int i = 0; i < outputs.size(); ++i) {
    Output &output(outputs[i]);
    if (output.volume != "*anytmp*") {
      if (output.localToJob == TaskRule::Must) {
        if (requiredBuildHost != output.host) {
          if (requiredBuildHost.empty()) {
            requiredBuildHost = output.host;
          } else {
            throw khException(kh::tr("Output #%1 requires host '%2', build something else requires host '%3'")
                              .arg(ToQString(i),ToQString(output.host))
                              .arg(ToQString(requiredBuildHost)));
          }
        }
      } else if ((output.localToJob == TaskRule::Prefer) &&
                 preferredBuildHost.empty()) {
        preferredBuildHost = output.host;
      }
    }
  }

  // check for same volume collisions
  for (unsigned int o = 0; o < outputs.size(); ++o) {
    Output &output(outputs[o]);
    for (unsigned int i = 0; i < inputs.size(); ++i) {
      Input &input(inputs[i]);
      if (input.volume.size()) {
        if (output.differentVolumes[i] == TaskRule::Must) {
          if (output.volume == input.volume) {
            throw khException
              (kh::tr("Output #%1 and Input #%2 are both on volume %3")
               .arg(ToQString(o), ToQString(i))
               .arg(ToQString(output.volume)));
          }
        }
      }
    }
  }
}


void
TaskRequirements::ApplyInputConstraint(unsigned int index,
                                       const TaskRule::InputConstraint &ic)
{
  inputs[index].localToJob = ic.localToJob;
}


// need to make this little guy since khExtension's default arg (includeDot)
// means it has an incompatible signature
std::string
GetExtension(const std::string &file)
{
  return khExtension(file);
}


class PathPatternTool
{
  SubstQualMap qualifiers;

  std::string taskid;
  std::string defaultpath;
  std::string vernum;
  std::string assetref;

 public:
  PathPatternTool(unsigned int taskid_,
                  const std::string &defaultpath_,
                  const AssetVersionRef &verref)
      : taskid(ToString(taskid_)),
        defaultpath(defaultpath_),
        assetref(verref.AssetRef())
  {
    qualifiers["basename"] = &khBasename;
    qualifiers["dirname"]  = &khDirname;
    qualifiers["sansext"]  = &khDropExtension;
    qualifiers["ext"]      = &GetExtension;

    unsigned int num = 0;
    FromString(verref.Version(), num);
    std::ostringstream out;
    out << std::setw(3) << std::setfill('0') << num;
    vernum = out.str();
  }

  std::string VarSubst(const std::string &pattern) {
    std::string newpath(pattern);
    newpath = varsubst(newpath, "taskid", taskid);
    newpath = varsubst(newpath, "defaultpath", defaultpath, qualifiers);
    newpath = varsubst(newpath, "vernum", vernum);
    newpath = varsubst(newpath, "assetref", assetref, qualifiers);
    return newpath;
  }

};



void
TaskRequirements::ApplyOutputConstraint(unsigned int index,
                                        const TaskRule::OutputConstraint &oc,
                                        unsigned int taskid,
                                        const std::string &verref)
{
  Output &output(outputs[index]);

  output.localToJob = oc.localToJob;
  if (oc.requiredVolume.size()) {
    if (oc.requiredVolume == "*anytmp*") {
      output.volume = oc.requiredVolume;
      output.host = std::string();
    } else {
      const VolumeDefList::VolumeDef *voldef =
        theVolumeManager.GetVolumeDef(oc.requiredVolume);
      if (!voldef) {
        throw khException(kh::tr("Unknown volume name '%1' in output constraint")
                          .arg(oc.requiredVolume.c_str()).toUtf8().constData());
      }
      output.volume = oc.requiredVolume;
      output.host   = voldef->host;
    }
  }

  // This version ref has been bound long ago. There is no need to
  // bind it now. In fact binding it now would be BAD since this
  // routine runs in a thread that's not allowed to access the asset
  // and version records.
  PathPatternTool patternTool(taskid, output.path, AssetVersionRef(verref));


  if (oc.pathPattern.size()) {
    try {
      output.path = patternTool.VarSubst(oc.pathPattern);
    } catch (const std::exception &e) {
      throw khException(kh::tr("Error in output constraint pathPattern: ") +
                        QString::fromUtf8(e.what()));
    } catch (...) {
      throw khException(kh::tr("Unknown error in output constraint pathPattern"));
    }
  }
  for (std::vector<TaskRule::OutputConstraint::InputPref>::const_iterator ip=
         oc.differentVolumes.begin();
       ip != oc.differentVolumes.end(); ++ip) {
    if (ip->num == -1) {
      for (unsigned int i = 0; i < inputs.size(); ++i) {
        output.differentVolumes[i] = ip->pref;
      }
    } else if ((ip->num >= 0) && (ip->num < (int)inputs.size())) {
      output.differentVolumes[ip->num] = ip->pref;
    } else {
      throw khException(kh::tr("Invalid input constraint number %1.\nValid range is (0, %2].")
                        .arg(ToQString(ip->num),
                             ToQString(inputs.size())));
    }
  }

#if 0 && defined(SUPPORT_TASK_RELOCATE)
  if (oc.relocateVolume.size()) {
    const VolumeDefList::VolumeDef *voldef =
      theVolumeManager.GetVolumeDef(oc.relocateVolume);
    if (!voldef) {
      throw khException(kh::tr("Unknown relocateVolume name '%1' in output constraint")
                        .arg(oc.relocateVolume));
    }

    if (oc.relocatePattern.size()) {
      try {
        output.relocateURI =
          khFusionURI(oc.relocateVolume,
                      patternTool.VarSubst(oc.relocatePattern));
      } catch (const std::exception &e) {
        throw khException(kh::tr("Error in output constraint relocatePattern: ") +
                          QString::fromUtf8(e.what()));
      } catch (...) {
        throw khException(kh::tr("Unknown error in output constraint relocatePattern"));
      }
    } else {
      throw khException(kh::tr("relocateVolume '%1' specified w/o relocatePattern")
                        .arg(oc.relocateVolume));
    }


  } else if (oc.relocatePattern.size()) {
    throw khException(kh::tr("relocatePattern '%1' specified w/o relocateVolume")
                      .arg(oc.relocatePattern));
  }
#endif
}


// ****************************************************************************
// ***  ApplyUserSuppliedRules
// ****************************************************************************
void
TaskRequirements::ApplyUserSuppliedRules(const TaskDef &taskDef,
                                         unsigned int taskid,
                                         const std::string &verref)
{
  // first type to find the taskrule specialized by asset type
  RuleMap::const_iterator found = taskRules.find(ToString(taskDef.assetType) +
                                                 taskDef.taskName);
  if (found == taskRules.end()) {
    // if that's not found, check for a taskrule w/o asset type
    found = taskRules.find(taskDef.taskName);
  }
  if (found != taskRules.end()) {
    const TaskRule &taskrule = found->second;

    // apply the input constraints
    for (std::vector<TaskRule::InputConstraint>::const_iterator ic =
           taskrule.inputConstraints.begin();
         ic != taskrule.inputConstraints.end(); ++ic) {
      if (ic->num == -1) {
        for (unsigned int i = 0; i < inputs.size(); ++i) {
          ApplyInputConstraint(i, *ic);
        }
      } else if ((ic->num >= 0) && (ic->num < (int)inputs.size())) {
        ApplyInputConstraint(ic->num, *ic);
      } else {
        throw khException(kh::tr("Invalid input constraint number %1.\nValid range is (0, %2].")
                          .arg(ToString(ic->num).c_str(),
                               ToQString(inputs.size())));
      }
    }

    // apply the output constraints
    for (std::vector<TaskRule::OutputConstraint>::const_iterator oc =
           taskrule.outputConstraints.begin();
         oc != taskrule.outputConstraints.end(); ++oc) {
      if (oc->num < outputs.size()) {
        ApplyOutputConstraint(oc->num, *oc, taskid, verref);
      } else {
        throw khException(kh::tr("Invalid output constraint number %1.\nValid range is (0, %2].")
                          .arg(ToQString(oc->num),
                               ToQString(outputs.size())));
      }
    }

    // apply the CPU constraints
    cpu.minNumCPU = taskrule.cpuConstraint.minNumCPU;
    cpu.maxNumCPU = taskrule.cpuConstraint.maxNumCPU;
    PERF_CONF_LOGGING( "task_config_mincpu", taskrule.taskname, cpu.minNumCPU  );
    PERF_CONF_LOGGING( "task_config_maxcpu", taskrule.taskname, cpu.maxNumCPU  );
  }
}


// ****************************************************************************
// ***  Init
// ****************************************************************************
TaskRequirements::RuleMap  TaskRequirements::taskRules;

void
TaskRequirements::LoadFromFiles(void)
{
  // clean out old ones
  taskRules.clear();

  // get the list of all the .taskrule files in the config dir
  std::string config_dir = geAssetRoot::Dirname(AssetDefs::AssetRoot(),
                                                geAssetRoot::ConfigDir);
  std::vector<std::string> filenames;
  khGetBasenamesMatchingPattern(config_dir,
                                std::string() /* prefix */,
                                ".taskrule"   /* suffix */,
                                &filenames);

  // load the TaskRules files and fail if there are problems
  for (std::vector<std::string>::const_iterator filename = filenames.begin();
       filename != filenames.end(); ++filename) {
    std::string path = khComposePath(config_dir, *filename);
    std::string rule = khDropExtension(*filename);
    TaskRule taskrule;
    if (taskrule.Load(path)) {
      // this should always be the case, but double check it since
      // there is no way to enforce it
      if (taskrule.taskname != rule) {
        notify(NFY_FATAL, "Error loading %s: taskname != %s",
               path.c_str(), rule.c_str());
      }
      PERF_CONF_LOGGING( "task_config_loadfromfile", path, 0 );
      taskRules.insert(std::make_pair(rule, taskrule));
    } else {
      notify(NFY_FATAL, "Unable to load %s", path.c_str());
    }
  }
}
