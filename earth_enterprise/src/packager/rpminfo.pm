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

package rpminfo;

my $rel_date = `date +%Y%m%d 2>/dev/null`;
chomp($rel_date);

my $gefusionverrel    = "3.1.1-$rel_date";
my $apacheverrel      = '2.2.9-0';

our %ARCH = (
             'binutils-ge'             => {ver => '2.16.91.0.7-4', srpm => 0},
             'gcc-ge'                  => {ver => '4.1.1-4',       srpm => 0},
             'gcc-ge-runtime'          => {ver => '4.1.1-4',       srpm => 1},
             'gdb-ge'                  => {ver => '6.5-3',         srpm => 0},
             'expat-ge'                => {ver => '2.0.1-0',       srpm => 1},
             'qt-ge'                   => {ver => '3.3.6-8',       srpm => 0},
             'xerces-c-ge'             => {ver => '2.7.0-3',       srpm => 1},
             'libtiff-ge'              => {ver => '3.8.2-2',       srpm => 1},
             'proj-ge'                 => {ver => '4.5.0-0',       srpm => 1},
             'libgeotiff-ge'           => {ver => '1.2.3-4',       srpm => 1},
             'ogdi-ge'                 => {ver => '3.1.5-6',       srpm => 1},
             'gdal-ge'                 => {ver => '1.4.2-2',       srpm => 0},
             'libcurl-ge'              => {ver => '7.16.3-3',      srpm => 1},
             'libjs-ge'                => {ver => '1.5.0.1-10',    srpm => 1},
             'libsgl-ge'               => {ver => '0.8-6',         srpm => 0},
             'openldap-ge'             => {ver => '2.3.39-0',      srpm => 1},
             'openssl-ge'              => {ver => '0.9.8h-0',      srpm => 1},
             'apache-ge'               => {ver => $apacheverrel,   srpm => 1},
             'apache-ge-devel'         => {ver => $apacheverrel,   srpm => 0},
             'tomcat-ge'               => {ver => '6.0.18-0',      srpm => 1},
             'jdk-ge'                  => {ver => '1.6.0-1',       srpm => 0},
             'geos-ge'                 => {ver => '2.2.3-0',       srpm => 1},
             'postgresql-ge'           => {ver => '8.1.8-4',       srpm => 1},
             'postgis-ge'              => {ver => '1.1.7-1',       srpm => 1},
             'GoogleEarthCommon'       => {ver => $gefusionverrel, srpm => 0},
             'GoogleEarthServer'       => {ver => $gefusionverrel, srpm => 0},
             'GoogleEarthServerDisconnected'
                                       => {ver => $gefusionverrel, srpm => 0},
             'GoogleEarthFusionLT'     => {ver => $gefusionverrel, srpm => 0},
             'GoogleEarthFusionPro'    => {ver => $gefusionverrel, srpm => 0},
             'GoogleEarthFusionGoogle' => {ver => $gefusionverrel, srpm => 0},
             );

our %NOARCH =
    (
     'GoogleEarthFusionTutorial' => {ver => $gefusionverrel, srpm => 0},
     'ant-ge'                    => {ver => '1.7.0-1',       srpm => 0},
     'GoogleEarthSearchExample'  => {ver => $gefusionverrel, srpm => 0},
     'GoogleEarthPlacesDB'       => {ver => $gefusionverrel, srpm => 0},
     );
