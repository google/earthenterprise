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

package platforms;

our @LIST =
    (
#     {
#         name   => 'grhat-9',
#         rpmdir => 'acharaniya::rpms',
#         arch   => 'i686',
#     },
     {
         name   => 'i686',
         rpmdir => 'gef-sles9-2::rpms',
         arch   => 'i686',
     },
     {
         name   => 'x86_64',
         rpmdir => 'seb::rpms',
         arch   => 'x86_64',
     },
     );
                      

if (`uname -n` eq "ample\n") {
    @LIST =
        (
#            {
#                name   => 'i686',
#                rpmdir => 'khbuild@gef-sles9-2:packages/RPMS/*',
#                arch   => 'i686',
#            },
         {
             name   => 'x86_64',
             rpmdir => 'ample:packages/RPMS/*',
             arch   => 'x86_64',
         },
         );
}

our %HASH;
foreach my $platform (@LIST) {
    $HASH{$platform->{name}} = $platform;
}


1;
