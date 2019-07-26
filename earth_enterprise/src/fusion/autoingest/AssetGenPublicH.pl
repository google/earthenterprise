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

my $public_h = shift;
if (!$public_h) {
    warn "No output file specified\n";
    usage();
}


# ****************************************************************************
# ***  Read .src file
# ****************************************************************************
my %config;
ReadSrcFile($srcfile, \%config);


# *****************************************************************************
# ***  ${name}AssetImpl.h
# ****************************************************************************#
EnsureDirExists($public_h);
my $fh;
my $hprot = basename($public_h);
$hprot =~ tr/./_/;
chmod 0777, $public_h;
open($fh, "> $public_h") || die "Unable to open $public_h: $!\n";
EmitAutoGeneratedWarning($fh);

print $fh <<EOF;
#ifndef __$hprot
#define __$hprot

#include <autoingest/Asset.h>
#include <autoingest/AssetVersion.h>


// ****************************************************************************
// ***  Supplied from ${name}.src
// ****************************************************************************
$config{"Asset.h"}


/******************************************************************************
***  ${name}AssetImpl
******************************************************************************/
class ${name}AssetImpl : public virtual AssetImpl
{
    friend class DerivedAssetHandle_<Asset, ${name}AssetImpl>;
    friend khRefGuard<AssetImpl> AssetImpl::Load(const std::string &);
public:
    typedef $config Config;
    Config config;

protected:
    //static khRefGuard<${name}AssetImpl> Load(const std::string &ref);

    // used only by ${name}AssetImplD, it has to pass
    // the storage directly to the virtual base classes
    ${name}AssetImpl(const Config& config_)
        : AssetImpl(), config(config_) { }

    ${name}AssetImpl(const AssetStorage &storage, const Config& config_)
        : AssetImpl(storage), config(config_) { }

    static khRefGuard<${name}AssetImpl> NewFromDOM(void *e);
    static khRefGuard<${name}AssetImpl> NewInvalid(const std::string &ref);

    // implemented in ReadOnlyFromStorage.cpp and
    // sysman/SysManFromStorage.cpp
    static khRefGuard<${name}AssetImpl>
    NewFromStorage(const AssetStorage &storage, const Config &config);

    // supplied from ${name}.src ---v
$config{"${name}AssetImpl"}
    // supplied from ${name}.src ---^
};



/******************************************************************************
***  ${name}AssetVersionImpl
******************************************************************************/
class ${name}AssetVersionImpl : public virtual ${base}AssetVersionImpl
{
    friend class DerivedAssetHandle_<AssetVersion, ${name}AssetVersionImpl>;
    friend khRefGuard<AssetVersionImpl> AssetVersionImpl::Load(const std::string &);

public:
    typedef $config Config;
    Config config;

    virtual std::string PluginName(void) const;
protected:
    //static khRefGuard<${name}AssetVersionImpl> Load(const std::string &ref);

    // used only by ${name}AssetVersionImplD, it has to pass
    // the storage directly to the virtual base classes
    ${name}AssetVersionImpl(const Config& config_)
        : AssetVersionImpl(),
          ${base}AssetVersionImpl(),
          config(config_) { }
    ${name}AssetVersionImpl(const AssetVersionStorage &storage,
			    const Config& config_)
        : AssetVersionImpl(storage),
          ${base}AssetVersionImpl(),
          config(config_) { }

    static khRefGuard<${name}AssetVersionImpl> NewFromDOM(void *e);
    static khRefGuard<${name}AssetVersionImpl> NewInvalid(const std::string &ref);

    // implemented in ReadOnlyFromStorage.cpp and
    // sysman/SysManFromStorage.cpp
    static khRefGuard<${name}AssetVersionImpl>
    NewFromStorage(const AssetVersionStorage &storage, const Config &config);


    // supplied from ${name}.src ---v
$config{"${name}AssetVersionImpl"}
    // supplied from ${name}.src ---^
};


/******************************************************************************
***  ${name}Asset & ${name}AssetHandle
******************************************************************************/
typedef DerivedAssetHandle_<Asset, ${name}AssetImpl> ${name}Asset;
typedef DerivedAssetHandle_<AssetVersion, ${name}AssetVersionImpl> ${name}AssetVersion;



#endif /* __$hprot */
EOF
    
close($fh);
chmod 0444, $public_h;
