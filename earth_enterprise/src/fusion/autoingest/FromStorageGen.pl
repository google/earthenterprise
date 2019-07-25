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


my $cppfile;
my $daemon = 0;
my $help = 0;
my $thiscommand = "@ARGV";

sub usage() {
    die "usage: $FindBin::Script --daemon --cppfile <cppfile> <plugin name>...\n";
}
GetOptions("cppfile=s" => \$cppfile,
	   "daemon"    => \$daemon,
	   "help|?"    => \$help) || usage();
usage() if $help;
usage() unless defined($cppfile);
my @plugins;
my %pluginpath;
foreach my $tmp (@ARGV) {
    if ($tmp =~ s,^deprecated/,,) {
        $pluginpath{$tmp} = "deprecated/$tmp";
    } else {
        $pluginpath{$tmp} = $tmp;
    }
    push @plugins, $tmp;
}

EnsureDirExists($cppfile);
my $fh;
chmod 0777, $cppfile;
open($fh, "> $cppfile") || die "Unable to open $cppfile: $!\n";
my $indent = '    ';

EmitAutoGeneratedWarning($fh);




print $fh "#include <khRefCounter.h>\n\n";

foreach my $plugin (@plugins) {
    my $assetimpl = $daemon ? "${plugin}AssetImplD" : "${plugin}AssetImpl";
    my $versionimpl = $daemon ? "${plugin}AssetVersionImplD" :
	"${plugin}AssetVersionImpl";
    my $header = $daemon ? "autoingest/sysman/plugins/$pluginpath{$plugin}AssetD.h" :
        "autoingest/plugins/$pluginpath{$plugin}Asset.h";

    print $fh <<EOF;
#include <$header>
#include <memory>

std::shared_ptr<${plugin}AssetImpl>
${plugin}AssetImpl::NewFromStorage(const AssetStorage &storage,
                                   const Config &config)
{
    std::shared_ptr<$assetimpl> retval =
        std::make_shared<$assetimpl>(storage,config);
    retval->AfterLoad();
    return retval;
}

std::shared_ptr<${plugin}AssetVersionImpl>
${plugin}AssetVersionImpl::NewFromStorage(const AssetVersionStorage &storage,
					  const Config &config)
{
    std::shared_ptr<$versionimpl> retval =
        std::make_shared<$versionimpl>(storage,config);
    retval->AfterLoad();
    return retval;
}



EOF

    
}

close($fh);
chmod 0444, $cppfile;



sub EmitAutoGeneratedWarning
{
    my ($fh, $cs) = @_;
    $cs = '//' unless defined $cs;
    print $fh <<WARNING;
$cs ***************************************************************************
$cs ***  This file was AUTOGENERATED with the following command:
$cs ***
$cs ***  $FindBin::Script $thiscommand
$cs ***
$cs ***  Any changes made here will be lost.
$cs ***************************************************************************
WARNING
}

sub EnsureDirExists
{
    my $dir = dirname($_[0]);
    if (! -d $dir) {
	EnsureDirExists($dir);
	mkdir($dir) || die "Unable to mkdir $dir: $!\n";
    }
}
