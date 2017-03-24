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
#

package InstallUtils;

use strict;
use warnings;


sub ActivePidFile {
    my $pidfile = shift;
    
    if (-f $pidfile) {
        if (open(PIDFILE, $pidfile)) {
            my $pid = <PIDFILE>;
            chomp $pid;
            close(PIDFILE);

            my $ret = kill(0, $pid);
            return ($ret == 1);
        }
    }

    return 0;
}

sub GetPlatform
{
    if (open(LSB, '/etc/lsb-release')) {
        while (<LSB>) {
            return "ubuntu" if (/Ubuntu/);
        }
        close(LSB);
    }

    if (-f "/etc/sysconfig/grhat") {
        return "grhat-9";
    } else {
        my $uname = `uname -m`;
        chomp $uname;
        return $uname;
    }
}

sub LibMatchesPlatform
{
    my ($file, $platform) = @_;

    my $wantbits = ($platform =~ /64/) ? 64 : 32;
    my $numbits = 0;
    open(FILE, "file $file |") || die "Unable to query file type\n";
    while (<FILE>) {
        if (/ELF 32-bit LSB shared object/) {
            $numbits = 32;
            last;
        } elsif (/ELF 64-bit LSB shared object/) {
            $numbits = 64;
            last;
        }
    }
    close(FILE);

    return ($numbits == $wantbits);
}


sub systemordie
{
    if (system(@_) != 0) {
        die "Failed to execute: @_\n";
    }
}

sub InstallScriptOrDie
{
  my $installer;
  if (-f "/sbin/chkconfig") {
    $installer = "/sbin/chkconfig --add @_";
  } else {
    $installer = "/usr/sbin/update-rc.d @_ defaults 90 10";
  }
  print "Executing command: $installer";
  systemordie($installer);
}

sub RemoveScript
{
  my $remover;
  if (-f "/sbin/chkconfig") {
    $remover = "/sbin/chkconfig --del $_ >/dev/null 2>&1";
  } else {
    $remover = "/usr/sbin/update-rc.d $_ remove >/dev/null 2>&1";
  }
  print "Executing command: $remover";
  system($remover);
}

# Replaces all occurrences of a specified token in a file.
# usage ReplaceTokenInFile(filename, token, value);
sub ReplaceTokenInFile($$$) {
   my $filename = shift @_;
   my $varname = shift @_;
   my $value = shift @_;
   # Escape the string
   $value =~ s/\//\\\//g;
   my $searchpattern="s/$varname/$value/";
   my $command = "sed -i '$searchpattern' $filename";
   `$command`;
}

1;
