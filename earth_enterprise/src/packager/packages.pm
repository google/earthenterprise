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
package packages;

# list the "current" products that go in each package
# the deprecated packages are handled automatically
our %LIST =
    (
     'google' => {
         platforms => ['i686', 'x86_64'],
         products  => [ 'GEFusionGoogle',
                        'GEServerGoogle',
                        'GEFusionTutorial',
                        'GEPlaces',
                        'ApacheDevel',
                        'GEServerDisconnected',
                       ],
     },
     'pro' => {
         platforms => ['i686', 'x86_64'],
         products  => [ 'GEFusionPro',
                        'GEServer',
                        'GEFusionTutorial',
                        'GEPlaces',
                        'ApacheDevel',
                        'GEServerDisconnected',
                        ],
     },
     'lt' => {
         platforms => ['i686', 'x86_64'],
         products  => [ 'GEFusionLT',
                        'GEServer',
                        'GEFusionTutorial',
                        'GEPlaces',
                        'ApacheDevel',
                        'GEServerDisconnected',
                        ],
     },
     );
                
1;
