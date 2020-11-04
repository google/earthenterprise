#!/usr/bin/env bash

#
# Copyright 2018 Open GEE Contributors
#
# Licensed under the Apache license, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the license.
#
# This script is used by Travis build stages (see travis.yml at root) that build
# and cache the third party build. Assumes the following environment variables
# are set:
# CPP_STD    - Should be set to `98` or `11` depending on the standard to build with
# BUILD_TYPE - The type of build `release`, `optimize`, or `internal`
# SERIES	 - This is used to version the cache for major third party changes.

# Terminate with an non-zero exit code, if a command doesn't succeed:
set -e

if test -z "$SERIES" ; then
	SERIES="1" ; fi

case "$CPP_STD" in
    11)
        ;;
    *)
        echo "Unsupported value or unset CPP_STD enviroment variable: $CPP_STD" >&2
        exit 1
        ;;
esac

case "$BUILD_TYPE" in
    release|optimize|internal)
        ;;
    *)
        echo "Unsupported value or unset BUILD_TYPE enviroment variable: $BUILD_TYPE" >&2
        exit 1
        ;;
esac

set -x
cd earth_enterprise/src
rm -rf NATIVE-REL-x86_64

if [ -f $HOME/cache/third_party$SERIES-$CPP_STD.tgz ]; then
   tar xf $HOME/cache/third_party$SERIES-$CPP_STD.tgz;
fi

python2.7 /usr/bin/scons -j3 $BUILD_TYPE=1 qt4_native=1 cpp_standard=gnu++$CPP_STD third_party > build.log
mkdir -p $HOME/cache
tar cfz $HOME/cache/third_party$SERIES-$CPP_STD.tgz NATIVE-REL-x86_64 .sconsign.dblite .sconf_temp

