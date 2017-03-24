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

package RPMVersion;

use strict;
use warnings;


sub new {
    my ($class, $fullverstr) = @_;

    my ($name, $ver, $release);
    if ($fullverstr !~ /[_-]/) {
        # no - characters whatsoever
        $name    = '';
        $ver     = $fullverstr;
        $release = '';
    } else {
        ($name, $ver, $release) =
            ($fullverstr =~ /^(?:(.*)[_-])?([^_-]+)[_-]([^_-]+)$/);
        die "Unable to decode $fullverstr\n"
            unless defined($ver) && defined($release);
        $name = '' unless defined $name;
    }

    return bless {
	name     => $name,
	ver      => $ver,
	release  => $release,
	fullver  => "$ver-$release",
	fullname => "$name-$ver-$release",
    }, $class;
}



sub Compare {
    my ($a, $b) = @_;
    if (ref($a) ne 'RPMVersion') {
        $a = new RPMVersion($a);
    }
    if (ref($b) ne 'RPMVersion') {
        $b = new RPMVersion($b);
    }

    die "You cannot compare versions from different RPMS\n"
	unless lc($a->{name}) eq lc($b->{name});

    my $cmp = CompareChunkedVersions($a->{ver}, $b->{ver});
    if ($cmp == 0) {
        $cmp = CompareChunkedVersions($a->{release}, $b->{release});
    }
    return $cmp;
}



# returns -1 if $a is older
# returns 1 if $b is older
# return 0 is they are the same
sub CompareChunkedVersions {
    my ($a, $b) = @_;

    if ($a eq $b) {
        return 0;
    }

    my @a = SplitChunks($a);
    my @b = SplitChunks($b);

    my $min = @a < @b ? @a : @b;

    for (my $i = 0; $i < $min; ++$i) {
        my $a_alpha = ($a[$i] =~ /^[a-zA-Z]+$/);
        my $b_alpha = ($b[$i] =~ /^[a-zA-Z]+$/);
        if ($a_alpha == $b_alpha) {
            my $cmp = $a_alpha ? ($a[$i] cmp $b[$i]) : ($a[$i] <=> $b[$i]);
            if ($cmp != 0) {
                return $cmp;
            }
        } else {
            return $a_alpha ? -1 : 1;
        }
    }

    # all compents match up to the short length
    # the result now depends on the differing lengths
    # the shorter on is older
    return scalar(@a) <=> scalar(@b);
}



sub SplitChunks {
    my $str = shift;

    # split based on perl boundaries and throw away any pieces
    # that aren't perl words
    my @w1 = grep(/^\w+$/, split(/\b/, $str));

    # split again on '_' (part of perl word)
    # resuling pieces are garanteed to be alphanum
    my @w2 = map(split(/_+/), @w1);

    # split on boundaries between alpha and numberic
    return map(split(/(?:(?<=[a-zA-Z])(?=[0-9]))|(?:(?<=[0-9])(?=[a-zA-Z]))/),
               @w2);
}



1;
