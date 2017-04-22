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

package prompt;

use strict;
use warnings;
use HotKey;
use TermHelps;
use Term::ANSIColor qw(:constants);
use InstallUtils;


sub Confirm {
    my ($msg, $default) = @_;
    $default =~ tr/yn/YN/ if defined $default;

    my $prev = $|;
    $| = 1;
    if (defined $default) {
        print BOLD, "$msg (Y/N)? ", BLUE, "[$default] ", RESET;
    } else {
        print BOLD, "$msg (Y/N)? ", RESET;
    }

    my $key;
    do {
        # beep if it's not the first time
        TermHelps::Beep() if (defined $key);
        $key = HotKey::ReadKey();
        if ($key eq "\n") {
            $key = $default if defined $default;
        } else {
            $key =~ tr/yn/YN/;
        }
    } while (($key ne 'Y') && ($key ne 'N'));

    print "$key\n";
    $| = $prev;
    return $key eq 'Y';
}

sub PressAnyKeyToContinue {
    print BOLD, "Press any key to continue", RESET;
    TermHelps::Beep();
    HotKey::ReadKey();
    print "\n";
}

sub GetNumber {
    my ($msg, $default) = @_;

    while (1) {
        my $prev = $|;
        $| = 1;
        if (defined $default) {
            print BOLD, "$msg: ", BLUE, "[$default] ", RESET;
        } else {
            print BOLD, "$msg: ", RESET;
        }
        $| = $prev;

        my $ans = <STDIN>;
        chomp $ans;

        # allow an empty string to return undef only if an empty string
        # was supplied as the default
        if ((length($ans) == 0) &&
            (defined($default) && (length($default)==0))) {
            return undef;
        }
    
        if ($ans =~ /^\d+$/) {
            return $ans + 0;
        } else {
            TermHelps::Beep();
        }
    }

};

sub GetString {
    my ($msg, $default) = @_;

    while (1) {
        my $prev = $|;
        $| = 1;
        if (defined $default) {
            print BOLD, "$msg: ", BLUE, "[$default] ", RESET;
        } else {
            print BOLD, "$msg: ", RESET;
        }
        $| = $prev;

        my $ans = <STDIN>;
        chomp $ans;

        if (length($ans) == 0) {
            # if we have a default, return it
            # othewise beep and reprompt
            if (defined $default) {
                return $default;
            } else {
                TermHelps::Beep();
            }
        } else {
            return $ans;
        }
    }
};

sub GetPlainFile {
    my ($msg, $default) = @_;
    my $file;
    
    TermHelps::PushWarnState();
    while (1) {
        $file = GetString($msg, $default);
        if (! -f $file) {
            warn "$file is not a plain file\n";
            TermHelps::Beep();
        } else {
            last;
        }
    }
    TermHelps::PopWarnState();
    return $file;
}

sub GetDirectory {
    my ($msg, $default, $allowmissing) = @_;
    my $dir;
    
    TermHelps::PushWarnState();
    while (1) {
        $dir = GetString($msg, $default);
        if (! -d $dir) {
            if ($allowmissing && ! -e $dir) {
                last;
            }
            warn "$dir is not a directory\n";
            TermHelps::Beep();
        } else {
            last;
        }
    }
    TermHelps::PopWarnState();
    return $dir;
}

sub GetSharedLibraryFile {
    my ($msg, $platform, $default) = @_;
    my $file;
    

    TermHelps::PushWarnState();
    while (1) {
        $file = GetPlainFile($msg, $default);
        my $islib = 0;
        open(FILE, "file $file |") || die "Unable to query file type\n";
        while (<FILE>) {
            if (/LSB shared object/) {
                $islib = 1;
                last;
            }
        }
        close(FILE);
        if (!$islib) {
            warn "$file doesn't appear to be a shared library\n";
            TermHelps::Beep();
        } elsif (!InstallUtils::LibMatchesPlatform($file, $platform)) {
            warn "$file doesn't match selected platform: $platform\n";
            TermHelps::Beep();
        } else {
            last;
        }
    }
    TermHelps::PopWarnState();
    return $file;
}


1;
