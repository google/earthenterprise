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


$name = shift;
@vers = `gequery --versions $name`;
die "Unable to find any verions for $name\n" unless (@vers);
($ver, $state) = $vers[0] =~ /^(.*): (\w+)$/;
if ($state ne 'Succeeded') {
    die "Most recent version of $name is not 'Succeeded'.\nRefusing to proceed.\n";
}
$num = 0;
for ($i = 1; $i < @vers; ++$i) {
    ($ver, $state) = $vers[$i] =~ /^(.*): (\w+)$/;
    if ($state ne 'Cleaned') {
	++$num;
	print "geclean $ver\n";
	system("geclean $ver") && die "\n";
    }
}
if (!$num) {
    print "Nothing to do\n";
}
