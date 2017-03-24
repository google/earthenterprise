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

package TermHelps;


use strict;
use warnings;
use POSIX;
use Term::Cap;
use Term::ANSIColor qw(:constants);

my $termios = new POSIX::Termios;
$termios->getattr;
my $ospeed = $termios->getospeed;

# allocate and initialize a terminal structure
my $terminal = Term::Cap->Tgetent({ OSPEED => $ospeed });

our $WARN_EMITTED;
my @WARN_STACK;

# add some formatting to error messages
BEGIN {
    $WARN_EMITTED = 0;

    $SIG{__DIE__} = sub {
        my @args = @_;
        chomp $args[$#args];
        TermHelps::Beep();
        die(RED, @args, "\n\n",
            "Installation FAILED.\n",
            "Please resolve the errors listed above and re-run the installer.",
            RESET, "\n");
    };

    $SIG{__WARN__} = sub {
        my @args = @_;
        chomp $args[$#args];
        $WARN_EMITTED = 1;
        warn RED, @args, RESET, "\n";
    };
}

sub PushWarnState {
    push @WARN_STACK, $WARN_EMITTED;
}

sub PopWarnState {
    $WARN_EMITTED = pop @WARN_STACK;
}

sub Clear {
    my $force = shift;
    if ($force || !$WARN_EMITTED) {
        $terminal->Tputs('cl', 1, *STDOUT);
    }
}

sub Beep {
    $terminal->Tputs('bl', 1, *STDOUT);
}




1;
