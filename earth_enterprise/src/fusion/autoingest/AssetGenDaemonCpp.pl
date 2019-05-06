#!/usr/bin/perl -w-
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


use strict;
use FindBin;
use Getopt::Long;
use File::Basename;
use lib $FindBin::Bin;
use AssetGen;

my $help = 0;
our $thiscommand = "@ARGV";

sub usage() {
    die "usage: $FindBin::Script <.srcfile> <outputfile>\n";
}
GetOptions("help|?"    => \$help) || usage();
usage() if $help;

my $srcfile = shift;
if (!$srcfile) {
    warn "No .srcfile specified\n";
    usage();
}

my $daemon_cpp = shift;
if (!$daemon_cpp) {
    warn "No output file specified\n";
    usage();
}


# ****************************************************************************
# ***  Read .src file
# ****************************************************************************
my %config;
ReadSrcFile($srcfile, \%config);
my $hasfixconfig  = $config{"${name}AssetImplD"} =~ /FixConfigBeforeUpdateCheck\(void\)/;
my $haspostupdate = $config{"${name}AssetImplD"} =~ /PostUpdate\(void\)/;

my ($template);
$template = "";
if ($base eq 'Composite') {
    if ($singleFormalExtraUpdateArg) {
      $template = "template <typename ProductAssetVersion>";
      $singleFormalExtraUpdateArg =~
        s/ExtraUpdateArg/ExtraUpdateArg\<ProductAssetVersion\>/;
      $formalExtraUpdateArg =~
        s/ExtraUpdateArg/ExtraUpdateArg\<ProductAssetVersion\>/;
    }
}


# *****************************************************************************
# ***  ${name}AssetImpl.h
# ****************************************************************************#
EnsureDirExists($daemon_cpp);
my $fh;
chmod 0777, $daemon_cpp;
open($fh, "> $daemon_cpp") || die "Unable to open $daemon_cpp: $!\n";
EmitAutoGeneratedWarning($fh);

print $fh <<EOF;


#include "${name}AssetD.h"
#include <khFileUtils.h>
#include <khGuard.h>
#include <khxml/khdom.h>
#include <AssetThrowPolicy.h>
using namespace khxml;
EOF

if ($base eq 'Leaf') {
    print $fh <<EOF;
#include <sysman/khAssetManager.h>
EOF
}

print $fh <<EOF;

// ****************************************************************************
// ***  Supplied from ${name}.src
// ****************************************************************************
$config{"AssetD.cpp"}


// ****************************************************************************
// ***  ${name}Factory - Auto generated
// ****************************************************************************
${name}AssetD
${name}Factory::Find(const std::string &ref_ $formaltypearg)
{
    try {
        Asset asset(ref_);
        if (asset &&
            (asset->type == $typeref) &&
            (asset->subtype == "$subtype")) {
            return ${name}AssetD(ref_);
        }
    } catch (...) {
        // do nothing - don't even generate any warnings
    }
    return ${name}AssetD();
}

${name}AssetVersionD
${name}Factory::FindVersion(const std::string &ref_ $formaltypearg)
{
    try {
        AssetVersion version(ref_);
        if (version &&
            (version->type == $typeref) &&
            (version->subtype == "$subtype")) {
            return ${name}AssetVersionD(ref_);
        }
    } catch (...) {
        // do nothing - don't even generate any warnings
    }
    return ${name}AssetVersionD();
}

void
${name}Factory::ValidateRefForInput(const std::string &ref $formaltypearg)
{
    if (AssetVersionRef(ref).Version() == "current") {
        Asset asset = Find(ref $forwardtypearg);
        if (!asset) {
            throw std::invalid_argument(
                "No such " + ToString($typeref) + " $subtype asset: " + ref);
        }
    } else {
        ${name}AssetVersionD version = FindVersion(ref $forwardtypearg);
        if (!version) {
            throw std::invalid_argument(
                "No such " + ToString($typeref) + " $subtype asset version: " +
                ref);
        }
    }
}

std::string
${name}Factory::SubAssetName(
        const std::string &parentAssetRef
        $formaltypearg,
        const std::string &basename)
{
    return AssetDefs::SubAssetName(parentAssetRef, basename,
                                   $actualtypearg, "$subtype");
}


Mutable${name}AssetD
${name}Factory::Make(const std::string &ref_ $formaltypearg,
                     $formalinputarg
                     const khMetaData &meta_,
                     const $config& config_)
{
    typedef ${name}AssetImplD Impl;

    // all of this wrapping is required and makes it nearly impossible
    // to misuse the Handle and khRefGuard class
    return Mutable${name}AssetD
             (khRefGuardFromNew(new Impl
                (AssetStorage::MakeStorage(ref_, $actualtypearg,
                                           "$subtype",$actualinputarg,
                                           meta_),
                 config_)));
}

Mutable${name}AssetD
${name}Factory::FindMake(const std::string &ref_ $formaltypearg,
                         $formalinputarg
                         const khMetaData &meta_,
                         const $config& config_)
{
    // keep hold of it as a mutable so we can change/create it and
    // have the changes automatically saved
    Mutable${name}AssetD asset = Find(ref_ $forwardtypearg);
    if (asset) {
        asset->Modify($forwardinputarg meta_, config_);
        return asset;
    } else {
        return Make(ref_ $forwardtypearg,
                    $forwardinputarg
                    meta_, config_);
    }
}


Mutable${name}AssetD
${name}Factory::FindAndModify(const std::string &ref_ $formaltypearg,
                              $formalinputarg
                              const khMetaData &meta_,
                              const $config& config_)
{
    Mutable${name}AssetD asset = Find(ref_ $forwardtypearg);
    if (asset) {
        asset->Modify($forwardinputarg meta_, config_);
        return asset;
    } else {
        throw khException(kh::tr("$subtype '%2' doesn't exist")
                          .arg(ref_));
    }
}


Mutable${name}AssetD
${name}Factory::MakeNew(const std::string &ref_ $formaltypearg,
                        $formalinputarg
                        const khMetaData &meta_,
                        const $config& config_)
{
    Mutable${name}AssetD asset = Find(ref_ $forwardtypearg);
    if (asset) {
        throw khException(kh::tr("$subtype '%2' already exists")
                          .arg(ref_));
    } else {
        return Make(ref_ $forwardtypearg,
                    $forwardinputarg
                    meta_, config_);
    }
}


$template
Mutable${name}AssetVersionD
${name}Factory::FindMakeAndUpdate(
        const std::string &ref_ $formaltypearg,
        $formalinputarg
        const khMetaData &meta_,
        const $config& config_
        $formalcachedinputarg
        $formalExtraUpdateArg)
{
    Mutable${name}AssetD asset = FindMake(ref_ $forwardtypearg,
                                          $forwardinputarg meta_, config_);
    bool needed = false;
    return asset->MyUpdate(needed $forwardcachedinputarg
                           $forwardExtraUpdateArg);
}

$template
Mutable${name}AssetVersionD
${name}Factory::FindMakeAndUpdateSubAsset(
        const std::string &parentAssetRef
        $formaltypearg,
        const std::string &basename,
        $formalinputarg
        const khMetaData &meta_,
        const $config& config_
        $formalcachedinputarg
        $formalExtraUpdateArg)
{
    return FindMakeAndUpdate
             (AssetDefs::SubAssetName(parentAssetRef, basename,
                                      $actualtypearg, "$subtype")
              $forwardtypearg, $forwardinputarg
              meta_, config_ $forwardcachedinputarg
              $forwardExtraUpdateArg);
}
EOF

if ($withreuse) {
    print $fh <<EOF;

$template
Mutable${name}AssetVersionD
${name}Factory::ReuseOrMakeAndUpdate(
        const std::string &ref_ $formaltypearg,
        $formalinputarg
        const khMetaData &meta_,
        const $config& config_
        $formalcachedinputarg
        $formalExtraUpdateArg)
{
    // make a copy since actualinputarg is macro substituted, so begin() &
    // end() could be called on different temporary objects
    std::vector<std::string> inputarg = $actualinputarg;
    // bind my input versions refs
    std::vector<std::string> boundInputs;
    boundInputs.reserve(inputarg.size());
    std::transform(inputarg.begin(), inputarg.end(), back_inserter(boundInputs),
                   ptr_fun(&AssetVersionRef::Bind));

    Mutable${name}AssetD asset = Find(ref_ $forwardtypearg);
    if (asset) {
	for (AssetStorage::VersionList::const_iterator v
                = asset->versions.begin();
             v != asset->versions.end(); ++v) {
            ${name}AssetVersionD version(*v);
            // Load an asset version without caching since we may not need it
            // (offline, obsolete and so on).
            version.LoadAsTemporary();
            if ((version->state != AssetDefs::Offline) &&
                (version->inputs == boundInputs) &&
                ::IsUpToDate(config_, version->config)) {

#if 0
                notify(NFY_NOTICE,
                       "${name}: ReuseOrMakeAndUpdate (reusing %s)",
                       version->GetRef().c_str());
                notify(NFY_NOTICE, "         boundinputs:");
                for (std::vector<std::string>::const_iterator bi =
                     boundInputs.begin();
                     bi != boundInputs.end(); ++bi) {
                    notify(NFY_NOTICE, "             %s", bi->c_str());
                }
                notify(NFY_NOTICE, "         version inputs:");
                for (std::vector<std::string>::const_iterator iv =
                     version->inputs.begin();
                     iv != version->inputs.end(); ++iv) {
                    notify(NFY_NOTICE, "             %s", iv->c_str());
                }
#endif
                // Tell the storage manager that we're going to use this
                // version again.
                version.MakePermanent();

                return version;
            } else {
              // Tell the storage manager we don't need this one any more.
              version.NoLongerNeeded();
            }
        }
        asset->Modify($forwardinputarg meta_, config_);
    } else {
        asset = Make(ref_ $forwardtypearg,
                     $forwardinputarg
                     meta_, config_);
    }
    bool needed = false;
    return asset->MyUpdate(needed $forwardcachedinputarg
                           $forwardExtraUpdateArg);
}

$template
Mutable${name}AssetVersionD
${name}Factory::ReuseOrMakeAndUpdateSubAsset(
        const std::string &parentAssetRef
        $formaltypearg,
        const std::string &basename,
        $formalinputarg
        const khMetaData &meta_,
        const $config& config_
        $formalcachedinputarg
        $formalExtraUpdateArg)
{
    return ReuseOrMakeAndUpdate(
        AssetDefs::SubAssetName(parentAssetRef, basename,
                                $actualtypearg, "$subtype")
        $forwardtypearg, $forwardinputarg
        meta_, config_  $forwardcachedinputarg
        $forwardExtraUpdateArg);
}
EOF
}


print $fh <<EOF;
// ****************************************************************************
// ***  ${name}AssetImplD -- Auto generated
// ****************************************************************************
EOF

if (@modify_resistant_config_members) {

print $fh <<EOF;
void ${name}AssetImplD::Modify($formalinputarg
            const khMetaData & meta_,
            const Config &config_) {
    Config copy = config_;
EOF

foreach my $member (@modify_resistant_config_members) {
    print $fh "    copy.$member = config.$member;\n";
}

print $fh <<EOF;
    AssetImplD::Modify($actualinputarg, meta_);
    config = copy;
}
EOF

} else {

print $fh <<EOF;
void ${name}AssetImplD::Modify($formalinputarg
            const khMetaData & meta_,
            const Config &config_) {
    AssetImplD::Modify($actualinputarg, meta_);
    config = config_;
}
EOF

}


print $fh <<EOF;

namespace {
    void AddConfig(DOMElement *parent, const $config &config);
}

khRefGuard<${name}AssetImplD>
${name}AssetImplD::Load(const std::string &boundref)
{
    khRefGuard<${name}AssetImplD> result;

    // make sure the base class loader actually instantiated one of me
    // this should always happen, but there are no compile time guarantees
    result.dyncastassign(${name}AssetImpl::Load(boundref));
    if (!result) {
        AssetThrowPolicy::FatalOrThrow(
            "Internal error: ${name}AssetImplD loaded wrong type for " +
            boundref);
    }

    return result;
}

extern void ToElement(DOMElement *elem, const AssetStorage &self);

bool
${name}AssetImplD::Save(const std::string &filename) const
{
    std::unique_ptr<GEDocument> doc = CreateEmptyDocument("${name}Asset");
    if (!doc) {
        notify(NFY_WARN, "Unable to create empty document: ${name}Asset");
        return false;
    }
    bool status = false;
    try {
        DOMElement *top = doc->getDocumentElement();
        if (top) {
            // use a temporary else templated ToElement doesn't
            // know which type to use
            const AssetStorage &storage = *this;
            ToElement(top, storage);
            AddConfig(top, config);
            status = WriteDocument(doc.get(), filename);
            if (!status && khExists(filename)) {
                khUnlink(filename);
            }
        } else {
            notify(NFY_WARN, "Unable to create document element %s",
                   filename.c_str());
        }
    } catch (const XMLException& toCatch) {
        notify(NFY_WARN, "Error saving %s: %s",
               filename.c_str(), XMLString::transcode(toCatch.getMessage()));
    } catch (const DOMException& toCatch) {
        notify(NFY_WARN, "Error saving %s: %s",
               filename.c_str(), XMLString::transcode(toCatch.msg));
    } catch (const std::exception &e) {
        notify(NFY_WARN, "Error saving %s: %s", filename.c_str(), e.what());
    } catch (...) {
        notify(NFY_WARN, "Unable to save %s", filename.c_str());
    }
    return status;
}



EOF

if ($haveBindConfig) {

    print $fh <<EOF
Mutable${name}AssetVersionD
${name}AssetImplD::MakeNewVersion(const ${name}AssetImplD::Config &bound_config)
{
    typedef ${name}AssetVersionImplD VerImpl;

    Mutable${name}AssetVersionD newver
        (khRefGuardFromNew(new VerImpl(this, bound_config)));

    AddVersionRef(newver->GetRef());
    return newver;
}
EOF

} else {

    print $fh <<EOF
Mutable${name}AssetVersionD
${name}AssetImplD::MakeNewVersion(void)
{
    typedef ${name}AssetVersionImplD VerImpl;

    Mutable${name}AssetVersionD newver(khRefGuardFromNew(new VerImpl(this)));

    AddVersionRef(newver->GetRef());
    return newver;
}
EOF

}



# ========== BEGIN - IsUpToDate ==========
{
    my $DEBUG_NO_VERSION = '';
    my $DEBUG_OFFLINE = '';
    my $DEBUG_CONFIG_OUT_OF_DATE = '';
    my $DEBUG_FORCE_UPDATE = '';
    my $DEBUG_INPUTS_OUT_OF_DATE = '';

    if ($debugIsUpToDate) {
        $DEBUG_NO_VERSION =
            "notify(NFY_WARN, \"${name} IsUpToDate: no version\");";
        $DEBUG_OFFLINE =
            "notify(NFY_WARN, \"${name} IsUpToDate: offline\");";
        $DEBUG_CONFIG_OUT_OF_DATE =
            "notify(NFY_WARN, \"${name} IsUpToDate: config out of date\");";
        $DEBUG_FORCE_UPDATE =
            "notify(NFY_WARN, \"${name} IsUpToDate: force update\");";
        $DEBUG_INPUTS_OUT_OF_DATE =
            "notify(NFY_WARN, \"${name} IsUpToDate: inputs out of date\");";
    }


    if ($haveBindConfig) {
        print $fh <<EOF;
bool
${name}AssetImplD::IsUpToDate(const ${name}AssetImplD::Config& bound_config $formalcachedinputarg) const
{
EOF
    } else {
        print $fh <<EOF;
bool
${name}AssetImplD::IsUpToDate($singleformalcachedinputarg) const
{
    const Config &bound_config = config;
EOF
    }



    print $fh <<EOF;
    ${name}AssetVersionD version = CurrVersionRef();
    if (!version) {
        $DEBUG_NO_VERSION
        return false;
    }
EOF
    # if Check_meta_is_update_array has been defined, add check for those
    # meta values
    foreach my $meta_val (@{$config{Check_meta_is_update_array}}) {
      print $fh <<EOF;
    if (version->meta.GetValue("$meta_val") != meta.GetValue("$meta_val")) {
        return false;
    }
EOF
    }
    print $fh <<EOF;
    if (version->state == AssetDefs::Offline) {
        $DEBUG_OFFLINE
        return false;
    }
    // before we check if the configs are different, make sure the asset
    // config has the idl_version field set to the most recent version
    bound_config._UpdateIdlVersion();
    if (!::IsUpToDate(bound_config, version->config)) {
        $DEBUG_CONFIG_OUT_OF_DATE
        return false;
    }
    if (version->MustForceUpdate()) {
        $DEBUG_FORCE_UPDATE
        return false;
    }
    if (!$inputsUpToDate) {
        $DEBUG_INPUTS_OUT_OF_DATE
        return false;
    }
    return true;
}
EOF

}
# ========== END - IsUpToDate ==========


if ($haveExtraUpdateInfo) {
    print $fh <<EOF;

AssetVersionD
${name}AssetImplD::Update(bool &needed) const
{
    throw khException(kh::tr("Internal Error: ${name} Update w/o extra args"));
}

EOF

} elsif ($hasinputs) {
    print $fh <<EOF;

AssetVersionD
${name}AssetImplD::Update(bool &needed) const
{
    return MyUpdate(needed, std::vector<AssetVersion>());
}

EOF

} else {
    print $fh <<EOF;

AssetVersionD
${name}AssetImplD::Update(bool &needed) const
{
    return MyUpdate(needed);
}

EOF

}


if ($hasinputs) {
    print $fh <<EOF;

$template
${name}AssetVersionD
${name}AssetImplD::MyUpdate(bool &needed $formalcachedinputarg
                            $formalExtraUpdateArg) const
{
    std::vector<AssetVersion> updatedInputVers;
    const std::vector<AssetVersion> *inputvers;
    if (cachedinputs_.size()) {
        inputvers = &cachedinputs_;
    } else {
        UpdateInputs(updatedInputVers);
        inputvers = &updatedInputVers;
    }
EOF

if ($hasfixconfig) {
    print $fh <<EOF;
    if (FixConfigBeforeUpdateCheck()) {
        // I've changed something (possibly my input order). So I need to
        // refetch my inputvers
        updatedInputVers.clear();
        UpdateInputs(updatedInputVers);
        inputvers = &updatedInputVers;
    }
EOF
}

if ($haveBindConfig) {
    print $fh <<EOF;
    // now see if I'm up to date
    Config bound_config = BindConfig();
    if (!IsUpToDate(bound_config, *inputvers)) {
        Mutable${name}AssetD self(GetRef());
        Mutable${name}AssetVersionD newver =
            self->MakeNewVersion(bound_config);
        AssetVersionImplD::InputVersionGuard guard(newver.operator->(),
                                                   *inputvers);
EOF
} else {
    print $fh <<EOF;
    // now see if I'm up to date
    if (!IsUpToDate(*inputvers)) {
        Mutable${name}AssetD self(GetRef());
        Mutable${name}AssetVersionD newver = self->MakeNewVersion();
        AssetVersionImplD::InputVersionGuard guard(newver.operator->(),
                                                   *inputvers);
EOF
}


if ($base eq 'Composite') {
    print $fh "        newver->UpdateChildren($singleForwardExtraUpdateArg);\n";
}

print $fh "        newver->SyncState();\n";

if ($haspostupdate) {
    print $fh "        self->PostUpdate();\n";
}


print $fh <<EOF;
        needed = true;
        return newver;
    }
    // If my current version has not changed but has been canceled,
    // then to update I must resume it.
    Mutable${name}AssetVersionD curr_ver(CurrVersionRef());
    AssetVersionImplD::InputVersionGuard guard(curr_ver.operator->(),
                                               *inputvers);
    if (curr_ver->state == AssetDefs::Canceled) {
      needed = true;
      curr_ver->Rebuild();
    }

    return CurrVersionRef();
}
EOF

} else {

    print $fh <<EOF;

$template
${name}AssetVersionD
${name}AssetImplD::MyUpdate(bool &needed $formalExtraUpdateArg) const
{
EOF

if ($hasfixconfig) {
    print $fh "    (void) FixConfigBeforeUpdateCheck();\n";
}

if ($haveBindConfig) {
    print $fh <<EOF;
    // now see if I'm up to date
    Config bound_config = BindConfig();
    if (!IsUpToDate(bound_cofig)) {
        Mutable${name}AssetD self(GetRef());
        Mutable${name}AssetVersionD newver =
            self->MakeNewVersion(bound_config);
EOF
}else {
    print $fh <<EOF;
    // now see if I'm up to date
    if (!IsUpToDate()) {
        Mutable${name}AssetD self(GetRef());
        Mutable${name}AssetVersionD newver = self->MakeNewVersion();
EOF
}

if ($base eq 'Composite') {
    print $fh "        newver->UpdateChildren($singleForwardExtraUpdateArg);\n";
}

print $fh "        newver->SyncState();\n";

if ($haspostupdate) {
    print $fh "        self->PostUpdate();\n";
}


print $fh <<EOF;
        needed = true;
        return newver;
    }
    // If my current version has not changed but has been canceled,
    // then to update I must resume it.
    Mutable${name}AssetVersionD curr_ver(CurrVersionRef());
    if (curr_ver->state == AssetDefs::Canceled) {
      needed = true;
      curr_ver->Rebuild();
    }

    return CurrVersionRef();
}
EOF

}



print $fh <<EOF;


// ****************************************************************************
// ***  ${name}AssetVersionImplD - Auto generated
// ****************************************************************************
khRefGuard<${name}AssetVersionImplD>
${name}AssetVersionImplD::Load(const std::string &boundref)
{
    khRefGuard<${name}AssetVersionImplD> result;

    // make sure the base class loader actually instantiated one of me
    // this should always happen, but there are no compile time guarantees
    result.dyncastassign(${name}AssetVersionImpl::Load(boundref));
    if (!result) {
        AssetThrowPolicy::FatalOrThrow(
            "Internal error: ${name}AssetVersionImplD loaded wrong type for " +
            boundref);
    }

    return result;
}


extern void ToElement(DOMElement *elem, const AssetVersionStorage &self);
bool
${name}AssetVersionImplD::Save(const std::string &filename) const
{
    std::unique_ptr<GEDocument> doc = CreateEmptyDocument("${name}AssetVersion");
    if (!doc) {
        notify(NFY_WARN,
               "Unable to create empty document: ${name}AssetVersion");
        return false;
    }
    bool status = false;
    try {
        DOMElement *top = doc->getDocumentElement();
        if (top) {
            const AssetVersionStorage &storage = *this;
            ToElement(top, storage);
            AddConfig(top, config);
            status = WriteDocument(doc.get(), filename);
            if (!status && khExists(filename)) {
                khUnlink(filename);
            }
        } else {
            notify(NFY_WARN, "Unable to create document element %s",
                   filename.c_str());
        }
    } catch (const std::exception &e) {
        notify(NFY_WARN, "%s while saving %s", e.what(), filename.c_str());
    } catch (...) {
        notify(NFY_WARN, "Unable to save %s", filename.c_str());
    }
    return status;
}


EOF

for (my $i = 0; $i < @ConfigHistory; ++$i) {
    print $fh "extern void ToElement(DOMElement *elem, const $ConfigHistory[$i]  &self);\n";
}
if (@ConfigHistory > 1) {
    print $fh "#include <khxml/khVersionedDOM.h>\n";
}

print $fh "namespace {\n";
print $fh "  void AddConfig(DOMElement *parent, const $config &config) {\n";
if (@ConfigHistory == 1) {
    print $fh "    AddElement(parent, \"config\", config);\n";
} else {
    print $fh "    AddVersionedElement(parent, \"config\", $#ConfigHistory, config);\n";
}
print $fh "  }\n";
print $fh "} // anonymous namespace\n\n";



close($fh);
chmod 0444, $daemon_cpp;