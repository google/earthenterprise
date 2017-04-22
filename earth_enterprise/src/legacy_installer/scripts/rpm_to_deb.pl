#!/usr/bin/perl
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

use File::Copy;

opendir(DIR, ".");
@files = grep(/.*\.rpm$/,readdir(DIR));
closedir(DIR);

foreach my $rpm (@files) { 
    open(RPM, "sudo alien --to-deb --scripts --keep-version $rpm |") || die ("could not call alien");
    print("converting " + <RPM>);
    close(RPM);
    my ($base)=split(/-(\d | noarch)/,$rpm);    

    opendir(DIR, ".");
    my $q = "$base.*\\.deb";
    @debs = grep(/$q$/i,readdir(DIR));
    closedir(DIR);
        
    foreach my $deb (@debs) {
        my ($base)=split(/\.i\d{3}/,$rpm);    
	rename($deb, "$base.i386.deb");
    }
}
