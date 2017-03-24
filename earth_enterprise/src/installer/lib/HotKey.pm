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

# HotKey.pm
# lifted from a PerlMonks HOWTO page
# no licence was specified
# http://www.perlmonks.org/?node=How%20do%20I%20read%20just%20one%20key%20without%20waiting%20for%20a%20return%20key%3F

# subsequent modifications make


package HotKey;

@ISA = qw(Exporter);
@EXPORT = qw(cbreak cooked readkey);

use strict;
use warnings;
use POSIX qw(:termios_h);

my $fd_stdin = fileno(STDIN);
my $term     = POSIX::Termios->new();
$term->getattr($fd_stdin);

# get original values
my $oterm     = $term->getlflag();

my $sans_icanon        = $oterm & ~ICANON;
my $sans_echo_icanon   = $oterm & ~(ECHO | ICANON);

sub cbreak {
    my $arg = shift;
    $term->setlflag($arg);
    $term->setcc(VTIME, 1);
    $term->setattr($fd_stdin, TCSANOW);
}

sub cooked {
    $term->setlflag($oterm);
    $term->setcc(VTIME, 0);
    $term->setattr($fd_stdin, TCSANOW);
}

sub ReadKey {
    my $key = '';
    cbreak($sans_echo_icanon);
    sysread(STDIN, $key, 1);
    cooked();
    return $key;
}

sub WaitForAnyKey {
    my $key = '';
    cbreak($sans_echo_icanon);
    sysread(STDIN, $key, 1);
    cooked();
}    


END { cooked() }

1;
