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
use Getopt::Long;
use FindBin qw($Script);
use File::Path;
use File::Basename;

my $host;
my $project = 'vector/vector_layers';
my $user;
my $tmpdir = $ENV{TMPDIR};
$tmpdir = '/tmp' unless defined $tmpdir;
my $locale = undef;
my $help = 0;
my $dryrun = 0;
my $fusionbin = '/opt/google/bin';


sub usage {
    warn "usage: $Script [options]\n";
    warn "   Fetch translations from fusion and get them ready to be committed\n";
    warn "Valid options are:\n";
    warn "  --help               Display this message\n";
    warn "  --host <host>        Fusion machine w/ access to assetroot\n";
    warn "  --project <project>  Project to export (default: $project)\n";
    warn "  --locale <localename> Which local to export (default: defaultLocale)\n";
    warn "  --user <user>        ssh user on Fusion machine (default: current user)\n";
    warn "  --tmpdir <tmpdir>    tmp dir for files (default: $tmpdir)\n";
    warn "  --dryrun             Don't do work. Just report\n";
        
    exit(1);
}


my $pwd = `pwd`;
chomp($pwd);

my $perforcedir = 'perforcelocation/googledata/localization/GoogleEarthLayers';
die "You must run $Script from $perforcedir\n"
    unless $pwd =~ /$perforcedir$/;


GetOptions('host=s'       => \$host,
           'project=s'    => \$project,
           'locale=s',    => \$locale,
           'user=s'       => \$user,
           'tmpdir=s'     => \$tmpdir,
           'help'         => \$help,
           'dryrun'       => \$dryrun,
           ) || usage();
usage() if $help;

if (!defined $host) {
    warn "You must specify --host\n\n";
    usage();
}
my $userhost = defined($user) ? "$user\@$host" : "$host";

my $localeName = defined($locale) ? $locale : 'en';


my $srcLayerFName;
my $srcLabelFName;
my $destLayerFName;
my $destLabelFName;
my $extraArgs = '';
if (defined($locale)) {
    $extraArgs .= " --exportLocale $localeName";
    $srcLayerFName  = "$tmpdir/LayerNames_$localeName.xlb";
    $srcLabelFName  = "$tmpdir/LabelNames_$localeName.xlb";
    $destLayerFName = "LayerNames/locales/LayerNames_$localeName.xlb";
    $destLabelFName = "LabelNames/locales/LabelNames_$localeName.xlb";
} else {
    $srcLayerFName  = "$tmpdir/LayerNames.xlb";
    $srcLabelFName  = "$tmpdir/LabelNames.xlb";
    $destLayerFName = "LayerNames/LayerNames.xlb";
    $destLabelFName = "LabelNames/LabelNames.xlb";
}

my $numChanged = 0;


# Get LayerNames
SSHCommand("$fusionbin/geexportvectorl10n $extraArgs -o $srcLayerFName '$project'");
SCPGet($srcLayerFName, "$destLayerFName.new");
SSHCommand("rm $srcLayerFName");

# Get LabelNames
SSHCommand("$fusionbin/geexportlabelsl10n $extraArgs -o $srcLabelFName");
SCPGet($srcLabelFName, "$destLabelFName.new");
SSHCommand("rm $srcLabelFName");

foreach my $file ($destLayerFName, $destLabelFName) {
    if ( -f "$file.new") {
        if (system("diff $file $file.new >/dev/null 2>&1") != 0) {
            ++$numChanged;
            if (!$dryrun) {
                Command("g4 edit $file");
                Command("mv $file.new $file");
            } else {
                warn "Would have updated $file\n";
            }
        }
    } else {
        ++$numChanged;
        if (!$dryrun) {
            Command("mv $file.new $file");
            Command("g4 add $file");
        } else {
            warn "Would have updated $file\n";
        }
    }
}

if ($numChanged) {
    warn "----- SUCCESS -----\n";
    if (!$dryrun) {
        Command("g4 opened");
    }
} else {
    warn "Nothing to update\n";
}

sub SCPGet {
    my ($src, $dest, $opt) = @_;
    $opt = '' unless defined($opt);

    my $cmd = "scp $opt $userhost:$src $dest";
    print "+ $cmd\n";
    if (system($cmd) != 0) {
        die "Copy failed\n";
    }
}

sub SSHCommand {
    my ($cmd) = @_;

    my $mycmd = "ssh $userhost $cmd";
    print "+ $mycmd\n";
    if (system($mycmd) != 0) {
        die "Remote command failed\n";
    }
}

sub Command {
    my ($cmd) = @_;

    print "+ $cmd\n";
    if (system($cmd) != 0) {
        die "Command failed\n";
    }
}
