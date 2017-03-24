#!/usr/bin/perl -w
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

#
# Run a sanity check on the current system to detect the
# correct installation of Google Earth Fusion
# Usage :
# geecheck [-version=3.0.1]
#   where version defaults to 3.0

require 5; # just to be safe

use strict;
# Standard packages
use FindBin;
use lib "$FindBin::Bin/lib";
BEGIN {
  our $ProductDir = "$FindBin::Bin/products"
}
use Getopt::Long;

# Custom packages
use FileUtils;
use FusionUtils;
use DiagnosticUtils;
use PackageUtils;
use PermissionUtils;
use SystemUtils;

print FileUtils::file_contains('/etc/lsb-release', qr/Ubuntu/) . "\n";
print FileUtils::file_contains('/etc/lsb-release', "Ubuntu") . "\n";
print FileUtils::file_contains('/etc/lsb-release', "REDHAT") . "\n";

