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

my $src = shift;

die "PopulateTests.pl <srcfile>\n" unless -f $src;

my $base = $src;
$base =~ s/\.[^.]+$//;

mkdir($base) || die "Unable to mkdir $base: $!\n";



my %tests = ('MOSAIC' => {'grid' => ['--split 3', 'INTERP'],
			  'gridfill' => ['--split 3 --fill 0,0,0', 'INTERP'],
			  'overlap' => ['--split 3 --overlap 50', 'INTERP'],
			  'overlapfill' => ['--split 3 --overlap 50 --fill 0,0,0', 'INTERP'],
			  'single' => ['', 'INTERP'],
			  },
	     'INTERP' => {'palette' => ['--index 256 --type uint8'],
			  'rgb'     => ['',            'DATATYPE'],
			  'grey'    => ['--band 2',    'DATATYPE']
			  },
	     'DATATYPE' => {'uint8'  => ['--type uint8'],
			    'int16'  => ['--type int16'],
			    'float32'=> ['--type float32'],
			  },
	     );



if (-f "$base.lut") {
    $tests{INTERP}->{'lut'} = ["--lut $base.lut --type uint8"];
}



foreach my $proj ('orig', 'repro', 'align') {
    my $projdir = "$base/$proj";
    mkdir($projdir) || die "Unable to mkdir $projdir: $!\n";
    my $projfile = "$projdir/test.tif";

    if ($proj eq 'orig') {
	link($src, $projfile) || die "Unable to link $src to $projfile: $!\n";
    } elsif ($proj eq 'repro') {
	mysystem("gereproject -of GTiff '$src' '$projfile'");
    } elsif ($proj eq 'align') {
	mysystem("gereproject --snapup -of GTiff '$src' '$projfile'");
    } else {
	die "Internal Error: Unexpected projection: $proj\n";
    }

    EmitTests($projdir, $projfile, 'MOSAIC', '');
}



sub EmitTests {
    my ($parentdir, $srcfile, $test, $args) = @_;

    my ($key, $val);
    while (($key, $val) = each %{$tests{$test}}) {
	my $dir = "$parentdir/$key";
	mkdir($dir) || die "Unable to mkdir $dir: $!\n";
	my $newargs = $args .
	    (length($val->[0]) > 0 ? ' ' : '') .
	    $val->[0];
	if (defined $val->[1]) {
	    EmitTests($dir, $srcfile, $val->[1], $newargs);
	} else {
	    my $testfile = "$dir/test.tif";
	    mysystem("gemunge $newargs '$srcfile' '$testfile'");
	}
    }
}

sub mysystem {
    my $cmd = shift;
    print $cmd, "\n";
    system($cmd) && die "\n";
}
