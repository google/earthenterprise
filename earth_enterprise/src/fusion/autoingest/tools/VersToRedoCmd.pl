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

my @levels;

my $assetroot = "/data2/assets";
my $numpieces = 10;


$| = 1;

die "Please remove ./scripts before proceeding\n" if (-e './scripts');
mkdir('scripts', 0777);

print "Finding commands and elapsedtimes ...\n";
while (<>) {
    chomp;
    my ($lev, $assetref, $version);
    ($lev, $assetref, $version) = /^(\d+)\s+(.+)\?version=(\d+)$/;
    $lev = int $lev;

    my $verref = "$assetref?version=$version";

    my $logfile = sprintf("$assetroot/$assetref/ver%03d/logfile", $version);

    if (open(LOG, "$logfile")) {
        my $line;
        my $cmd = undef;
        my $elapsed = 0;
        while ($line = <LOG>) {
            chomp $line;
            if ($line =~ /^COMMAND: (.+)$/) {
                $cmd = $1;
            }
            if ($line =~ /^ELAPSEDTIME:\s+(\d+)/) {
                $elapsed = int $1;
                last;
            }
        }
        close(LOG);
        if (! defined($cmd)) {
            warn "WARNING: Unable to find COMMAND in $logfile\n";
        }

        # try to account for the overhead of the command invocation
        # Otherwise all 0 sec jobs would go to the same sub piece
        $elapsed += 5;

        push @{$levels[$lev]}, { cmd => $cmd, elapsed => $elapsed };
    } else {
        if ($assetref !~ /source\.k.?source/) {
            warn "WARNING: Unable to open $logfile: $!\n";
        }
    }
}

print "Sorting by elapsedtime ...\n";
for (my $lev = 0; $lev <= $#levels; ++$lev) {
    next unless defined $levels[$lev];

    @{$levels[$lev]} = sort {$b->{elapsed} <=> $a->{elapsed}} @{$levels[$lev]};
}

print "Splitting ...\n";
for (my $lev = 0; $lev <= $#levels; ++$lev) {
    next unless defined $levels[$lev];

    my $levdir = sprintf("scripts/%02d", $lev);
    mkdir($levdir, 0777);

    # make a bunch of empty buckets
    my @buckets;
    for (my $b = 0; $b < $numpieces; ++$b) {
        $buckets[$b] = [ 0, [ ] ];
    }
    
    # fill the buckets - the tasks are aleady reverse sorted by
    # elapsed time
    foreach my $task (@{$levels[$lev]}) {
        # find the most empty bucket
        my $minb = 0;
        for (my $b = 1; $b < $numpieces; ++$b) {
            if ($buckets[$b]->[0] < $buckets[$minb]->[0]) {
                $minb = $b;
            }
        }

        # add this to the bucket
        $buckets[$minb]->[0] += $task->{elapsed};
        push @{$buckets[$minb]->[1]}, $task;
    }


    my $currTaskNum = 0;
    for (my $b = 0; $b < $numpieces; ++$b) {
        # skip empty buckets
        next unless @{$buckets[$b]->[1]};

        my $script = sprintf("$levdir/%02d.sh", $b);
        my $logfile = sprintf("%02d.log", $b);

        open(SCRIPT, "> $script") ||
            die "Unable to open $script for writing: $!\n";
        print SCRIPT "#!/bin/sh\n\n";
        print SCRIPT "rm -f $logfile\n\n";

        print SCRIPT "echo 'Estimated total time: ",
        ElapsedString($buckets[$b]->[0]), "'\n";


        foreach my $task (@{$buckets[$b]->[1]}) {
            print SCRIPT "\necho `date` ': -- $currTaskNum --: est. ",
            ElapsedString($task->{elapsed}), "'\n";
            
      print SCRIPT "$task->{cmd} 1>$logfile 2>&1 || echo 'FAILED \#$currTaskNum'\n";
            ++$currTaskNum;
        }

        close(SCRIPT);
        chmod(0755, $script);
    }
}


sub ElapsedString {
    my $elapsed = shift;
    my $str = "$elapsed sec";

    if ($elapsed >= 60) {
        my $sec  = $elapsed;
        my $hour = int $sec/3600;
        $sec    -= $hour*3600;
        my $min  = int $sec/60;
        $sec    -= $min*60;
        if ($hour > 0) {
            $str .= sprintf(" (%uh%um%us)", $hour, $min, $sec);
        } else {
            $str .= sprintf(" (%um%us)", $min, $sec);
        }
    }
    return $str;
}
