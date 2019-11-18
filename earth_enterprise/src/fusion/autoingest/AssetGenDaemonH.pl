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

my $daemon_h = shift;
if (!$daemon_h) {
    warn "No output file specified\n";
    usage();
}


# ****************************************************************************
# ***  Read .src file
# ****************************************************************************
my %config;
ReadSrcFile($srcfile, \%config);

# ****************************************************************************
# ***  Augment the supplied content with standard pieces
# ****************************************************************************
my %extra;
my ($template);
$template = "";
my $templateName="ProductAssetVersion";

if ($base eq 'Composite') {
    if ($singleFormalExtraUpdateArg) {
      $template = "    template <typename $templateName>";
      $singleFormalExtraUpdateArg =~
        s/ExtraUpdateArg/ExtraUpdateArg\<$templateName\>/;
      $formalExtraUpdateArg =~
        s/ExtraUpdateArg/ExtraUpdateArg\<$templateName\>/;
    }

    $extra{"${name}AssetVersionImplD"} =
        $template 
        . "    void UpdateChildren($singleFormalExtraUpdateArg);\n";

} else {
    $extra{"${name}AssetVersionImplD"} =
	"    virtual void DoSubmitTask(void);\n";
}



# *****************************************************************************
# ***  ${name}AssetImplD.h
# ****************************************************************************#
EnsureDirExists($daemon_h);
my $fh;
my $hprot = basename($daemon_h);
$hprot =~ tr/./_/;
chmod 0777, $daemon_h;
open($fh, "> $daemon_h") || die "Unable to open $daemon_h: $!\n";
EmitAutoGeneratedWarning($fh);

my $header;
if ($deprecated) {
    $header = "autoingest/plugins/deprecated/${name}Asset.h"
} else {
    $header = "autoingest/plugins/${name}Asset.h"
}

print $fh <<EOF;
#ifndef __$hprot
#define __$hprot

#include <$header>
#include <sysman/AssetD.h>
#include <memory>

// ****************************************************************************
// ***  Supplied from ${name}.src
// ****************************************************************************
$config{"AssetD.h"}

class ${name}AssetImplD;

/******************************************************************************
***  ${name}AssetVersionImplD
******************************************************************************/
class ${name}AssetVersionImplD :
    public ${name}AssetVersionImpl,
    public ${base}AssetVersionImplD
{
    friend class ${name}AssetVersionImpl;
    friend class ${name}AssetImplD;
    friend class DerivedAssetHandleD_<${name}AssetVersion, AssetVersionD, ${name}AssetVersionImplD>;
public:
    using AssetType = DerivedAssetHandleD_<${name}Asset, AssetD, ${name}AssetImplD>;
    using MutableAssetType = MutableDerivedAssetHandleD_<AssetType, MutableAssetD>;



    virtual std::string GetName() const;
    virtual void SerializeConfig(khxml::DOMElement*) const;
    virtual uint64 GetHeapUsage() const override;

    // Only used when constructing a new version from an asset.
    // The decision to use the raw ImplD* here was a tough one.
    // Originally it had an asset handle, but the call point is a member
    // function in ${name}AssetImplD that didn't already have a handle
    // So I either needed a way to create a handle from a already existent
    // ImplD* (as opposed to a newly created one) or make this routine
    // deal with a raw ImplD* instead. Since the end code never uses this
    // class directly, I figured it was the safer one to open up a bit.
EOF

if ($haveBindConfig) {
print $fh <<EOF;
    ${name}AssetVersionImplD(${name}AssetImplD *asset,
                             const Config &bound_config);
EOF
} else {
print $fh <<EOF;
    ${name}AssetVersionImplD(${name}AssetImplD *asset);
EOF
}

print $fh <<EOF;

    ${name}AssetVersionImplD(const AssetVersionStorage &storage,
                const Config& config_)
        : AssetVersionImpl(storage),
          ${base}AssetVersionImpl(),
          ${name}AssetVersionImpl(config_),
          ${base}AssetVersionImplD() { }

    static const AssetDefs::Type EXPECTED_TYPE;
    static const std::string EXPECTED_SUBTYPE;

$extra{"${name}AssetVersionImplD"}

    // supplied from ${name}.src ---v
$config{"${name}AssetVersionImplD"}
    // supplied from ${name}.src ---^
};

typedef DerivedAssetHandleD_<${name}AssetVersion, AssetVersionD, ${name}AssetVersionImplD>
    ${name}AssetVersionD;
typedef MutableDerivedAssetHandleD_<${name}AssetVersionD, MutableAssetVersionD>
    Mutable${name}AssetVersionD;



/******************************************************************************
***  ${name}AssetImplD
******************************************************************************/
class ${name}AssetImplD : public ${name}AssetImpl, public AssetImplD
{
    friend class ${name}AssetImpl;
    friend class ${name}Factory;
    friend class DerivedAssetHandleD_<${name}Asset, AssetD, ${name}AssetImplD>;
public:
    virtual std::string GetName() const override;
    virtual void SerializeConfig(khxml::DOMElement*) const override;
    void Modify($formalinputarg
                const khMetaData & meta_,
                const Config &config_);
    virtual uint64 GetHeapUsage() const override;
EOF
    
if ($haveBindConfig) {
print $fh <<EOF;
    virtual bool IsUpToDate(const Config &bound_config $formalcachedinputarg) const;
EOF
} else {
print $fh <<EOF;
    virtual bool IsUpToDate($singleformalcachedinputarg) const;
EOF
}

print $fh <<EOF;
    virtual AssetVersionD Update(bool &needed) const;

protected:
    static std::shared_ptr<${name}AssetImplD> Load(const std::string &ref);

public: // this will nee to be looked at or MyUpdate moved to AssetFactory
    $template
    ${name}AssetVersionD MyUpdate(bool &needed
                                  $formalcachedinputarg
                                  $formalExtraUpdateArg) const;

protected:
    ${name}AssetImplD(const std::string &ref_ $formaltypearg,
		$formalinputarg
                const khMetaData &meta_,
                const Config& config_)
        : AssetImpl(AssetStorage::MakeStorage(ref_, $actualtypearg, "$subtype", $actualinputarg, meta_)),
          ${name}AssetImpl(config_), AssetImplD() { }

public:

    ${name}AssetImplD(const AssetStorage &storage,
			 const Config& config_)
        : AssetImpl(storage),
          ${name}AssetImpl(config_), AssetImplD() { }

    static const AssetDefs::Type EXPECTED_TYPE;
    static const std::string EXPECTED_SUBTYPE;

protected:
EOF


print $fh <<EOF;

    // supplied from ${name}.src ---v
$config{"${name}AssetImplD"}
    // supplied from ${name}.src ---^
};

typedef DerivedAssetHandleD_<${name}Asset, AssetD, ${name}AssetImplD>
    ${name}AssetD;
typedef MutableDerivedAssetHandleD_<${name}AssetD, MutableAssetD>
    Mutable${name}AssetD;


// ****************************************************************************
// ***  ${name}VersionImplD -- out of line so it can be defined
// ***  after ${name}AssetImplD
// ****************************************************************************
EOF

    if ($haveBindConfig) {
        print $fh <<EOF;
inline
${name}AssetVersionImplD::${name}AssetVersionImplD
    (${name}AssetImplD *asset, const Config &bound_config)
    : AssetVersionImpl(MakeStorageFromAsset(*asset)),
      ${base}AssetVersionImpl(),
      ${name}AssetVersionImpl(bound_config),
      ${base}AssetVersionImplD(asset->inputs){}
EOF
    } else {
        print $fh <<EOF;
inline
${name}AssetVersionImplD::${name}AssetVersionImplD(${name}AssetImplD *asset)
    : AssetVersionImpl(MakeStorageFromAsset(*asset)),
      ${base}AssetVersionImpl(),
      ${name}AssetVersionImpl(asset->config),
      ${base}AssetVersionImplD(asset->inputs){}
EOF
    }


print $fh <<EOF;


// ****************************************************************************
// ***  ${name}Factory
// ****************************************************************************
class ${name}Factory
{
public:
    static std::string
    SubAssetName(const std::string &parentAssetRef
                 $formaltypearg,
                 const std::string &basename);
EOF

print $fh <<EOF;
};



#endif /* __$hprot */
EOF
close($fh);
chmod 0444, $daemon_h;
